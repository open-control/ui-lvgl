#include "LvglFrameProfiler.hpp"

#if OC_ENABLE_LVGL_BENCHMARK
#include <algorithm>

#include <oc/time/Time.hpp>

namespace oc::ui::lvgl::benchmark {
namespace {
Snapshot result;
Frame current;
bool frameActive = false;
std::array<uint32_t, PHASE_COUNT> phaseDepth{};
std::array<uint32_t, PHASE_COUNT> phaseStartedAtUs{};

void addCalls(uint32_t& value, uint32_t increment) {
    if (increment > UINT32_MAX - value) {
        value = UINT32_MAX;
        result.errors |= CounterOverflow;
    } else {
        value += increment;
    }
}

void addTime(uint64_t& value, uint64_t increment) {
    if (increment > UINT64_MAX - value) {
        value = UINT64_MAX;
        result.errors |= CounterOverflow;
    } else {
        value += increment;
    }
}
}  // namespace

bool beginRun(uint32_t runId) {
    if (frameActive || !oc::time::isMicrosConfigured()) return false;
    result = {};
    current = {};
    phaseDepth = {};
    result.runId = runId;
    result.active = true;
    return true;
}

void setMarker(uint32_t marker) {
    if (result.active) result.marker = marker;
}

void endRun() {
    if (frameActive) result.errors |= UnbalancedScope;
    frameActive = false;
    phaseDepth = {};
    result.active = false;
}

const Snapshot& snapshot() { return result; }

bool beginFrame() {
    if (!result.active) return false;
    if (frameActive) {
        result.errors |= ReentrantFrame;
        return false;
    }
    current = {};
    current.runId = result.runId;
    current.marker = result.marker;
    current.sequence = result.frames;
    addCalls(current.sequence, 1U);
    phaseDepth = {};
    frameActive = true;
    current.startedAtUs = oc::time::micros32();
    return true;
}

void endFrame(uint32_t invalidatedPixels, uint32_t submittedPixels) {
    if (!result.active) return;
    if (!frameActive) {
        result.errors |= UnbalancedScope;
        return;
    }
    current.handlerUs = oc::time::micros32() - current.startedAtUs;
    frameActive = false;
    for (auto depth : phaseDepth) {
        if (depth != 0U) result.errors |= UnbalancedScope;
    }
    current.invalidatedPixels = invalidatedPixels;
    current.submittedPixels = submittedPixels;
    addCalls(result.frames, 1U);
    addTime(result.totalHandlerUs, current.handlerUs);
    result.last = current;
    if (result.frames == 1U || current.handlerUs > result.worst.handlerUs)
        result.worst = current;
    for (size_t index = 0; index < PHASE_COUNT; ++index) {
        auto& total = result.totals[index];
        const auto& sample = current.phase[index];
        addCalls(total.calls, sample.calls);
        addTime(total.totalUs, sample.totalUs);
        total.maxUs = std::max(total.maxUs, sample.maxUs);
    }
}

void phaseBegin(unsigned int phase) {
    if (!frameActive) return;
    if (phase >= PHASE_COUNT) { result.errors |= InvalidPhase; return; }
    addCalls(current.phase[phase].calls, 1U);
    if (phaseDepth[phase] == 0U)
        phaseStartedAtUs[phase] = oc::time::micros32();
    addCalls(phaseDepth[phase], 1U);
}

void phaseEnd(unsigned int phase) {
    if (!frameActive) return;
    if (phase >= PHASE_COUNT) { result.errors |= InvalidPhase; return; }
    if (phaseDepth[phase] == 0U) {
        result.errors |= UnbalancedScope;
        return;
    }
    if (--phaseDepth[phase] != 0U) return;
    const uint32_t elapsedUs = oc::time::micros32() - phaseStartedAtUs[phase];
    auto& metric = current.phase[phase];
    addTime(metric.totalUs, elapsedUs);
    metric.maxUs = std::max(metric.maxUs, elapsedUs);
}
}  // namespace oc::ui::lvgl::benchmark

extern "C" void oc_lvgl_profile_begin(unsigned int phase) {
    oc::ui::lvgl::benchmark::phaseBegin(phase);
}
extern "C" void oc_lvgl_profile_end(unsigned int phase) {
    oc::ui::lvgl::benchmark::phaseEnd(phase);
}
#endif
