#include <cassert>
#include <cstdint>

#include <oc/time/Time.hpp>
#include <oc/ui/lvgl/LvglFrameProfiler.hpp>
#include <oc/ui/lvgl/LvglProfilerHooks.h>

namespace bench = oc::ui::lvgl::benchmark;
namespace {
uint32_t nowUs = 0;
uint32_t clockReads = 0;
uint32_t clock() { ++clockReads; return nowUs; }
constexpr unsigned layout = static_cast<unsigned>(bench::Phase::Layout);
constexpr unsigned style = static_cast<unsigned>(bench::Phase::StyleRefresh);
void lv_obj_update_layout() { LV_PROFILER_BEGIN; nowUs += 7U; LV_PROFILER_END; }
void ignored_function() { LV_PROFILER_BEGIN; nowUs += 3U; LV_PROFILER_END; }
void lv_timer_exec() { LV_PROFILER_BEGIN_TAG("timer_cb"); nowUs += 5U; LV_PROFILER_END_TAG("timer_cb"); }
}

int main() {
    oc::time::setMicrosProvider(clock);
    assert(!bench::beginFrame());
    oc_lvgl_profile_begin(layout); oc_lvgl_profile_end(layout);
    assert(clockReads == 0U); // Inactive run never reads the clock.
    assert(bench::beginRun(17U));
    bench::setMarker(101U);
    nowUs = UINT32_MAX - 10U;
    assert(bench::beginFrame());
    assert(!bench::beginRun(99U)); // Cannot reset a live frame.
    oc_lvgl_profile_begin(layout);
    nowUs += 4U;
    oc_lvgl_profile_begin(layout);
    nowUs += 3U;
    oc_lvgl_profile_end(layout);
    nowUs += 5U;
    oc_lvgl_profile_end(layout);
    bench::setMarker(102U); // Changes next frame, not current/worst identity.
    oc_lvgl_profile_begin(style);
    nowUs += 8U;
    oc_lvgl_profile_end(style);
    nowUs += 2U;
    bench::endFrame(123U, 456U);
    const auto& first = bench::snapshot();
    assert(first.frames == 1U && first.errors == 0U);
    assert(first.worst.runId == 17U && first.worst.marker == 101U);
    assert(first.worst.handlerUs == 22U && first.worst.sequence == 1U);
    assert(first.worst.invalidatedPixels == 123U && first.worst.submittedPixels == 456U);
    assert(first.worst.phase[layout].calls == 2U);
    assert(first.worst.phase[layout].totalUs == 12U && first.worst.phase[layout].maxUs == 12U);
    assert(first.worst.phase[style].calls == 1U && first.worst.phase[style].totalUs == 8U);
    assert(bench::beginFrame());
    lv_obj_update_layout();
    const auto reads = clockReads;
    ignored_function();
    assert(clockReads == reads); // Nonselected function has no hook/clock calls.
    lv_timer_exec();
    bench::endFrame(1U, 2U);
    bench::endRun();
    const auto& completed = bench::snapshot();
    assert(!completed.active && completed.frames == 2U && completed.totalHandlerUs == 37U);
    assert(completed.last.marker == 102U && completed.last.handlerUs == 15U);
    assert(completed.worst.marker == 101U && completed.worst.handlerUs == 22U);
    assert(completed.totals[layout].calls == 3U && completed.totals[layout].totalUs == 19U);
    assert(completed.last.phase[0].calls == 1U && completed.last.phase[0].totalUs == 5U);
    const auto stoppedReads = clockReads;
    lv_obj_update_layout();
    bench::setMarker(999U);
    assert(clockReads == stoppedReads && bench::snapshot().frames == 2U);
    assert(bench::snapshot().marker == 102U);

    assert(bench::beginRun(18U));
    assert(bench::snapshot().frames == 0U && bench::snapshot().totals[layout].calls == 0U);
    assert(bench::beginFrame());
    assert(!bench::beginFrame());
    oc_lvgl_profile_end(layout); // Unmatched scopes invalidate the report.
    oc_lvgl_profile_begin(99U);
    oc_lvgl_profile_begin(style); // Still open at endFrame.
    nowUs += 1U;
    bench::endFrame(0U, 0U);
    assert(bench::snapshot().errors == (bench::ReentrantFrame | bench::UnbalancedScope | bench::InvalidPhase));
    bench::endRun();
}
