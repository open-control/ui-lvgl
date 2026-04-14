#include "Bridge.hpp"

#include <oc/log/Log.hpp>

namespace oc::ui::lvgl {

#if defined(PERF_LOG)
namespace {

struct FlushPerfWindow {
    uint32_t startedAtMs = 0;
    uint32_t callbackCount = 0;
    uint32_t submittedCount = 0;
    uint32_t skippedCount = 0;
    uint32_t dirtyPixels = 0;
    uint32_t submittedPixels = 0;
    uint32_t maxAreaPixels = 0;
    uint32_t largeAreaCount = 0;
    uint32_t fullAreaCount = 0;
};

FlushPerfWindow g_flushPerfWindow;

uint32_t rectPixelCount(const lv_area_t* area) {
    if (!area) return 0;

    const int32_t width = area->x2 - area->x1 + 1;
    const int32_t height = area->y2 - area->y1 + 1;
    if (width <= 0 || height <= 0) return 0;

    return static_cast<uint32_t>(width) * static_cast<uint32_t>(height);
}

void maybeLogFlushPerfWindow(uint32_t nowMs) {
    auto& window = g_flushPerfWindow;
    if (window.startedAtMs == 0) {
        window.startedAtMs = nowMs;
        return;
    }

    if ((nowMs - window.startedAtMs) < 1000) {
        return;
    }

    if (window.callbackCount > 0) {
        const uint32_t avgAreaPixels = window.dirtyPixels / window.callbackCount;
        const uint32_t avgSubmittedPixels =
            (window.submittedCount > 0) ? (window.submittedPixels / window.submittedCount) : 0;

        OC_LOG_INFO(
            "[Perf][Display][LVGL] callbacks={} submitted={} skipped={} dirtyPx={} "
            "submittedPx={} maxAreaPx={} avgAreaPx={} avgSubmittedPx={} largeAreas={} fullAreas={}",
            window.callbackCount,
            window.submittedCount,
            window.skippedCount,
            window.dirtyPixels,
            window.submittedPixels,
            window.maxAreaPixels,
            avgAreaPixels,
            avgSubmittedPixels,
            window.largeAreaCount,
            window.fullAreaCount
        );
    }

    window = {};
    window.startedAtMs = nowMs;
}

void displayInvalidateEvent(lv_event_t* e) {
    lv_display_t* disp = static_cast<lv_display_t*>(lv_event_get_target(e));
    lv_area_t* area = lv_event_get_invalidated_area(e);
    if (!disp || !area) return;

    const uint32_t nowMs = lv_tick_get();
    auto& window = g_flushPerfWindow;
    if (window.startedAtMs == 0) {
        window.startedAtMs = nowMs;
    }

    const int32_t hor = lv_display_get_horizontal_resolution(disp);
    const int32_t ver = lv_display_get_vertical_resolution(disp);
    if (hor <= 0 || ver <= 0) return;

    const uint32_t screenPixels = static_cast<uint32_t>(hor) * static_cast<uint32_t>(ver);
    const uint32_t areaPixels = rectPixelCount(area);
    if (areaPixels >= (screenPixels / 2U)) {
        ++window.largeAreaCount;
    }
    if (areaPixels >= screenPixels) {
        ++window.fullAreaCount;
    }
}

}  // namespace
#endif

Bridge::Bridge(interface::IDisplay& driver, void* buffer,
               oc::type::TimeProvider time,
               const BridgeConfig& config)
    : driver_(&driver)
    , buffer_(buffer)
    , bufferSize_(driver.width() * driver.height() * sizeof(lv_color_t))
    , timeProvider_(time)
    , config_(config)
{}

Bridge::~Bridge() {
    if (display_) {
        lv_display_delete(display_);
        display_ = nullptr;
    }
}

Bridge::Bridge(Bridge&& other) noexcept
    : driver_(other.driver_)
    , buffer_(other.buffer_)
    , bufferSize_(other.bufferSize_)
    , timeProvider_(other.timeProvider_)
    , config_(other.config_)
    , display_(other.display_)
    , initialized_(other.initialized_)
{
    other.display_ = nullptr;
    other.initialized_ = false;
}

Bridge& Bridge::operator=(Bridge&& other) noexcept {
    if (this != &other) {
        if (display_) {
            lv_display_delete(display_);
        }
        driver_ = other.driver_;
        buffer_ = other.buffer_;
        bufferSize_ = other.bufferSize_;
        timeProvider_ = other.timeProvider_;
        config_ = other.config_;
        display_ = other.display_;
        initialized_ = other.initialized_;
        other.display_ = nullptr;
        other.initialized_ = false;
    }
    return *this;
}

oc::type::Result<void> Bridge::init() {
    using R = oc::type::Result<void>;
    using E = oc::type::ErrorCode;

    if (initialized_) return R::ok();

    if (!driver_) return R::err({E::INVALID_ARGUMENT, "display driver required"});
    if (!buffer_) return R::err({E::INVALID_ARGUMENT, "buffer required"});
    if (!timeProvider_) return R::err({E::INVALID_ARGUMENT, "time provider required"});

    // Initialize LVGL (idempotent - safe to call multiple times)
    lv_init();

    // Set tick callback for LVGL timing
    lv_tick_set_cb(timeProvider_);

    // Create display with dimensions from driver
    display_ = lv_display_create(driver_->width(), driver_->height());
    if (!display_) return R::err({E::HARDWARE_INIT_FAILED, "LVGL display create"});

    // Set draw buffers
    lv_display_set_buffers(
        display_,
        buffer_,
        config_.buffer2,
        bufferSize_,
        config_.renderMode
    );

    // Set color format (RGB565 for ILI9341 and similar displays)
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565);

