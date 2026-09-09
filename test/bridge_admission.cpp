#include <array>
#include <cassert>
#include <oc/ui/lvgl/Bridge.hpp>
#include <oc/ui/lvgl/StaticSurfaceInvalidation.hpp>
#if OC_ENABLE_LVGL_BENCHMARK
#include <oc/time/Time.hpp>
#endif

namespace {
uint32_t nowMs = 0;
uint32_t timerTicks = 0;
std::array<uint16_t, 320 * 240> pixels{};

struct Display : oc::interface::IDisplay {
    bool ready = false;
    unsigned regions = 0;
    unsigned frames = 0;
    uint16_t sampledPixel = 0;
    oc::type::Result<void> init() override { return oc::type::Result<void>::ok(); }
    uint16_t width() const override { return 320; }
    uint16_t height() const override { return 240; }
    bool canAcceptFrame() const override { return ready; }
    void flush(const void*, const oc::interface::Rect&) override { assert(false); }
    void flushRegion(const void* data, const oc::interface::Rect&, uint16_t stride, bool last) override {
        assert(ready);
        assert(data == pixels.data() && stride == width());
        ++regions;
        if (last) {
            ++frames;
            sampledPixel = static_cast<const uint16_t*>(data)[20 * stride + 20];
            ready = false; // Simulate DMA ownership until explicitly completed.
        }
    }
};

void checkStaticSurfaceBatch(oc::ui::lvgl::Bridge& bridge, Display& driver) {
    using oc::ui::lvgl::StaticSurfaceInvalidationBatch;
    auto* display = bridge.getDisplay();
    auto* surface = lv_obj_create(lv_display_get_screen_active(display));
    lv_obj_remove_style_all(surface);
    lv_obj_set_pos(surface, 90, 10);
    lv_obj_set_size(surface, 180, 120);
    lv_obj_set_style_bg_opa(surface, LV_OPA_COVER, 0);
    auto* label = lv_label_create(surface);
    lv_obj_set_size(label, 60, 20);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    auto* card = lv_obj_create(surface);
    lv_obj_remove_style_all(card);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);

    const auto present = [&] {
        driver.ready = true;
        nowMs += 10;
        bridge.refresh();
    };
    const auto populate = [&](bool edited) {
        lv_obj_set_style_bg_color(surface, lv_color_hex(edited ? 0x181818 : 0x080808), 0);
        lv_label_set_text_static(label, edited ? "Changed label" : "Initial");
        lv_obj_set_style_text_color(label, lv_color_hex(edited ? 0x00ffff : 0xffffff), 0);
        lv_obj_set_pos(label, edited ? 135 : 8, 5); // Exercise parent clipping.
        lv_obj_set_pos(card, edited ? 45 : 8, 35);
        lv_obj_set_size(card, edited ? 90 : 30, edited ? 50 : 20);
        lv_obj_set_style_bg_color(card, lv_color_hex(edited ? 0xff0000 : 0x0000ff), 0);
        lv_obj_update_layout(surface);
    };
    populate(false);
    present();
    const auto initialPixels = pixels;

    unsigned invalidations = 0;
    const auto countInvalidation = +[](lv_event_t* event) {
        ++*static_cast<unsigned*>(lv_event_get_user_data(event));
    };
    lv_display_add_event_cb(display, countInvalidation, LV_EVENT_INVALIDATE_AREA, &invalidations);
    populate(true);
    assert(invalidations > 1U);
    present();
    const auto expectedPixels = pixels;
    assert(expectedPixels != initialPixels);
    populate(false);
    present();
    assert(pixels == initialPixels);

    invalidations = 0;
    {
        StaticSurfaceInvalidationBatch<1> batch(surface);
        batch.include(surface); // All child geometry remains within this fixed surface.
        assert(!lv_display_is_invalidation_enabled(display));
        {
            StaticSurfaceInvalidationBatch<1> nested(card);
            nested.include(card);
            populate(true);
        }
        assert(!lv_display_is_invalidation_enabled(display));
        assert(invalidations == 0U);
        batch.flush();
        assert(lv_display_is_invalidation_enabled(display));
        assert(invalidations == 1U);
    }
    assert(invalidations == 1U); // Explicit flush + destruction is idempotent.
    present();
    assert(pixels == expectedPixels); // Same complete RGB565 image as ordinary LVGL mutations.

    invalidations = 0;
    lv_display_enable_invalidation(display, false);
    {
        StaticSurfaceInvalidationBatch<1> batch(surface);
        batch.include(surface);
        populate(false);
    }
    assert(!lv_display_is_invalidation_enabled(display) && invalidations == 0U);
    lv_display_enable_invalidation(display, true);
    lv_obj_invalidate(surface);
    present();
    assert(pixels == initialPixels);
    lv_display_remove_event_cb_with_user_data(display, countInvalidation, &invalidations);
    lv_obj_delete(surface);
}

