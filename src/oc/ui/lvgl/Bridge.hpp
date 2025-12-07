#pragma once

#include <lvgl.h>

#include <oc/hal/IDisplayDriver.hpp>
#include <oc/hal/Types.hpp>

namespace oc::ui::lvgl {

/**
 * @brief Configuration options for LVGL bridge (constexpr-friendly)
 */
struct BridgeConfig {
    /// Render mode (FULL recommended for small displays)
    lv_display_render_mode_t renderMode = LV_DISPLAY_RENDER_MODE_FULL;

    /// Optional second buffer for double-buffering
    void* buffer2 = nullptr;

    /// Refresh rate in Hz (0 = use LVGL default)
    uint32_t refreshHz = 0;
};

/**
 * @brief Bridge between LVGL and Open Control display driver
 *
 * Handles all LVGL initialization internally (lv_init, tick, display).
 *
 * @code
 * // Config.hpp
 * constexpr BridgeConfig LVGL_CONFIG = {
 *     .refreshHz = 100
 * };
 *
 * // main.cpp
 * bridge = Bridge(*display, Buffer::lvgl, millis, LVGL::CONFIG);
 * bridge->init();
 * @endcode
 */
class Bridge {
public:
    /**
     * @brief Construct LVGL bridge
     *
     * @param driver  Display driver (must outlive the bridge)
     * @param buffer  Primary draw buffer (DMAMEM lv_color_t[width*height])
     * @param time    Time provider for LVGL tick (e.g., millis)
     * @param config  Optional configuration
     */
    Bridge(hal::IDisplayDriver& driver, void* buffer,
           hal::TimeProvider time,
           const BridgeConfig& config = {});

    ~Bridge();

    // Moveable
    Bridge(Bridge&& other) noexcept;
    Bridge& operator=(Bridge&& other) noexcept;

    // Non-copyable
    Bridge(const Bridge&) = delete;
    Bridge& operator=(const Bridge&) = delete;

    /**
     * @brief Initialize LVGL and display
     *
     * Calls lv_init() (idempotent), sets tick callback,
     * creates display, configures buffers and refresh rate.
     *
     * @return true if successful
     */
    bool init();

    /**
     * @brief Process LVGL timers and rendering
     */
    void refresh();

    bool isInitialized() const { return initialized_; }
    lv_display_t* getDisplay() const { return display_; }

private:
    static void flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

    hal::IDisplayDriver* driver_;
    void* buffer_;
    size_t bufferSize_;
    hal::TimeProvider timeProvider_;
    BridgeConfig config_;
    lv_display_t* display_ = nullptr;
    bool initialized_ = false;
};

}  // namespace oc::ui::lvgl
