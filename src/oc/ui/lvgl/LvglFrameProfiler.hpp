#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#ifndef OC_ENABLE_LVGL_BENCHMARK
#define OC_ENABLE_LVGL_BENCHMARK 0
#endif

#if OC_ENABLE_LVGL_BENCHMARK
namespace oc::ui::lvgl::benchmark {

enum class Phase : uint8_t {
    TimerCallback, DisplayRefresh, Layout, StyleRefresh, DrawArea, Flush, Count
};
inline constexpr size_t PHASE_COUNT = static_cast<size_t>(Phase::Count);

struct PhaseMetrics {
    uint32_t calls = 0;
    // Nested calls of the SAME phase are counted but timed as one outer span.
    // Different phases overlap: their inclusive times must not be summed.
    uint64_t totalUs = 0;
    uint32_t maxUs = 0;
};

struct Frame {
    uint32_t runId = 0;
    uint32_t marker = 0;
    uint32_t sequence = 0;
    uint32_t startedAtUs = 0;
    uint32_t handlerUs = 0;
    uint32_t invalidatedPixels = 0;
    uint32_t submittedPixels = 0;
    std::array<PhaseMetrics, PHASE_COUNT> phase{};
};

enum Error : uint32_t {
    UnbalancedScope = 1U << 0U,
    ReentrantFrame = 1U << 1U,
    CounterOverflow = 1U << 2U,
    InvalidPhase = 1U << 3U,
};

struct Snapshot {
    uint32_t runId = 0;
    uint32_t marker = 0;
    bool active = false;
    uint32_t frames = 0;
    uint64_t totalHandlerUs = 0;
    uint32_t errors = 0;
    Frame last{};
    Frame worst{};
    std::array<PhaseMetrics, PHASE_COUNT> totals{};
};

// Foreground LVGL thread only; no ISR access, allocation, logging or sample queue.
// Reconfiguration is rejected inside a frame or without a microsecond clock.
bool beginRun(uint32_t runId);
void setMarker(uint32_t marker);
void endRun();
// Read after endRun for a stable result; valid until the next beginRun.
const Snapshot& snapshot();

// Bridge-owned boundaries. Frame/marker identity is frozen at beginFrame.
bool beginFrame();
void endFrame(uint32_t invalidatedPixels, uint32_t submittedPixels);

}  // namespace oc::ui::lvgl::benchmark
#endif
