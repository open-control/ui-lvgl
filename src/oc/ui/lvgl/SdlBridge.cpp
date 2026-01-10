// Include lvgl.h first to get LV_USE_SDL from lv_conf.h
#include <lvgl.h>

#if LV_USE_SDL

#include "SdlBridge.hpp"

namespace oc::ui::lvgl {

SdlBridge::SdlBridge(uint16_t width, uint16_t height,
                     hal::TimeProvider timeProvider,
                     const SdlBridgeConfig& config)
    : width_(width), height_(height),
      timeProvider_(timeProvider), config_(config) {}

SdlBridge::~SdlBridge() {
    // LVGL handles cleanup of display and indevs
}

SdlBridge::SdlBridge(SdlBridge&& other) noexcept
    : width_(other.width_), height_(other.height_),
      timeProvider_(other.timeProvider_), config_(other.config_),
      display_(other.display_) {
    other.display_ = nullptr;
}

SdlBridge& SdlBridge::operator=(SdlBridge&& other) noexcept {
    if (this != &other) {
        width_ = other.width_;
        height_ = other.height_;
        timeProvider_ = other.timeProvider_;
        config_ = other.config_;
        display_ = other.display_;
        other.display_ = nullptr;
    }
    return *this;
}

core::Result<void> SdlBridge::init() {
    // Initialize LVGL (idempotent)
    lv_init();

    // Set tick callback
    lv_tick_set_cb(timeProvider_);

    // Create SDL window and display
    display_ = lv_sdl_window_create(width_, height_);
    if (!display_) {
        return core::Result<void>::err(core::ErrorCode::HARDWARE_INIT_FAILED);
    }

    // Configure window
    lv_sdl_window_set_title(display_, config_.windowTitle);

    if (config_.centered) {
        SDL_Window* window = lv_sdl_window_get_window(display_);
        if (window) {
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }

    // Create LVGL input devices for widget interaction
    if (config_.createInputDevices) {
        lv_group_t* group = lv_group_create();
        lv_group_set_default(group);

        lv_indev_t* mouse = lv_sdl_mouse_create();
        lv_indev_set_group(mouse, group);
        lv_indev_set_display(mouse, display_);

        lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
        lv_indev_set_display(mousewheel, display_);
        lv_indev_set_group(mousewheel, group);

        lv_indev_t* keyboard = lv_sdl_keyboard_create();
        lv_indev_set_display(keyboard, display_);
        lv_indev_set_group(keyboard, group);
    }

    lv_display_set_default(display_);

    return core::Result<void>::ok();
}

void SdlBridge::refresh() {
    lv_timer_handler();
}

SDL_Renderer* SdlBridge::getRenderer() const {
    return display_ ? static_cast<SDL_Renderer*>(lv_sdl_window_get_renderer(display_)) : nullptr;
}

SDL_Window* SdlBridge::getWindow() const {
    return display_ ? lv_sdl_window_get_window(display_) : nullptr;
}

} // namespace oc::ui::lvgl

#endif // LV_USE_SDL