    // Wire flush callback to our display driver
    lv_display_set_flush_cb(display_, flushCallback);
    lv_display_set_user_data(display_, driver_);
#if defined(PERF_LOG)
    lv_display_add_event_cb(display_, displayInvalidateEvent, LV_EVENT_INVALIDATE_AREA, nullptr);
#endif

    // Configure refresh rate if specified
    if (config_.refreshHz > 0) {
        lv_timer_set_period(lv_display_get_refr_timer(display_),
                            1000 / config_.refreshHz);
    }

    // Set screen background color
    lv_obj_set_style_bg_color(lv_screen_active(), config_.screenBgColor, 0);

    initialized_ = true;
    return R::ok();
}

void Bridge::refresh() {
    if (initialized_) {
        lv_timer_handler();
    }
}

void Bridge::flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto* driver = static_cast<interface::IDisplay*>(lv_display_get_user_data(disp));
#if defined(PERF_LOG)
    const uint32_t nowMs = lv_tick_get();
    const uint32_t areaPixels = rectPixelCount(area);
    auto& perf = g_flushPerfWindow;
    if (perf.startedAtMs == 0) {
        perf.startedAtMs = nowMs;
    }
    ++perf.callbackCount;
    perf.dirtyPixels += areaPixels;
    if (areaPixels > perf.maxAreaPixels) {
        perf.maxAreaPixels = areaPixels;
    }
#endif

    if (driver) {
        uint8_t* buffer = px_map;

        const auto mode = lv_display_get_render_mode(disp);
        if (mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
            // DIRECT mode may flush several dirty areas. For framebuffer-based
            // drivers that diff against the full frame, trigger transfer once
            // on the last area and always provide the active framebuffer base.
            if (!lv_display_flush_is_last(disp)) {
#if defined(PERF_LOG)
                ++perf.skippedCount;
                maybeLogFlushPerfWindow(nowMs);
#endif
                lv_display_flush_ready(disp);
                return;
            }

            if (auto* active = lv_display_get_buf_active(disp); active && active->data) {
                buffer = active->data;
            }
        }

        interface::Rect rect{
            .x1 = area->x1,
            .y1 = area->y1,
            .x2 = area->x2,
            .y2 = area->y2
        };
#if defined(PERF_LOG)
        ++perf.submittedCount;
        perf.submittedPixels += areaPixels;
#endif
        driver->flush(buffer, rect);
    }

#if defined(PERF_LOG)
    maybeLogFlushPerfWindow(nowMs);
#endif
    lv_display_flush_ready(disp);
}

}  // namespace oc::ui::lvgl
