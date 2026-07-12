#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

#include <lvgl.h>

namespace oc::ui::lvgl {

/**
 * Invalidates an exact area of a static, effect-free object.
 *
 * The object must not draw outside its coordinates through shadows, blur, or
 * overflow effects. Use normal LVGL invalidation when that contract is false.
 */
void invalidateStaticSurfaceArea(lv_obj_t* clipObject,
                                 const lv_area_t& requested);

/**
 * Batches mutations for a static, effect-free LVGL surface.
 *
 * While this scope owns the display invalidation pause, all invalidations on
 * that display are suppressed. Keep scopes synchronous and narrow, include
 * every affected region, and never call user callbacks from inside a batch.
 * Include regions before and after mutations that can change geometry.
 */
template <std::size_t MaxRegions = 16>
class StaticSurfaceInvalidationBatch {
    static_assert(MaxRegions > 0, "StaticSurfaceInvalidationBatch requires storage");

public:
    explicit StaticSurfaceInvalidationBatch(lv_obj_t* clipObject, bool enabled = true)
        : clip_object_(clipObject)
        , display_(enabled && clipObject ? lv_obj_get_display(clipObject) : nullptr) {
        if (display_ && lv_display_is_invalidation_enabled(display_)) {
            lv_display_enable_invalidation(display_, false);
            owns_pause_ = true;
        }
    }

    ~StaticSurfaceInvalidationBatch() {
        flush();
    }

    StaticSurfaceInvalidationBatch(const StaticSurfaceInvalidationBatch&) = delete;
    StaticSurfaceInvalidationBatch& operator=(const StaticSurfaceInvalidationBatch&) = delete;

    void include(lv_obj_t* object) {
        if (!owns_pause_ || !object) return;
        lv_area_t area{};
        lv_obj_get_coords(object, &area);
        include(area);
    }

    void include(const lv_area_t& area) {
        if (!owns_pause_) return;

        if (collapsed_) {
            join(regions_[0], area);
            return;
        }
        if (region_count_ < regions_.size()) {
            regions_[region_count_++] = area;
            return;
        }

        for (std::size_t i = 1; i < region_count_; ++i) {
            join(regions_[0], regions_[i]);
        }
        join(regions_[0], area);
        region_count_ = 1;
        collapsed_ = true;
    }

    void flush() {
        if (!owns_pause_) return;

        lv_display_enable_invalidation(display_, true);
        owns_pause_ = false;
        for (std::size_t i = 0; i < region_count_; ++i) {
            invalidateStaticSurfaceArea(clip_object_, regions_[i]);
        }
        region_count_ = 0;
        collapsed_ = false;
    }

private:
    static void join(lv_area_t& target, const lv_area_t& next) {
        target.x1 = std::min(target.x1, next.x1);
        target.y1 = std::min(target.y1, next.y1);
        target.x2 = std::max(target.x2, next.x2);
        target.y2 = std::max(target.y2, next.y2);
    }

    lv_obj_t* clip_object_ = nullptr;
    lv_display_t* display_ = nullptr;
    std::array<lv_area_t, MaxRegions> regions_{};
    std::size_t region_count_ = 0;
    bool owns_pause_ = false;
    bool collapsed_ = false;
};

}  // namespace oc::ui::lvgl
