#pragma once

#include <lvgl.h>

#include <oc/core/struct/Binding.hpp>

namespace oc::ui {

// Forward declaration
class IElement;

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
inline core::IsActiveFn lvglIsActive(lv_obj_t* obj) {
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
inline core::ScopeID lvglScopeID(lv_obj_t* obj) {
    return reinterpret_cast<core::ScopeID>(obj);
}

// ═══════════════════════════════════════════════════
// Fluent API Scope Provider
// ═══════════════════════════════════════════════════

/**
 * @brief Scope provider for LVGL-based UI elements
 *
 * Used with the fluent binding API to automatically set both
 * scope ID and visibility-based activation.
 *
 * @code
 * using oc::ui::lvgl;
 *
 * // Bindings tied to dialog visibility
 * api.button(BTN_OK).onPress().scope(lvgl(dialog)).then([]{ confirm(); });
 * api.button(BTN_CANCEL).onPress().scope(lvgl(dialog)).then([]{ cancel(); });
 *
 * // Clear all bindings when dialog closes
 * api.clearScope(lvglScopeID(dialog->getElement()));
 * @endcode
 */
class LVGLScope {
public:
    /**
     * @brief Construct from raw LVGL object pointer
     */
    explicit LVGLScope(lv_obj_t* element) : element_(element) {}

    /**
     * @brief Get the scope ID for binding registration
     *
     * Required by the fluent API's duck-typed scope() template.
     */
    core::ScopeID getScopeID() const {
        return lvglScopeID(element_);
    }

    /**
     * @brief Get the activation predicate
     *
     * Optional method detected by the fluent API via SFINAE.
     * When present, bindings will only trigger when the element is visible.
     */
    core::IsActiveFn getIsActive() const {
        return lvglIsActive(element_);
    }

private:
    lv_obj_t* element_;
};

/**
 * @brief Create LVGLScope from raw LVGL object
 *
 * Factory function for the fluent API.
 *
 * @param element LVGL object pointer
 * @return LVGLScope for use with scope()
 */
inline LVGLScope lvgl(lv_obj_t* element) {
    return LVGLScope(element);
}

/**
 * @brief Create LVGLScope from IElement
 *
 * Factory function for the fluent API.
 *
 * @param element Reference to UI element implementing IElement
 * @return LVGLScope for use with scope()
 */
inline LVGLScope lvgl(const IElement& element);

}  // namespace oc::ui

// Include IElement to complete the inline implementation
#include <oc/ui/interface/IElement.hpp>

namespace oc::ui {

inline LVGLScope lvgl(const IElement& element) {
    return LVGLScope(element.getElement());
}

}  // namespace oc::ui
