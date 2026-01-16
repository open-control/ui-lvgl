#include "Screen.hpp"

namespace oc::ui::lvgl {

namespace {
lv_obj_t* g_root = nullptr;
}

void Screen::setRoot(lv_obj_t* root) {
    g_root = root;
}

lv_obj_t* Screen::root() {
    return g_root ? g_root : lv_screen_active();
}

}  // namespace oc::ui::lvgl
