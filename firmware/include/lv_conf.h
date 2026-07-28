#pragma once
#ifndef LV_CONF_H
#define LV_CONF_H
//
// lv_conf.h — minimal LVGL v8.4 config for Family Hub on ESP32-S3-Touch-LCD-7B.
// Enabled via build flag -DLV_CONF_INCLUDE_SIMPLE (see platformio.ini).
//
#define LV_CONF_INCLUDE_SIMPLE 1

#define LV_COLOR_DEPTH        16
#define LV_COLOR_16_SWAP      0      // RGB565 byte order for esp_lcd RGB panel

// Memory: let LVGL allocate from the standard heap; large pixel buffers live in
// PSRAM and are allocated explicitly in display.cpp (heap_caps_malloc).
// Grocery board + chore rows need more than 64KB when both exist briefly.
#define LV_MEM_CUSTOM         0
#define LV_MEM_SIZE           (96U * 1024U)

// Drive LVGL's clock from millis() — without this (or manual lv_tick_inc
// calls, which display.cpp doesn't make) timers and input never advance.
#define LV_TICK_CUSTOM        1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#define LV_DPI_DEF            130

// Fonts actually referenced by ui_manager.cpp
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_DEFAULT       &lv_font_montserrat_20

// Keep the build lean — demos/examples off.
#define LV_USE_PERF_MONITOR   0
#define LV_USE_LOG            0

// Settings screen (Diagnostics → Open Settings): host/port/token editing.
#define LV_USE_TEXTAREA       1
#define LV_USE_KEYBOARD       1

// App page: QR for the Family Hub web URL.
#define LV_USE_QRCODE         1

#endif // LV_CONF_H
