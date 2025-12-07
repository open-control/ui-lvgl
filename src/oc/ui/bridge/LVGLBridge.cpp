#include "LVGLBridge.hpp"

namespace oc::ui {

LVGLBridge::LVGLBridge(hal::IDisplayDriver& driver, void* buffer,
                       hal::TimeProvider time,
                       const LVGLBridgeConfig& config)
    : driver_(&driver)
    , buffer_(buffer)
    , bufferSize_(driver.width() * driver.height() * sizeof(lv_color_t))
    , timeProvider_(time)
    , config_(config)
{}

LVGLBridge::~LVGLBridge() {
    if (display_) {
        lv_display_delete(display_);
        display_ = nullptr;
    }
}

LVGLBridge::LVGLBridge(LVGLBridge&& other) noexcept
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

LVGLBridge& LVGLBridge::operator=(LVGLBridge&& other) noexcept {
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

bool LVGLBridge::init() {
    if (initialized_) return true;

    if (!driver_) return false;
    if (!buffer_) return false;
    if (!timeProvider_) return false;

    // Initialize LVGL (idempotent - safe to call multiple times)
    lv_init();

    // Set tick callback for LVGL timing
    lv_tick_set_cb(timeProvider_);

    // Create display with dimensions from driver
    display_ = lv_display_create(driver_->width(), driver_->height());
    if (!display_) return false;

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

    initialized_ = true;
    return true;
}

void LVGLBridge::refresh() {
    if (initialized_) {
        lv_timer_handler();
    }
}

void LVGLBridge::flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    auto* driver = static_cast<hal::IDisplayDriver*>(lv_display_get_user_data(disp));

    if (driver) {
        hal::Rect rect{
            .x1 = area->x1,
            .y1 = area->y1,
            .x2 = area->x2,
            .y2 = area->y2
        };
        driver->flush(px_map, rect);
    }

    lv_display_flush_ready(disp);
}

}  // namespace oc::ui
