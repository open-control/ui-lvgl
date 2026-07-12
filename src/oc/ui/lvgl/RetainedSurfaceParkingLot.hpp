#pragma once

#include <lvgl.h>

namespace oc::ui::lvgl {

/**
 * Owns an off-screen LVGL screen used to park retained surface trees.
 *
 * Each geometry family should use its own host. park() mirrors the source
 * viewport before reparenting so percentage-sized roots keep reusable layout.
 *
 * LVGL owns children through their parent. Destroy parked surface owners before
 * this parking lot, or attach those surfaces elsewhere before destruction.
 */
class RetainedSurfaceParkingLot {
public:
    RetainedSurfaceParkingLot() = default;
    ~RetainedSurfaceParkingLot();

    RetainedSurfaceParkingLot(const RetainedSurfaceParkingLot&) = delete;
    RetainedSurfaceParkingLot& operator=(const RetainedSurfaceParkingLot&) = delete;
    RetainedSurfaceParkingLot(RetainedSurfaceParkingLot&&) = delete;
    RetainedSurfaceParkingLot& operator=(RetainedSurfaceParkingLot&&) = delete;

    [[nodiscard]] bool initialize();
    [[nodiscard]] lv_obj_t* createHost() const;
    [[nodiscard]] bool valid() const { return screen_ != nullptr; }

    static void attach(lv_obj_t* root, lv_obj_t* parent);
    static void park(lv_obj_t* root, lv_obj_t* host);

private:
    static void mirrorViewport(lv_obj_t* host, lv_obj_t* sourceParent);

    lv_obj_t* screen_ = nullptr;
};

}  // namespace oc::ui::lvgl
