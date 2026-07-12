#pragma once

#include <cstdint>

#include <lvgl.h>

namespace oc::ui::lvgl {

/** RAII wrapper for an LVGL timer that starts paused. */
class PausableTimer {
public:
    PausableTimer(uint32_t periodMs, lv_timer_cb_t callback, void* userData);
    ~PausableTimer();

    PausableTimer(const PausableTimer&) = delete;
    PausableTimer& operator=(const PausableTimer&) = delete;
    PausableTimer(PausableTimer&&) = delete;
    PausableTimer& operator=(PausableTimer&&) = delete;

    void pause();
    void resume(bool ready = false);
    [[nodiscard]] bool valid() const { return timer_ != nullptr; }

private:
    lv_timer_t* timer_ = nullptr;
};

}  // namespace oc::ui::lvgl
