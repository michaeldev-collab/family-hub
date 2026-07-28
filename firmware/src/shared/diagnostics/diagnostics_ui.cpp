#include "diagnostics_ui.h"

#ifdef WAVESHARE_7B

#include <lvgl.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include "display.h"
#include "panel_config.h"

namespace {

lv_obj_t* g_root = nullptr;
lv_obj_t* g_title = nullptr;
lv_obj_t* g_wifi = nullptr;
lv_obj_t* g_api = nullptr;
lv_obj_t* g_payload = nullptr;
lv_obj_t* g_meta = nullptr;
lv_obj_t* g_note = nullptr;
bool g_visible = false;
bool (*g_openSettings)() = nullptr;

lv_obj_t* makeLabel(lv_obj_t* parent, lv_coord_t y) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_set_width(label, PANEL_H_RES - 48);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 24, y);
  lv_obj_set_style_text_color(label, lv_color_hex(0xE8EEF5), 0);
  return label;
}

void onClose(lv_event_t* /*e*/) {
  diagnosticsUiHide();
}

void onSettings(lv_event_t* /*e*/) {
  if (g_openSettings && g_openSettings()) {
    diagnosticsUiHide();
  }
}

void ensureBuilt() {
  if (g_root) return;

  g_root = lv_obj_create(lv_layer_top());
  lv_obj_set_size(g_root, PANEL_H_RES, PANEL_V_RES);
  lv_obj_set_pos(g_root, 0, 0);
  lv_obj_set_style_bg_color(g_root, lv_color_hex(0x101820), 0);
  lv_obj_set_style_bg_opa(g_root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(g_root, 0, 0);
  lv_obj_set_style_radius(g_root, 0, 0);
  lv_obj_clear_flag(g_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(g_root, LV_OBJ_FLAG_HIDDEN);

  g_title = makeLabel(g_root, 16);
  lv_obj_set_style_text_font(g_title, &lv_font_montserrat_20, 0);
  lv_label_set_text(g_title, "Family Hub — Diagnostics");

  g_wifi = makeLabel(g_root, 56);
  g_api = makeLabel(g_root, 120);
  g_payload = makeLabel(g_root, 200);
  g_meta = makeLabel(g_root, 280);
  g_note = makeLabel(g_root, 360);
  lv_label_set_text(g_note, "Close · Hold header 2s toggles · Open Settings for host/token");

  lv_obj_t* closeBtn = lv_btn_create(g_root);
  lv_obj_set_size(closeBtn, 220, 56);
  lv_obj_align(closeBtn, LV_ALIGN_BOTTOM_MID, -150, -24);
  lv_obj_add_event_cb(closeBtn, onClose, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* closeLab = lv_label_create(closeBtn);
  lv_label_set_text(closeLab, "Close");
  lv_obj_center(closeLab);

  lv_obj_t* settingsBtn = lv_btn_create(g_root);
  lv_obj_set_size(settingsBtn, 280, 56);
  lv_obj_align(settingsBtn, LV_ALIGN_BOTTOM_MID, 140, -24);
  lv_obj_add_event_cb(settingsBtn, onSettings, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* lab = lv_label_create(settingsBtn);
  lv_label_set_text(lab, "Open Settings");
  lv_obj_center(lab);
}

}  // namespace

void diagnosticsUiBegin() {
  Serial.println("[diag-ui] ready (top-layer overlay; not a nav tab)");
}

void diagnosticsUiHide() {
  if (!g_visible) return;
  g_visible = false;
  if (!displayReady() || !g_root) return;
  displayLock();
  lv_obj_add_flag(g_root, LV_OBJ_FLAG_HIDDEN);
  displayUnlock();
  Serial.println("[diag-ui] hidden");
}

void diagnosticsUiToggle() {
  if (g_visible) {
    diagnosticsUiHide();
  } else {
    g_visible = true;
    Serial.println("[diag-ui] show");
  }
}

bool diagnosticsUiVisible() { return g_visible; }

void diagnosticsUiRender(const PanelStatus& status, bool dashboardValid,
                         int schemaVersion, bool (*openSettingsCb)()) {
  if (!displayReady() || !g_visible) return;
  g_openSettings = openSettingsCb;

  displayLock();
  ensureBuilt();

  char line[192];

  if (WiFi.status() == WL_CONNECTED) {
    snprintf(line, sizeof(line), "Wi-Fi: %s  RSSI %d dBm",
             WiFi.SSID().c_str(), status.wifiRssi);
  } else {
    snprintf(line, sizeof(line), "Wi-Fi: DOWN");
  }
  lv_label_set_text(g_wifi, line);

  snprintf(line, sizeof(line), "API: %s:%u  HTTP %d  err=%s",
           status.serverHost.c_str(), (unsigned)status.serverPort,
           status.lastHttpCode,
           status.lastError.length() ? status.lastError.c_str() : "-");
  lv_label_set_text(g_api, line);

  if (schemaVersion >= 0) {
    snprintf(line, sizeof(line), "Dashboard: %s  schema_version=%d",
             dashboardValid ? "VALID" : "INVALID", schemaVersion);
  } else {
    snprintf(line, sizeof(line), "Dashboard: %s  schema_version=missing",
             dashboardValid ? "VALID" : "INVALID / EMPTY");
  }
  lv_label_set_text(g_payload, line);

  snprintf(line, sizeof(line), "FW %s  device %s  heap free %u",
           status.firmwareVersion.c_str(), status.deviceId.c_str(),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT));
  lv_label_set_text(g_meta, line);

  lv_obj_clear_flag(g_root, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(g_root);
  displayUnlock();
}

#endif
