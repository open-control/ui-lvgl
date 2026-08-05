cmake_minimum_required(VERSION 3.29)

project(open_control_ui_lvgl VERSION 0.2.0 LANGUAGES CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/OpenControlUiLvglSources.cmake")

# Consumers provide LVGL and the selected display backend. An INTERFACE source
# target preserves those choices while publishing one exact source authority.
add_library(oc_ui_lvgl INTERFACE)
target_sources(oc_ui_lvgl INTERFACE ${OC_UI_LVGL_SOURCES})
target_include_directories(oc_ui_lvgl
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(oc_ui_lvgl INTERFACE cxx_std_17)