void checkAdjacentDamage(oc::ui::lvgl::Bridge& bridge, Display& driver) {
    auto* display = bridge.getDisplay();
    auto* screen = lv_display_get_screen_active(display);
    const auto present = [&] {
        driver.ready = true;
        nowMs += 10;
        bridge.refresh();
    };
    present();
    struct Damage {
        unsigned count = 0;
        unsigned pixels = 0;
    } damage;
    const auto capture = +[](lv_event_t* event) {
        auto& damage = *static_cast<Damage*>(lv_event_get_user_data(event));
        const auto& area = *static_cast<lv_area_t*>(lv_event_get_param(event));
        ++damage.count;
        damage.pixels += lv_area_get_size(&area);
    };
    lv_display_add_event_cb(display, capture, LV_EVENT_INVALIDATE_AREA, &damage);
    for (bool horizontal : {false, true}) {
        damage = {};
        {
            oc::ui::lvgl::StaticSurfaceInvalidationBatch<8> batch(screen);
            for (int i = 0; i < 8; ++i) {
                batch.include(horizontal ? lv_area_t{20 + i * 10, 20, 29 + i * 10, 22}
                                         : lv_area_t{20, 20 + i * 10, 22, 29 + i * 10});
            }
        }
        assert(damage.count == 1U && damage.pixels == 240U);
        present();
    }
    damage = {};
    {
        oc::ui::lvgl::StaticSurfaceInvalidationBatch<8> batch(screen);
        batch.include(lv_area_t{20, 20, 22, 29});
        batch.include(lv_area_t{20, 31, 22, 40}); // Gap must stay untouched.
        batch.include(lv_area_t{21, 41, 23, 50}); // Different playhead position.
        batch.include(lv_area_t{21, 51, 24, 60}); // Different damage width.
    }
    assert(damage.count == 4U && damage.pixels == 130U);
    present();
    lv_display_remove_event_cb_with_user_data(display, capture, &damage);
}

void checkFullLayoutRedraw(oc::ui::lvgl::Bridge& bridge, Display& driver) {
    using oc::ui::lvgl::updateLayoutWithFullRedraw;
    auto* display = bridge.getDisplay();
    auto* surface = lv_obj_create(lv_display_get_screen_active(display));
    lv_obj_remove_style_all(surface);
    lv_obj_set_size(surface, 100, 100);
    auto* child = lv_obj_create(surface);
    lv_obj_set_size(child, LV_PCT(100), 30);
    // A layout callback also moves another layer, outside the requested object.
    auto* sibling = lv_obj_create(lv_display_get_layer_top(display));
    lv_obj_set_size(sibling, 25, 25);
    lv_obj_add_event_cb(child, [](lv_event_t* event) {
        lv_obj_set_pos(static_cast<lv_obj_t*>(lv_event_get_user_data(event)),
                       lv_obj_get_width(lv_event_get_target_obj(event)), 150);
    }, LV_EVENT_SIZE_CHANGED, sibling);
    const auto present = [&] {
        driver.ready = true;
        nowMs += 10;
        bridge.refresh();
    };
    lv_obj_update_layout(surface);
    present();
    const auto initial = pixels;
    lv_obj_set_width(surface, 180);
    lv_obj_update_layout(surface);
    present();
    const auto expected = pixels;
    assert(initial != expected);
    lv_obj_set_width(surface, 100);
    lv_obj_update_layout(surface);
    present();
    assert(initial == pixels);

    lv_obj_set_width(surface, 180);
    updateLayoutWithFullRedraw(surface);
    assert(lv_display_is_invalidation_enabled(display));
    present();
    assert(lv_obj_get_x(sibling) == 180);
    assert(pixels == expected);

    lv_display_enable_invalidation(display, false);
    lv_obj_set_width(surface, 100);
    updateLayoutWithFullRedraw(surface);
    assert(!lv_display_is_invalidation_enabled(display));
    lv_display_enable_invalidation(display, true);
    lv_obj_invalidate(lv_display_get_screen_active(display));
    present();
    assert(lv_obj_get_x(sibling) == 100);
    assert(pixels == initial);
    updateLayoutWithFullRedraw(nullptr);
    // Opacity changes must reveal/recover the underlying pixels, not just paint
    // the new foreground. This is the fixed overlay-curtain use case.
    for (const lv_opa_t opacity : {LV_OPA_TRANSP, LV_OPA_90, LV_OPA_COVER, LV_OPA_TRANSP}) {
        lv_obj_set_style_bg_opa(surface, opacity, 0);
        present();
        const auto expectedPixels = pixels;
        lv_obj_set_style_bg_opa(surface, opacity == LV_OPA_COVER ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
        present();
        assert(pixels != expectedPixels);
        {
            oc::ui::lvgl::StaticSurfaceInvalidationBatch<1> batch(surface);
            batch.include(surface);
            lv_obj_set_style_bg_opa(surface, opacity, 0);
        }
        present();
        assert(pixels == expectedPixels);
    }
    lv_obj_delete(surface);
    lv_obj_delete(sibling);
}
}

