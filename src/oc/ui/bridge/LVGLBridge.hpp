#pragma once

#include <lvgl.h>

#include <oc/hal/IDisplayDriver.hpp>

namespace oc::ui {

/**
 * @brief Configuration options for LVGL bridge (constexpr-friendly)
 *
 * Contains only rendering options, no runtime pointers.
 */
struct LVGLBridgeConfig {
    /// Render mode (FULL recommended for small displays)
    lv_display_render_mode_t renderMode = LV_DISPLAY_RENDER_MODE_FULL;

    /// Optional second buffer for double-buffering (nullptr = single buffer)
    void* buffer2 = nullptr;
};

/**
 * @brief Runtime buffers for LVGL bridge
 */
struct LVGLBridgeBuffers {
    void* buffer1 = nullptr;      ///< Primary draw buffer (DMAMEM on Teensy)
    void* buffer2 = nullptr;      ///< Optional second buffer for double-buffering
    size_t bufferSize = 0;        ///< Size in bytes, 0 = auto-calculate from driver
};

/**
 * @brief Bridge between LVGL and Open Control display driver
 *
 * Connects LVGL rendering to any IDisplayDriver implementation.
 * Consumer is responsible for LVGL initialization and memory configuration.
 *
 * Prerequisites (consumer responsibility):
 * - Configure lv_conf.h (memory, features, tick)
 * - Call lv_init() before bridge.init()
 * - Provide draw buffer(s)
 *
 * @code
 * // Config.hpp
 * constexpr LVGLBridgeConfig LVGL_CONFIG = {
 *     .renderMode = LV_DISPLAY_RENDER_MODE_FULL
 * };
 *
 * // main.cpp
 * lv_init();
 * lvgl.emplace(*display, Buffers::lvgl, LVGL_CONFIG);
 * lvgl->init();
 * @endcode
 */
class LVGLBridge {
public:
    /**
     * @brief Construct LVGL bridge with driver, buffer, and optional config
     *
     * @param driver     Display driver (must outlive the bridge)
     * @param buffer     Primary draw buffer (DMAMEM lv_color_t[width*height])
     * @param bufferSize Size in bytes, or 0 to auto-calculate from driver dimensions
     * @param config     Optional configuration (render mode, etc.)
     */
    LVGLBridge(hal::IDisplayDriver& driver, void* buffer, size_t bufferSize = 0,
               const LVGLBridgeConfig& config = {});

    ~LVGLBridge();

    // Moveable
    LVGLBridge(LVGLBridge&& other) noexcept;
    LVGLBridge& operator=(LVGLBridge&& other) noexcept;

    // Non-copyable
    LVGLBridge(const LVGLBridge&) = delete;
    LVGLBridge& operator=(const LVGLBridge&) = delete;

    /**
     * @brief Initialize LVGL display
     *
     * Creates lv_display, sets buffers, wires flush callback.
     * Requires lv_init() to have been called first by consumer.
     *
     * @return true if successful
     */
    bool init();

    /**
     * @brief Process LVGL timers and rendering
     *
     * Call this regularly in main loop (typically every frame).
     * Triggers rendering and flush to display driver.
     */
    void refresh();

    /**
     * @brief Check if bridge is initialized
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Get underlying LVGL display
     * @return Display pointer or nullptr if not initialized
     */
    lv_display_t* getDisplay() const { return display_; }

private:
    static void flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

    hal::IDisplayDriver* driver_;
    void* buffer_;
    size_t bufferSize_;
    LVGLBridgeConfig config_;
    lv_display_t* display_ = nullptr;
    bool initialized_ = false;
};

}  // namespace oc::ui
