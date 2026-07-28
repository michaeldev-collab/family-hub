#include "shell_gestures.h"

#include "display.h"

namespace {

bool g_begun = false;

// Touch chrome latch state
bool g_touch_was_down = false;
bool g_touch_latched = false;
uint32_t g_touch_down_ms = 0;
uint32_t g_touch_last_rise_ms = 0;
bool g_touch_in_chrome = false;

// Joystick SW (no-op until Elecrow presents hardware)
bool g_joy_was_pressed = false;
bool g_joy_latched = false;
uint32_t g_joy_down_ms = 0;
uint32_t g_joy_last_rise_ms = 0;

bool joystickPresent() {
  // Waveshare primary: no Crowtail stick. Elecrow bring-up can flip this later.
  return false;
}

bool joystickSwPressed() { return false; }

ShellGesture pollTouchChrome(bool sleeping, bool modalBlocks) {
  uint16_t x = 0, y = 0;
  const bool down = displayTouchSample(&x, &y);
  const bool rise = down && !g_touch_was_down;
  const bool fall = !down && g_touch_was_down;
  g_touch_was_down = down;

  // While asleep any tap wakes (chrome is invisible).
  if (sleeping) {
    if (rise) {
      g_touch_latched = false;
      g_touch_last_rise_ms = 0;
      Serial.println("[gest] touch → wake");
      return ShellGesture::SleepToggle;
    }
    return ShellGesture::None;
  }

  if (rise) {
    g_touch_in_chrome = (y < kShellChromeHeightPx);
    g_touch_down_ms = millis();
    g_touch_latched = g_touch_in_chrome;
    if (g_touch_in_chrome) {
      const uint32_t now = millis();
      const bool is_double = (g_touch_last_rise_ms != 0) &&
                             (now - g_touch_last_rise_ms <= kShellDoubleActivateMs);
      g_touch_last_rise_ms = now;
      if (is_double) {
        g_touch_last_rise_ms = 0;
        g_touch_latched = false;
        Serial.println("[gest] touch double-activate → sleep toggle");
        return ShellGesture::SleepToggle;
      }
    }
  }

  if (fall) {
    g_touch_latched = false;
    g_touch_in_chrome = false;
  }

  if (modalBlocks) {
    g_touch_latched = false;
    return ShellGesture::None;
  }

  if (down && g_touch_latched && g_touch_in_chrome) {
    if (millis() - g_touch_down_ms >= kShellHoldDiagMs) {
      g_touch_latched = false;
      g_touch_down_ms = millis() + 100000;  // anti-retrigger until release
      Serial.println("[gest] touch hold → diagnostics");
      return ShellGesture::DiagnosticsToggle;
    }
  }

  return ShellGesture::None;
}

ShellGesture pollJoystick(bool sleeping, bool modalBlocks) {
  if (!joystickPresent()) return ShellGesture::None;

  const bool pressed = joystickSwPressed();
  const bool rise = pressed && !g_joy_was_pressed;
  g_joy_was_pressed = pressed;

  if (rise) {
    const uint32_t now = millis();
    g_joy_last_rise_ms = now;
    // Double-click sleep removed with panel sleep.
    g_joy_down_ms = now;
    g_joy_latched = true;
  }

  if (!pressed) {
    g_joy_latched = false;
    return ShellGesture::None;
  }

  if (sleeping || modalBlocks) {
    g_joy_latched = false;
    return ShellGesture::None;
  }

  if (g_joy_latched && millis() - g_joy_down_ms >= kShellHoldDiagMs) {
    g_joy_latched = false;
    g_joy_down_ms = millis() + 100000;
    Serial.println("[gest] stick hold → diagnostics");
    return ShellGesture::DiagnosticsToggle;
  }

  return ShellGesture::None;
}

}  // namespace

void shellGesturesBegin() {
  g_begun = true;
  Serial.println("[gest] ShellGestures ready (touch chrome; stick no-op until elecrow7)");
  Serial.println("[gest] double-tap header = sleep; any tap while asleep = wake; hold header 2s = diagnostics");
}

ShellGesture shellGesturesPoll(bool modalBlocks) {
  if (!g_begun) return ShellGesture::None;
  const bool sleeping = displayIsSleeping();

  // Sleep wins: check both sources; prefer SleepToggle if either fires.
  const ShellGesture touch = pollTouchChrome(sleeping, modalBlocks);
  if (touch == ShellGesture::SleepToggle) return touch;

  const ShellGesture joy = pollJoystick(sleeping, modalBlocks);
  if (joy == ShellGesture::SleepToggle) return joy;

  if (touch == ShellGesture::DiagnosticsToggle) return touch;
  if (joy == ShellGesture::DiagnosticsToggle) return joy;
  return ShellGesture::None;
}
