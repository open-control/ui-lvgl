#include "StaticSurfaceInvalidation.hpp"

// LVGL 9 keeps direct display invalidation private. This translation unit is
// the only package boundary allowed to depend on that internal API.
#include <src/core/lv_refr_private.h>

static_assert(LVGL_VERSION_MAJOR == 9,
              "Review direct invalidation for this LVGL major");

namespace oc::ui::lvgl {

void updateLayoutWithFullRedraw(lv_obj_t* object) {
    if (!object) return;
    auto* display = lv_obj_get_display(object);
    const bool ownsPause = lv_display_is_invalidation_enabled(display);
    if (ownsPause) lv_display_enable_invalidation(display, false);
    lv_obj_update_layout(object);
    if (ownsPause) {
        lv_display_enable_invalidation(display, true);
        // Normal LVGL invalidation retains effect/layer handling once per layout.
        lv_obj_invalidate(lv_display_get_screen_active(display));
    }
}

void invalidateStaticSurfaceArea(lv_obj_t* clipObject,
                                 const lv_area_t& requested) {
    if (!clipObject) return;

    lv_display_t* display = lv_obj_get_display(clipObject);
    if (!display || !lv_display_is_invalidation_enabled(display)) return;

    lv_area_t visible = requested;
    if (!lv_obj_area_is_visible(clipObject, &visible)) return;
    (void)lv_inv_area(display, &visible);
}

}  // namespace oc::ui::lvgl
