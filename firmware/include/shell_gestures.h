#pragma once

#include <Arduino.h>

enum class ShellGesture : uint8_t {
  None = 0,
  SleepToggle,
  DiagnosticsToggle,
};

// Chrome = header status bar (Family Hub header is 70px).
static constexpr uint16_t kShellChromeHeightPx = 70;
static constexpr uint32_t kShellHoldDiagMs = 2000;
static constexpr uint32_t kShellDoubleActivateMs = 450;

void shellGesturesBegin();

// Call every loop. At most one action per call; SleepToggle wins over Diagnostics.
// When sleeping, only SleepToggle (wake) is emitted.
// When modalBlocks is true, chrome gestures are ignored (wake still allowed via sleep path).
ShellGesture shellGesturesPoll(bool modalBlocks);
