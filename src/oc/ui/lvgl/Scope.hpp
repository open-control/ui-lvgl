#pragma once

#include <lvgl.h>

#include <oc/core/input/Binding.hpp>

#include "IElement.hpp"

namespace oc::ui::lvgl {

/**
 * @brief Create IsActiveFn from LVGL object
 *
 * Returns a function that checks if the LVGL object exists and is not hidden.
 * Use this to create scoped bindings tied to view/component visibility.
 *
 * When the LVGL object is hidden (LV_OBJ_FLAG_HIDDEN), the binding
 * will be inactive and won't trigger callbacks.
 *
 * @param obj LVGL object to track (typically from IView::getElement())
 * @return IsActiveFn that returns true when obj is visible
 */
inline core::IsActiveFn isActive(lv_obj_t* obj) {
    return [obj]() {
        return obj && !lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN);
    };
}

/**
 * @brief Convert LVGL object pointer to ScopeID
 *
 * Creates a unique scope identifier from an LVGL object pointer.
 * Useful when you need to identify bindings by their scope.
 *
 * @param obj LVGL object
 * @return Unique ScopeID derived from pointer
 */
inline core::ScopeID scopeID(lv_obj_t* obj) {
    return reinterpret_cast<core::ScopeID>(obj);
}

/**
 * @brief Scope provider for LVGL-based UI elements
 *
 * Used with the fluent binding API to automatically set both
 * scope ID and visibility-based activation.
 *
 * @code
 * using oc::ui::lvgl::scope;
 *
 * // Bindings tied to dialog visibility
 * api.button(BTN_OK).onPress().scope(scope(dialog)).then([]{ confirm(); });
 * api.button(BTN_CANCEL).onPress().scope(scope(dialog)).then([]{ cancel(); });
 *
 * // Clear all bindings when dialog closes
 * api.clearScope(scopeID(dialog->getElement()));
 * @endcode
 */
class Scope {
public:
    /**
     * @brief Construct from raw LVGL object pointer
     */
    explicit Scope(lv_obj_t* element) : element_(element) {}

    /**
     * @brief Get the scope ID for binding registration
     *
     * Required by the fluent API's duck-typed scope() template.
     */
    core::ScopeID getScopeID() const {
        return scopeID(element_);
    }

    /**
     * @brief Get the activation predicate
     *
     * Optional method detected by the fluent API via SFINAE.
     * When present, bindings will only trigger when the element is visible.
     */
    core::IsActiveFn getIsActive() const {
        return isActive(element_);
    }

private:
    lv_obj_t* element_;
};

/**
 * @brief Create Scope from raw LVGL object
 *
 * Factory function for the fluent API.
 *
 * @param element LVGL object pointer
 * @return Scope for use with scope()
 */
inline Scope scope(lv_obj_t* element) {
    return Scope(element);
}

/**
 * @brief Create Scope from IElement
 *
 * Factory function for the fluent API.
 *
 * @param element Reference to UI element implementing IElement
 * @return Scope for use with scope()
 */
inline Scope scope(const IElement& element) {
    return Scope(element.getElement());
}

}  // namespace oc::ui::lvgl
