#include "sleep_button.h"

#include <Arduino.h>

#include "display.h"
#include "panel_config.h"

#if defined(WAVESHARE_7B)

#ifndef SLEEP_BUTTON_GPIO
#define SLEEP_BUTTON_GPIO 6
#endif

namespace {

constexpr uint32_t kDebounceMs = 50;

bool g_begun = false;
bool g_stable_pressed = false;   // debounced level (true = pressed / LOW)
bool g_raw_pressed = false;
uint32_t g_last_change_ms = 0;
bool g_armed = true;             // require release before next press
bool g_toggle_pending = false;

bool rawPressed() {
  return digitalRead(SLEEP_BUTTON_GPIO) == LOW;
}

}  // namespace

void sleepButtonInit() {
  pinMode(SLEEP_BUTTON_GPIO, INPUT_PULLUP);
  g_raw_pressed = rawPressed();
  g_stable_pressed = g_raw_pressed;
  g_last_change_ms = millis();
  g_armed = !g_stable_pressed;  // if held at boot, wait for release
  g_toggle_pending = false;
  g_begun = true;
  Serial.printf("[SleepButton] Initialized on GPIO %d\n", SLEEP_BUTTON_GPIO);
}

void sleepButtonUpdate() {
  if (!g_begun || !displayReady()) return;

  const bool now_raw = rawPressed();
  const uint32_t now = millis();

  if (now_raw != g_raw_pressed) {
    g_raw_pressed = now_raw;
    g_last_change_ms = now;
  }

  if ((now - g_last_change_ms) < kDebounceMs) return;
  if (now_raw == g_stable_pressed) return;

  g_stable_pressed = now_raw;

  if (g_stable_pressed) {
    if (!g_armed) return;
    g_armed = false;
    Serial.println("[SleepButton] Press detected");
    if (displayIsSleeping()) {
      Serial.println("[SleepButton] Waking display");
    } else {
      Serial.println("[SleepButton] Entering sleep");
    }
    g_toggle_pending = true;
  } else {
    g_armed = true;
  }
}

bool sleepButtonConsumeToggle() {
  if (!g_toggle_pending) return false;
  g_toggle_pending = false;
  return true;
}

#else  // !WAVESHARE_7B

void sleepButtonInit() {}
void sleepButtonUpdate() {}
bool sleepButtonConsumeToggle() { return false; }

#endif
