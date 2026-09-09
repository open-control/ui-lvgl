#ifndef LV_CONF_H
#define LV_CONF_H

// Headless admission test: real LVGL timers and RGB565 software rendering.
#define LV_COLOR_DEPTH 16
#define LV_MEM_SIZE (256 * 1024U)
#define LV_USE_LOG 0

#if OC_ENABLE_LVGL_BENCHMARK
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
#endif

#endif
