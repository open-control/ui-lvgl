# Canonical library source inventory for OpenControl UI-LVGL consumers.
# The standalone src/main.cpp entry point is intentionally not a library source.
set(OC_UI_LVGL_SOURCE_PATHS
    src/oc/ui/lvgl/Bridge.cpp
    src/oc/ui/lvgl/FontLoader.cpp
    src/oc/ui/lvgl/FontUtils.cpp
    src/oc/ui/lvgl/PausableTimer.cpp
    src/oc/ui/lvgl/RetainedSurfaceParkingLot.cpp
    src/oc/ui/lvgl/Screen.cpp
    src/oc/ui/lvgl/SdlBridge.cpp
    src/oc/ui/lvgl/StaticSurfaceInvalidation.cpp
)

set(OC_UI_LVGL_SOURCES ${OC_UI_LVGL_SOURCE_PATHS})
list(TRANSFORM OC_UI_LVGL_SOURCES
    PREPEND "${CMAKE_CURRENT_LIST_DIR}/../")
