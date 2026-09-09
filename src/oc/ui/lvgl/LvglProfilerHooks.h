#ifndef OC_UI_LVGL_PROFILER_HOOKS_H
#define OC_UI_LVGL_PROFILER_HOOKS_H

/* C-compatible LV_PROFILER_INCLUDE. Enable only in the opt-in benchmark build.
 * GCC/Clang fold __builtin_strcmp(__func__, literal) at each call site, leaving
 * calls only in these selected functions; there is no runtime label lookup. */
#if OC_ENABLE_LVGL_BENCHMARK
#ifdef __cplusplus
extern "C" {
#endif
void oc_lvgl_profile_begin(unsigned int phase);
void oc_lvgl_profile_end(unsigned int phase);
#ifdef __cplusplus
}
#endif

#undef LV_PROFILER_BEGIN
#undef LV_PROFILER_END
#undef LV_PROFILER_BEGIN_TAG
#undef LV_PROFILER_END_TAG
#define OC_LVGL_SELECTED_PHASE \
    (__builtin_strcmp(__func__, "lv_display_refr_timer") == 0 ? 1U : \
     __builtin_strcmp(__func__, "lv_obj_update_layout") == 0 ? 2U : \
     __builtin_strcmp(__func__, "lv_obj_refresh_style") == 0 ? 3U : \
     __builtin_strcmp(__func__, "refr_area") == 0 ? 4U : \
     __builtin_strcmp(__func__, "call_flush_cb") == 0 ? 5U : 6U)
#define LV_PROFILER_BEGIN do { \
    if (OC_LVGL_SELECTED_PHASE < 6U) oc_lvgl_profile_begin(OC_LVGL_SELECTED_PHASE); \
} while (0)
#define LV_PROFILER_END do { \
    if (OC_LVGL_SELECTED_PHASE < 6U) oc_lvgl_profile_end(OC_LVGL_SELECTED_PHASE); \
} while (0)
#define LV_PROFILER_BEGIN_TAG(tag) do { \
    if (__builtin_strcmp(__func__, "lv_timer_exec") == 0 && \
        __builtin_strcmp((tag), "timer_cb") == 0) oc_lvgl_profile_begin(0U); \
} while (0)
#define LV_PROFILER_END_TAG(tag) do { \
    if (__builtin_strcmp(__func__, "lv_timer_exec") == 0 && \
        __builtin_strcmp((tag), "timer_cb") == 0) oc_lvgl_profile_end(0U); \
} while (0)
#endif
#endif
