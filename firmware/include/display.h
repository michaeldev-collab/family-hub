#pragma once
//
// display.h — panel + LVGL bring-up for Family Hub.
//
// On WAVESHARE_7B: brings up ST7701 RGB panel (1024x600), GT911 touch, CH32V003
// backlight, and an LVGL v8 display + input device. Exposes a persistent screen
// that ui_manager.cpp draws into.
//
// On devkit / elecrow (no WAVESHARE_7B): all calls are cheap no-ops so the
// existing serial UI keeps working unchanged.
//
#include <Arduino.h>

// True once the panel + LVGL are live (false on devkit / on bring-up failure).
bool displayReady();

// Initialize panel, touch, backlight, LVGL. Safe to call once in setup().
// Returns false (and leaves displayReady()==false) if the panel is absent or
// bring-up fails — caller should fall back to serial UI.
bool displayBegin();

// Pump LVGL timers + touch. Call frequently from loop(). No-op if not ready.
void displayTick();

// Last LVGL pointer state. Callers use this to defer background sync/image
// work until a toddler's press has completed.
bool displayTouchActive();

// Report LVGL allocator health and decide whether a full dashboard update can
// safely duplicate the upcoming text into LVGL. requiredTextBytes is derived
// from the compact state being rendered, not a fixed guessed heap threshold.
bool displayRenderMemoryAvailable(size_t requiredTextBytes, const char* boundary);

// Recursive LVGL lock/unlock — ui_manager must hold this around any lv_* call,
// because LVGL is not thread-safe and displayTick() also touches it.
void displayLock();
void displayUnlock();

// Set backlight brightness 0..100. No-op if not ready.
void displaySetBacklight(uint8_t percent);

// Cycle backlight kill combinations over serial (key 'B'); watch glass.
void displayBacklightProbe();

// Force panel out of stuck-dark (BL off while firmware thinks it's awake).
// Serial key 'u'. Safe anytime after displayBegin().
void displayForceBacklightOn();

// Real sleep: black framebuffers + settle + backlight DISP off + pause LVGL.
// Must not leave a static UI scanning with backlight only off (burn-in path).
void displayEnterSleep();
void displayWake();
bool displayIsSleeping();
void displayForceFullRedraw();

// Latest pointer sample for shell gestures (works while sleeping).
// Returns true when a finger is currently down.
bool displayTouchSample(uint16_t* x, uint16_t* y);

// Soft-reinit GT911 (sleep health / I2C recover). Safe no-op if display not ready.
bool displayTouchReinit();
