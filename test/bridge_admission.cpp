#include <array>
#include <cassert>
#include <oc/ui/lvgl/Bridge.hpp>

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
}

int main() {
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
}
