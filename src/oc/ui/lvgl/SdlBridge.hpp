#pragma once

// Include lvgl.h first to get LV_USE_SDL from lv_conf.h
#include <lvgl.h>

#if LV_USE_SDL

#include <SDL.h>
#include <oc/type/Ids.hpp>
#include <oc/type/Callbacks.hpp>
#include <oc/type/Result.hpp>

namespace oc::ui::lvgl {

struct SdlBridgeConfig {
    const char* windowTitle = "Open Control";
    bool centered = true;
    bool resizable = false;
    bool createInputDevices = true;  // Create mouse, keyboard, mousewheel LVGL indevs
};

/**
 * @brief SDL bridge for LVGL desktop development
 *
 * Wraps LVGL's SDL driver (lv_sdl_window_create) with a clean C++ interface.
 * Handles LVGL initialization, tick callback, and optional input devices.
 *
 * @code
 * SdlBridge bridge(1013, 1013, SDL_GetTicks, {.windowTitle = "My App"});
 * bridge.init();
 *
 * while (running) {
 *     bridge.refresh();
 *     // ... compositing with HwSimulator
 * }
 * @endcode
 */
class SdlBridge {
public:
    /**
     * @brief Construct SDL bridge for LVGL
     *
     * @param width Window width in pixels (consumer decides: PANEL_SIZE or SCREEN_W)
     * @param height Window height in pixels
     * @param timeProvider Time function for LVGL tick (e.g., SDL_GetTicks)
     * @param config Optional configuration
     */
    SdlBridge(uint16_t width, uint16_t height,
              oc::type::TimeProvider timeProvider,
              const SdlBridgeConfig& config = {});
    ~SdlBridge();

    // Non-copyable, moveable
    SdlBridge(const SdlBridge&) = delete;
    SdlBridge& operator=(const SdlBridge&) = delete;
    SdlBridge(SdlBridge&&) noexcept;
    SdlBridge& operator=(SdlBridge&&) noexcept;

    /**
     * @brief Initialize LVGL and SDL display
     *
     * Creates SDL window, LVGL display, and optionally input devices.
     * Input devices (mouse, keyboard, mousewheel) are created by default
     * to enable interaction with LVGL widgets.
     */
    oc::type::Result<void> init();

    /**
     * @brief Process LVGL timers
     *
     * Calls lv_timer_handler(). For compositing scenarios, call this
     * between SDL_SetRenderTarget() switches.
     */
    void refresh();

    bool isInitialized() const { return display_ != nullptr; }
    lv_display_t* getDisplay() const { return display_; }

    /**
     * @brief Get SDL renderer for compositing
     *
     * Use this to render custom content (HwSimulator) alongside LVGL.
     * The renderer is owned by LVGL's SDL driver.
     */
    SDL_Renderer* getRenderer() const;

    /**
     * @brief Get SDL window for window operations
     */
    SDL_Window* getWindow() const;

    uint16_t width() const { return width_; }
    uint16_t height() const { return height_; }

private:
    uint16_t width_;
    uint16_t height_;
    oc::type::TimeProvider timeProvider_;
    SdlBridgeConfig config_;
    lv_display_t* display_ = nullptr;
};

}  // namespace oc::ui::lvgl

#endif // LV_USE_SDL
