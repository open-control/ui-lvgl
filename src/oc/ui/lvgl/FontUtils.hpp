#pragma once

/**
 * @file FontUtils.hpp
 * @brief Font loading utilities for LVGL binary fonts
 *
 * Provides safe font loading with retry logic for embedded systems
 * where memory fragmentation can cause transient failures.
 *
 * Usage:
 * @code
 * lv_font_t* font = oc::ui::lvgl::loadBinaryFont(font_data, font_len);
 * if (font) {
 *     // Use font...
 *     oc::ui::lvgl::freeFont(font);
 * }
 * @endcode
 */

#include <cstdint>

#include <lvgl.h>

namespace oc::ui::lvgl {

/**
 * @brief Load a binary font from buffer with retry logic
 *
 * Attempts to load the font multiple times with exponential backoff
 * to handle transient memory allocation failures.
 *
 * @param buffer Pointer to font binary data
 * @param length Size of font data in bytes
 * @param maxRetries Maximum load attempts (default: 5)
 * @param baseDelayMs Initial retry delay in ms (doubles each retry)
 * @return Loaded font pointer, or nullptr on failure
 */
lv_font_t* loadBinaryFont(const uint8_t* buffer, uint32_t length, int maxRetries = 5,
                          int baseDelayMs = 10);

/**
 * @brief Free a previously loaded binary font
 *
 * Safe to call with nullptr.
 *
 * @param font Font to free (set to nullptr after)
 */
void freeFont(lv_font_t*& font);

}  // namespace oc::ui::lvgl
