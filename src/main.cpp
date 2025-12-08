/**
 * @file main.cpp
 * @brief Compilation test for ui-lvgl
 */
#include <Arduino.h>

#include <oc/ui/lvgl/Bridge.hpp>

// Verify types compile correctly
static_assert(sizeof(oc::ui::lvgl::BridgeConfig) > 0, "BridgeConfig");

void setup() {}
void loop() {}
