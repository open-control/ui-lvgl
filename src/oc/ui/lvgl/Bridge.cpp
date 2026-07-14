#include "Bridge.hpp"

#include <algorithm>

#include <oc/diagnostics/Performance.hpp>

namespace oc::ui::lvgl {

namespace {

constexpr lv_color_format_t DISPLAY_COLOR_FORMAT = LV_COLOR_FORMAT_RGB565;

}  // namespace

#if OC_ENABLE_STATS
namespace {

uint32_t rectPixelCount(const lv_area_t* area) {
    if (!area) return 0;

    const int32_t width = area->x2 - area->x1 + 1;
    const int32_t height = area->y2 - area->y1 + 1;
    if (width <= 0 || height <= 0) return 0;

    return static_cast<uint32_t>(width) * static_cast<uint32_t>(height);
}

}  // namespace
#endif

Bridge::Bridge(interface::IDisplay& driver, void* buffer,
               oc::type::TimeProvider time,
               const BridgeConfig& config)
    : driver_(&driver)
    , buffer_(buffer)
    , bufferSize_(driver.width() * driver.height()
                  * lv_color_format_get_size(DISPLAY_COLOR_FORMAT))
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
#if OC_ENABLE_STATS
    , refresh_diagnostics_(other.refresh_diagnostics_)
#endif
{
    if (display_) lv_display_set_user_data(display_, this);
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
#if OC_ENABLE_STATS
        refresh_diagnostics_ = other.refresh_diagnostics_;
#endif
        if (display_) lv_display_set_user_data(display_, this);
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

    // The buffer size contract depends on the display color format. Configure
    // RGB565 first so LVGL validates and interprets the raw storage as 2 Bpp.
    lv_display_set_color_format(display_, DISPLAY_COLOR_FORMAT);

    // Set draw buffers
    lv_display_set_buffers(
        display_,
        buffer_,
        config_.buffer2,
        bufferSize_,
        config_.renderMode
    );

    // Wire flush callback to our display driver
    lv_display_set_flush_cb(display_, flushCallback);
    lv_display_set_user_data(display_, this);
#if OC_ENABLE_STATS
    lv_display_add_event_cb(
        display_,
        displayInvalidateEvent,
        LV_EVENT_INVALIDATE_AREA,
        display_
    );
#endif

    // Configure refresh rate if specified
    if (config_.refreshHz > 0) {
        lv_timer_set_period(
            lv_display_get_refr_timer(display_),
            std::max<uint32_t>(1U, 1000U / config_.refreshHz)
        );
    }

    // Set screen background color
    lv_obj_set_style_bg_color(lv_screen_active(), config_.screenBgColor, 0);

    initialized_ = true;
    return R::ok();
}

void Bridge::refresh() {
    if (initialized_) {
#if OC_ENABLE_STATS
        refresh_diagnostics_.invalidatedPixels =
            refresh_diagnostics_.pendingInvalidatedPixels;
        refresh_diagnostics_.pendingInvalidatedPixels = 0;
        refresh_diagnostics_.submittedPixels = 0;
        refresh_diagnostics_.active = true;
#endif
        OC_PERF_SCOPE(perfRefresh, "display.lvgl.refresh");
        lv_timer_handler();
#if OC_ENABLE_STATS
        refresh_diagnostics_.active = false;
        OC_PERF_UNITS(
            perfRefresh,
            refresh_diagnostics_.invalidatedPixels,
            refresh_diagnostics_.submittedPixels
        );
#endif
    }
}

void Bridge::flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto* bridge = static_cast<Bridge*>(lv_display_get_user_data(disp));
    auto* driver = bridge ? bridge->driver_ : nullptr;
    OC_PERF_SCOPE(perfFlush, "display.lvgl.flush-callback");
#if OC_ENABLE_STATS
    const uint32_t areaPixels = rectPixelCount(area);
    OC_PERF_UNITS(perfFlush, areaPixels, 0U);
#endif

    if (driver) {
        uint8_t* buffer = px_map;
        bool directRegionSubmitted = false;

        const auto mode = lv_display_get_render_mode(disp);
        const bool isLastFlush = lv_display_flush_is_last(disp);
        interface::Rect rect{
            .x1 = area->x1,
            .y1 = area->y1,
            .x2 = area->x2,
            .y2 = area->y2
        };

        if (mode == LV_DISPLAY_RENDER_MODE_DIRECT) {
            if (auto* active = lv_display_get_buf_active(disp); active && active->data) {
                buffer = active->data;
            } else if (bridge && bridge->buffer_) {
                buffer = static_cast<uint8_t*>(bridge->buffer_);
            } else {
                buffer = nullptr;
            }

            if (buffer) {
#if OC_ENABLE_STATS
                if (bridge->refresh_diagnostics_.active) {
                    bridge->refresh_diagnostics_.submittedPixels += areaPixels;
                }
#endif
                OC_PERF_UNITS(perfFlush, areaPixels, 1U);
                driver->flushRegion(
                    buffer,
                    rect,
                    static_cast<uint16_t>(lv_display_get_horizontal_resolution(disp)),
                    isLastFlush
                );
                directRegionSubmitted = true;
            }

            if (!directRegionSubmitted && !isLastFlush) {
                lv_display_flush_ready(disp);
                return;
            }
        }

        if (!directRegionSubmitted) {
#if OC_ENABLE_STATS
            if (bridge->refresh_diagnostics_.active) {
                bridge->refresh_diagnostics_.submittedPixels += areaPixels;
            }
#endif
            OC_PERF_UNITS(perfFlush, areaPixels, 1U);
            driver->flush(buffer, rect);
        }
    }

    lv_display_flush_ready(disp);
}

#if OC_ENABLE_STATS
void Bridge::displayInvalidateEvent(lv_event_t* event) {
    auto* display = static_cast<lv_display_t*>(lv_event_get_user_data(event));
    auto* bridge = display
        ? static_cast<Bridge*>(lv_display_get_user_data(display))
        : nullptr;
    lv_area_t* area = lv_event_get_invalidated_area(event);
    if (!bridge || !area) return;

    const uint32_t pixels = rectPixelCount(area);
    if (bridge->refresh_diagnostics_.active) {
        bridge->refresh_diagnostics_.invalidatedPixels += pixels;
    } else {
        bridge->refresh_diagnostics_.pendingInvalidatedPixels += pixels;
    }
}
#endif

}  // namespace oc::ui::lvgl
