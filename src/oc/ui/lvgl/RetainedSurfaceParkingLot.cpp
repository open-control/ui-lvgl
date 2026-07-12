#include "RetainedSurfaceParkingLot.hpp"

namespace oc::ui::lvgl {

bool RetainedSurfaceParkingLot::initialize() {
    if (screen_) return true;

    screen_ = lv_obj_create(nullptr);
    if (screen_) lv_obj_remove_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
    return screen_ != nullptr;
}

RetainedSurfaceParkingLot::~RetainedSurfaceParkingLot() {
    if (!screen_) return;
    lv_obj_delete(screen_);
    screen_ = nullptr;
}

lv_obj_t* RetainedSurfaceParkingLot::createHost() const {
    if (!screen_) return nullptr;

    lv_obj_t* host = lv_obj_create(screen_);
    if (!host) return nullptr;
    lv_obj_remove_style_all(host);
    lv_obj_set_size(host, 1, 1);
    lv_obj_add_flag(host, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_remove_flag(host, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(host, LV_OBJ_FLAG_SCROLLABLE);
    return host;
}

void RetainedSurfaceParkingLot::attach(lv_obj_t* root, lv_obj_t* parent) {
    if (!root || !parent || lv_obj_get_parent(root) == parent) return;
    lv_obj_set_parent(root, parent);
}

void RetainedSurfaceParkingLot::park(lv_obj_t* root, lv_obj_t* host) {
    if (!root || !host || lv_obj_get_parent(root) == host) return;

    mirrorViewport(host, lv_obj_get_parent(root));
    attach(root, host);
}

void RetainedSurfaceParkingLot::mirrorViewport(lv_obj_t* host, lv_obj_t* sourceParent) {
    if (!host || !sourceParent) return;

    lv_area_t sourceArea{};
    lv_obj_get_coords(sourceParent, &sourceArea);
    const lv_coord_t width = lv_obj_get_width(sourceParent);
    const lv_coord_t height = lv_obj_get_height(sourceParent);
    if (width <= 0 || height <= 0) return;

    lv_obj_set_pos(host, sourceArea.x1, sourceArea.y1);
    lv_obj_set_size(host, width, height);
}

}  // namespace oc::ui::lvgl