int main() {
#if OC_ENABLE_LVGL_BENCHMARK
    namespace bench = oc::ui::lvgl::benchmark;
    oc::time::setMicrosProvider([] { return nowMs * 1000U; });
    assert(bench::beginRun(55U));
    bench::setMarker(8U);
#endif
    Display driver;
    oc::ui::lvgl::Bridge bridge(driver, pixels.data(), [] { return nowMs; }, {
        .renderMode = LV_DISPLAY_RENDER_MODE_DIRECT,
        .refreshHz = 120,
    });
    assert(bridge.init());
    auto* screen = lv_display_get_screen_active(bridge.getDisplay());
    auto* item = lv_obj_create(screen);
    lv_obj_set_pos(item, 0, 0);
    lv_obj_set_size(item, 80, 80);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(item, lv_color_hex(0xffffff), 0);
    auto* otherItem = lv_obj_create(screen);
    lv_obj_set_pos(otherItem, 200, 140);
    lv_obj_set_size(otherItem, 40, 40);
    auto* timer = lv_timer_create([](lv_timer_t* timer) {
        ++timerTicks;
        // Model an animation invalidating during lv_timer_handler itself.
        lv_obj_invalidate(static_cast<lv_obj_t*>(lv_timer_get_user_data(timer)));
#if OC_ENABLE_LVGL_BENCHMARK
        lv_obj_set_style_border_width(static_cast<lv_obj_t*>(lv_timer_get_user_data(timer)), timerTicks & 1U, 0);
#endif
    }, 1, item);

    const auto refresh = [&] { nowMs += 10; bridge.refresh(); };
    refresh();
    refresh();
    assert(timerTicks >= 2 && driver.frames == 0 && driver.regions == 0);
    driver.ready = true;
    refresh();
    assert(driver.frames == 1 && driver.sampledPixel == 0xffff);

    // Keep only the latest visual state while busy, but do not lose its damage.
    const auto previousRegions = driver.regions;
    lv_obj_set_style_bg_color(item, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_bg_color(otherItem, lv_color_hex(0x0000ff), 0);
    refresh();
    lv_obj_set_style_bg_color(item, lv_color_hex(0xff0000), 0);
    refresh();
    assert(driver.frames == 1 && timerTicks >= 5);
    driver.ready = true;
    refresh();
    assert(driver.frames == 2 && driver.sampledPixel == 0xf800);
    assert(driver.regions >= previousRegions + 2); // Multiple regions, one DMA launch.

    // An externally paused display timer must not be resumed by the bridge.
    auto* refreshTimer = lv_display_get_refr_timer(bridge.getDisplay());
    lv_timer_delete(timer);
    lv_obj_invalidate(item);
    lv_timer_pause(refreshTimer);
    driver.ready = true;
    refresh();
    assert(lv_timer_get_paused(refreshTimer) && driver.frames == 2);
    lv_timer_resume(refreshTimer);
    refresh();
    assert(driver.frames == 3);
#if OC_ENABLE_LVGL_BENCHMARK
    bench::endRun();
    const auto& profile = bench::snapshot();
    assert(profile.frames == 8U && profile.errors == 0U);
    assert(profile.last.runId == 55U && profile.last.marker == 8U);
    assert(profile.totals[static_cast<size_t>(bench::Phase::TimerCallback)].calls >= timerTicks);
    assert(profile.totals[static_cast<size_t>(bench::Phase::Layout)].calls > 0U);
    assert(profile.totals[static_cast<size_t>(bench::Phase::StyleRefresh)].calls >= timerTicks);
    assert(profile.totals[static_cast<size_t>(bench::Phase::DrawArea)].calls > 0U);
    assert(profile.totals[static_cast<size_t>(bench::Phase::Flush)].calls == driver.regions);
    assert(profile.last.submittedPixels > 0U);
#endif
    checkStaticSurfaceBatch(bridge, driver);
    checkAdjacentDamage(bridge, driver);
    checkFullLayoutRedraw(bridge, driver);
}
