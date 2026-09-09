# Opt-in LVGL frame benchmark

`OC_ENABLE_LVGL_BENCHMARK=1` enables a bounded foreground-only collector,
independent of `OC_ENABLE_STATS` and the performance sample queue. Normal builds
default to zero: the collector implementation and Bridge calls are absent.
The collector does not allocate, log, change timer periods, alter damage, or
submit extra frames. It retains totals, the last frame, and the complete worst
frame; no ring can discard the worst frame when rendering stalls.

Configure the **same benchmark build's LVGL C sources** with:

```c
#define LV_USE_PROFILER 1
#define LV_USE_PROFILER_BUILTIN 0
#define LV_PROFILER_INCLUDE "oc/ui/lvgl/LvglProfilerHooks.h"
#define LV_PROFILER_LAYOUT 1
#define LV_PROFILER_STYLE 1
#define LV_PROFILER_REFR 1
#define LV_PROFILER_TIMER 1
#define LV_PROFILER_DRAW 0
#define LV_PROFILER_INDEV 0
#define LV_PROFILER_DECODER 0
#define LV_PROFILER_FONT 0
#define LV_PROFILER_CACHE 0
#define LV_PROFILER_FS 0
#define LV_PROFILER_EVENT 0
```

The C-compatible hook uses GCC/Clang constant-folded `__func__` comparisons.
Optimized nonselected sites contain no probe calls and no runtime string table
or function-name lookup. Enabling `REFR` selects `refr_area` even though the
much more frequent low-level `DRAW` profiler category is disabled.

```cpp
namespace bench = oc::ui::lvgl::benchmark;
if (!bench::beginRun(runId)) { /* clock missing or frame already active */ }
bench::setMarker(actionId);
// Execute the normal app loop and normal controller grammar.
bench::endRun();
const auto& captured = bench::snapshot();
// Serialize now, outside the measured run. Keep `errors` in the result.
```

The real display `Bridge` brackets `lv_timer_handler`; no Core endpoint needs
to call `beginFrame/endFrame`. A marker is captured at the start of each frame,
so a later marker cannot relabel a retained worst frame. Results remain stable
after `endRun`, until the next successful `beginRun`. Access and reconfiguration
are foreground-only; do not read the reference from an ISR. The SDL-specific
bridge is not an alternate source of device measurements.

| Phase index | Selected LVGL scope | Meaning |
| --- | --- | --- |
| 0 `TimerCallback` | `lv_timer_exec`, tag `timer_cb` | All executed timer callbacks, including display refresh |
| 1 `DisplayRefresh` | `lv_display_refr_timer` | Complete admitted display refresh |
| 2 `Layout` | `lv_obj_update_layout` | Calls that reach LVGL's layout-profiler scope |
| 3 `StyleRefresh` | `lv_obj_refresh_style` | Style refresh, not every raw style setter |
| 4 `DrawArea` | `refr_area` | Rendering an invalidated display area |
| 5 `Flush` | `call_flush_cb` | Synchronous display flush callback, not later DMA completion |

`Frame` carries `runId`, `marker`, `sequence`, `startedAtUs`, `handlerUs`,
`invalidatedPixels`, `submittedPixels`, and six `phase` metrics. `Snapshot`
carries `active`, `frames`, `totalHandlerUs`, `errors`, `last`, `worst`, and six
`totals`. A phase metric has `calls`, `totalUs`, `maxUs`. Calls count nested
entries, but timing unions nesting of the **same** phase; its maximum is the
largest outer span. Different phases are **inclusive and overlap**: adding
timer, refresh, layout, draw and flush would double-count.

Durations use unsigned 32-bit microsecond subtraction, including wraparound;
a single measured frame/phase must last less than one 32-bit clock period
(about 71 minutes). Totals use 64 bits; counters saturate and set an error
instead of wrapping. Clock reads and IRQ/OS preemption contribute to measured
wall time. No profiler overhead correction is claimed. Snapshot copying and
aggregate merging happen after the recorded handler end timestamp.

Pixel counters reuse Bridge's existing area sums: overlapping invalidations
are not deduplicated, and submitted pixels are not necessarily distinct.
They are not a DMA-completion timestamp or a guarantee of bytes transferred.
Any nonzero `errors` makes the affected run suspect: unmatched/incomplete
phase, reentrant frame, counter overflow, or invalid phase. A rejected
reentrant frame does not close its caller's frame.

## Checks

Configure `test/` with the actual LVGL checkout and build twice, setting
`OC_TEST_LVGL_BENCHMARK` to `ON` and `OFF`. Run CTest in each build directory.
`bridge_admission` exercises real LVGL/display admission and, when enabled,
all selected phase families and submitted-pixel accounting.
`lvgl_frame_profiler` checks deterministic timings, wraparound, nesting,
marker/worst correlation, reset/freeze, disabled-run behavior, compile-time
selection, and error reporting. The standalone unit target needs no LVGL.

On this checkout, GNU 15.2 Release builds pass both tests with profiling on and
off. Symbol inspection of optimized LVGL style/refresh objects finds only the
selected C hook calls and no `strcmp`; the nonselected object-event unit has
no hook references. Hardware timings still require running the hardware bench.
