#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "api_client.h"

#ifdef WAVESHARE_7B

void diagnosticsUiBegin();
void diagnosticsUiToggle();
bool diagnosticsUiVisible();
void diagnosticsUiHide();
void diagnosticsUiRender(const PanelStatus& status, bool dashboardValid,
                         int schemaVersion, bool (*openSettingsCb)());

#else

inline void diagnosticsUiBegin() {}
inline void diagnosticsUiToggle() {}
inline bool diagnosticsUiVisible() { return false; }
inline void diagnosticsUiHide() {}
inline void diagnosticsUiRender(const PanelStatus&, bool, int, bool (*)()) {}

#endif
