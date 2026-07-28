#pragma once
//
// sleep_button.h — physical sleep/wake button on Waveshare GPIO header GP6.
// Momentary switch between GPIO 6 and GND; INPUT_PULLUP (pressed = LOW).
// Poll from the main loop — never call display sleep/wake from an ISR.
// On a debounced press, sleepButtonConsumeToggle() returns true once so the
// app can call the same toggle path as touch/serial (shared sleep state).
//

void sleepButtonInit();
void sleepButtonUpdate();
bool sleepButtonConsumeToggle();
