#pragma once

/**
 * @file Screen.hpp
 * @brief Application UI root management
 *
 * Provides a configurable root for application UI. By default returns
 * lv_screen_active(), but can be configured to return a different parent
 * (e.g., HwSimulator's screenArea for SDL desktop builds).
 *
 * ## Usage
 *
 * @code
 * // SDL environment setup
 * Screen::setRoot(hwSimulator->getScreenArea());
 *
 * // In contexts (works on all platforms)
 * view_container_ = std::make_unique<ViewContainer>(Screen::root());
 * @endcode
 *
 * On Teensy/WASM without explicit setRoot(), root() returns lv_screen_active().
 */

#include <lvgl.h>

namespace oc::ui::lvgl {

namespace Screen {

/**
 * @brief Set the application UI root
 * @param root LVGL object to use as root for application UI
 *
 * Call this before creating any contexts. On SDL, typically called
 * with HwSimulator's screenArea.
 */
void setRoot(lv_obj_t* root);

/**
 * @brief Get the application UI root
 * @return Configured root, or lv_screen_active() if not set
 *
 * Use this instead of lv_screen_active() when creating top-level
 * UI containers in contexts.
 */
lv_obj_t* root();

}  // namespace Screen

}  // namespace oc::ui::lvgl
