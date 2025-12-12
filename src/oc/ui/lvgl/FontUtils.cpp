#include "FontUtils.hpp"

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <chrono>
#include <thread>
#endif

namespace oc::ui::lvgl {

namespace {

void delayMs(int ms) {
#ifdef ARDUINO
    delay(ms);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

}  // namespace

lv_font_t* loadBinaryFont(const uint8_t* buffer, uint32_t length, int maxRetries, int baseDelayMs) {
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        lv_font_t* font = lv_binfont_create_from_buffer(const_cast<void*>(static_cast<const void*>(buffer)), length);
        if (font != nullptr) {
            return font;
        }
        int delayTime = baseDelayMs << attempt;  // Exponential backoff
        delayMs(delayTime);
    }
    return nullptr;
}

void freeFont(lv_font_t*& font) {
    if (font) {
        lv_binfont_destroy(font);
        font = nullptr;
    }
}

}  // namespace oc::ui::lvgl
