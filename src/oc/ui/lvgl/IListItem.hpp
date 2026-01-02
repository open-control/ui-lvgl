#pragma once

#include "IComponent.hpp"

namespace oc::ui::lvgl {

/**
 * @brief Interface for list item components with highlight support
 *
 * Represents reusable items displayed in virtual lists (VirtualList).
 * Extends IComponent with highlight functionality for selection state.
 *
 * List items are pooled and recycled by VirtualList, so they must:
 * - Support show/hide for visibility toggling between item types
 * - Support setHighlighted for selection feedback
 * - Be stateless (data comes from render props)
 *
 * Examples:
 * - Track title items in track selector
 * - Device title items in device selector
 * - Back navigation buttons
 */
class IListItem : public IComponent {
public:
    /**
     * @brief Set the highlight state of the item
     * @param highlighted true if item is selected/focused
     *
     * Implementation typically updates visual appearance:
     * - Background color/opacity
     * - Text color
     * - Border style
     */
    virtual void setHighlighted(bool highlighted) = 0;
};

}  // namespace oc::ui::lvgl
