#include "FontLoader.hpp"
#include "FontUtils.hpp"

namespace oc::ui::lvgl::font {

void load(const Entry* entries, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const auto& e = entries[i];
        if (*e.target == nullptr) {
            *e.target = loadBinaryFont(e.data, e.size);
        }
    }
}

void loadEssential(const Entry* entries, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const auto& e = entries[i];
        if (e.essential && *e.target == nullptr) {
            *e.target = loadBinaryFont(e.data, e.size);
        }
    }
}

void unload(const Entry* entries, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const auto& e = entries[i];
        if (*e.target != nullptr) {
            freeFont(*e.target);
        }
    }
}

size_t countLoaded(const Entry* entries, size_t count) {
    size_t loaded = 0;
    for (size_t i = 0; i < count; ++i) {
        if (*entries[i].target != nullptr) {
            ++loaded;
        }
    }
    return loaded;
}

}  // namespace oc::ui::lvgl::font
