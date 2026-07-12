#include "PausableTimer.hpp"

namespace oc::ui::lvgl {

PausableTimer::PausableTimer(uint32_t periodMs, lv_timer_cb_t callback, void* userData)
    : timer_(lv_timer_create(callback, periodMs, userData)) {
    pause();
}

PausableTimer::~PausableTimer() {
    if (!timer_) return;
    lv_timer_delete(timer_);
    timer_ = nullptr;
}

void PausableTimer::pause() {
    if (timer_) lv_timer_pause(timer_);
}

void PausableTimer::resume(bool ready) {
    if (!timer_) return;

    lv_timer_resume(timer_);
    if (ready) lv_timer_ready(timer_);
}

}  // namespace oc::ui::lvgl
