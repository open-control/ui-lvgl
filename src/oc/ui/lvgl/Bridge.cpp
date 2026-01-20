#include "Bridge.hpp"

namespace oc::ui::lvgl {

Bridge::Bridge(interface::IDisplay& driver, void* buffer,
               oc::TimeProvider time,
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

oc::Result<void> Bridge::init() {
    using R = oc::Result<void>;
    using E = oc::ErrorCode;

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

    if (driver) {
        interface::Rect rect{
            .x1 = area->x1,
            .y1 = area->y1,
            .x2 = area->x2,
            .y2 = area->y2
        };
        driver->flush(px_map, rect);
    }

    lv_display_flush_ready(disp);
}

}  // namespace oc::ui::lvgl
