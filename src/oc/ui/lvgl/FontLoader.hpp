#pragma once

/**
 * @file FontLoader.hpp
 * @brief Stateless font loading API for embedded systems
 *
 * Provides zero-allocation font management with compile-time font entries
 * stored in flash and runtime load/unload for RAM management.
 *
 * Usage:
 * @code
 * // Define font storage (RAM)
 * struct MyFonts {
 *     lv_font_t* regular = nullptr;
 *     lv_font_t* bold = nullptr;
 * };
 * MyFonts fonts;
 *
 * // Define font entries (Flash)
 * const oc::ui::lvgl::font::Entry FONT_ENTRIES[] = {
 *     {&fonts.regular, regular_bin, regular_len, "Regular", false},
 *     {&fonts.bold, bold_bin, bold_len, "Bold", true},  // essential
 * };
 *
 * // Load/unload
 * oc::ui::lvgl::font::load(FONT_ENTRIES);
 * oc::ui::lvgl::font::unload(FONT_ENTRIES);
 * @endcode
 */

#include <cstddef>
#include <cstdint>

#include <lvgl.h>

namespace oc::ui::lvgl::font {

#if LV_USE_FS_MEMFS

/**
 * @brief Font entry descriptor
 *
 * Stored in flash (const). Points to:
 * - target: RAM location for loaded font pointer
 * - data: Flash location of binary font data
 *
 * @note Requires LV_USE_FS_MEMFS to be enabled in lv_conf.h
 */
struct Entry {
    lv_font_t** target;      ///< RAM: where to store loaded font
    const uint8_t* data;     ///< Flash: binary font data
    uint32_t size;           ///< Size of font data
    const char* name;        ///< Debug name
    bool essential;          ///< Load during boot/splash
};

// =============================================================================
// Core API (non-template)
// =============================================================================

/**
 * @brief Load all fonts where *target == nullptr
 *
 * Idempotent: already-loaded fonts are skipped.
 *
 * @param entries Pointer to font entry array
 * @param count Number of entries
 */
void load(const Entry* entries, size_t count);

/**
 * @brief Load only essential fonts where *target == nullptr
 *
 * Use for boot/splash screen when only critical fonts are needed.
 *
 * @param entries Pointer to font entry array
 * @param count Number of entries
 */
void loadEssential(const Entry* entries, size_t count);

/**
 * @brief Unload all fonts where *target != nullptr
 *
 * Frees RAM used by loaded fonts. Safe to call multiple times.
 *
 * @param entries Pointer to font entry array
 * @param count Number of entries
 */
void unload(const Entry* entries, size_t count);

/**
 * @brief Count loaded fonts
 *
 * @param entries Pointer to font entry array
 * @param count Number of entries
 * @return Number of fonts where *target != nullptr
 */
size_t countLoaded(const Entry* entries, size_t count);

// =============================================================================
// Template API (size deduced from array)
// =============================================================================

/**
 * @brief Load all fonts from static array
 * @tparam N Array size (auto-deduced)
 */
template<size_t N>
inline void load(const Entry (&entries)[N]) {
    load(entries, N);
}

/**
 * @brief Load essential fonts from static array
 * @tparam N Array size (auto-deduced)
 */
template<size_t N>
inline void loadEssential(const Entry (&entries)[N]) {
    loadEssential(entries, N);
}

/**
 * @brief Unload all fonts from static array
 * @tparam N Array size (auto-deduced)
 */
template<size_t N>
inline void unload(const Entry (&entries)[N]) {
    unload(entries, N);
}

/**
 * @brief Count loaded fonts in static array
 * @tparam N Array size (auto-deduced)
 */
template<size_t N>
inline size_t countLoaded(const Entry (&entries)[N]) {
    return countLoaded(entries, N);
}

#endif  // LV_USE_FS_MEMFS

}  // namespace oc::ui::lvgl::font
