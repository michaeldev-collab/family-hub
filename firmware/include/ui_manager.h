#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "api_client.h"

#ifdef WAVESHARE_7B
struct _lv_obj_t;
#endif

enum class ScreenId {
  Home,
  Grocery,
  Chores,
  Dinner,
  Notes,
  App,
  Settings,  // Diagnostics only — not a bottom-nav tab
};

class UiManager {
 public:
  void begin();
  void render(const PanelStatus& status, const JsonDocument& state, bool stale);
  void setScreen(ScreenId screen) { current_ = screen; renderRequested_ = true; }
  ScreenId screen() const { return current_; }
  void nextScreen();
  void requestSync();
  bool consumeRenderRequest();
  bool consumeSyncRequest();
  bool consumeChoreCompleteRequest(String& outChoreId);
  void reportChoreCompleteResult(bool ok, bool offline);
  bool consumeSettingsSave(String& host, uint16_t& port, String& token,
                           String& wifiSsid, String& wifiPass);
  bool consumeSettingsBack();
#ifdef WAVESHARE_7B
  void handleNavTarget(_lv_obj_t* target);
  void handleChoreRowTap(_lv_obj_t* target);
  void handleGroceryRowTap(_lv_obj_t* target);
  void handleModalPrimary();
  void handleModalSecondary();
  void handleSettingsSave();
  void handleSettingsBack();
  bool modalOpen() const { return modalMode_ != 0; }
  bool consumeGroceryToggleRequest(String& outGroceryId);
#endif
  void showWriteResult(bool ok, const char* action);

 private:
  void renderStatusBar(const PanelStatus& status, bool stale);
  void renderHome(const JsonDocument& state);
  void renderGrocery(const JsonDocument& state);
  void renderChores(const JsonDocument& state);
  void renderDinner(const JsonDocument& state);
  void renderNotes(const JsonDocument& state);
  void renderApp(const PanelStatus& status);
  void renderSettings(const PanelStatus& status);
  const char* badgeLabel(const PanelStatus& status, bool stale) const;

#ifdef WAVESHARE_7B
  void resetLvglPointers();
  void lvglEnsureScreen();
  void lvglUpdate(const PanelStatus& status, const JsonDocument& state, bool stale);
  void lvglUpdateNav();
  void lvglLayoutCard(int index, int x, int y, int w, int h, bool bodyLarge = false,
                      bool bodyScroll = false);
  void lvglSetCard(int index, const char* title, const String& body, bool visible = true);
  _lv_obj_t* headerBar_ = nullptr;
  _lv_obj_t* brandTitle_ = nullptr;
  _lv_obj_t* brandSub_ = nullptr;
  _lv_obj_t* syncButton_ = nullptr;
  _lv_obj_t* syncLabel_ = nullptr;
  _lv_obj_t* statusPill_ = nullptr;
  _lv_obj_t* statusDot_ = nullptr;
  _lv_obj_t* statusText_ = nullptr;
  _lv_obj_t* pageTitle_ = nullptr;
  _lv_obj_t* pageMeta_ = nullptr;
  _lv_obj_t* cards_[4] = {};
  _lv_obj_t* cardTitles_[4] = {};
  _lv_obj_t* cardBodies_[4] = {};       // scroll viewport
  _lv_obj_t* cardBodyLabels_[4] = {};   // text inside viewport
  _lv_obj_t* toast_ = nullptr;
  static const int kNavTabCount = 6;
  _lv_obj_t* navButtons_[kNavTabCount] = {};
  _lv_obj_t* navLabels_[kNavTabCount] = {};
  _lv_obj_t* navBg_ = nullptr;

  _lv_obj_t* settingsPanel_ = nullptr;
  _lv_obj_t* settingsStatus_ = nullptr;
  _lv_obj_t* settingsWifiSsid_ = nullptr;
  _lv_obj_t* settingsWifiPass_ = nullptr;
  _lv_obj_t* settingsHost_ = nullptr;
  _lv_obj_t* settingsPort_ = nullptr;
  _lv_obj_t* settingsToken_ = nullptr;
  _lv_obj_t* settingsKeyboard_ = nullptr;
  bool settingsSeeded_ = false;

  _lv_obj_t* appPanel_ = nullptr;
  _lv_obj_t* appUrlLabel_ = nullptr;
  _lv_obj_t* appHintLabel_ = nullptr;
  _lv_obj_t* appQr_ = nullptr;
  String appQrUrl_;

  static const int kMaxChoreRows = 8;
  _lv_obj_t* choreList_ = nullptr;
  _lv_obj_t* choreRowBtns_[kMaxChoreRows] = {};
  _lv_obj_t* choreRowLabels_[kMaxChoreRows] = {};
  String     choreRowIds_[kMaxChoreRows];
  int        choreRowCount_ = 0;

  static const int kGroceryCols = 3;
  // Keep lean — 21 buttons exhausted the 64KB LVGL pool alongside chore rows.
  static const int kGroceryRowsPerCol = 5;
  _lv_obj_t* groceryBoard_ = nullptr;
  _lv_obj_t* groceryColTitles_[kGroceryCols] = {};
  _lv_obj_t* groceryRowBtns_[kGroceryCols][kGroceryRowsPerCol] = {};
  _lv_obj_t* groceryRowLabels_[kGroceryCols][kGroceryRowsPerCol] = {};
  char       groceryRowIds_[kGroceryCols][kGroceryRowsPerCol][40] = {};

  _lv_obj_t* modal_ = nullptr;
  _lv_obj_t* modalTitle_ = nullptr;
  _lv_obj_t* modalBody_ = nullptr;
  _lv_obj_t* modalStatus_ = nullptr;
  _lv_obj_t* modalPrimaryBtn_ = nullptr;
  _lv_obj_t* modalPrimaryLabel_ = nullptr;
  _lv_obj_t* modalSecondaryBtn_ = nullptr;
  _lv_obj_t* modalSecondaryLabel_ = nullptr;
  int    modalMode_ = 0;

  void buildChoreModal();
  void openChoreDetail(int rowIndex);
  void closeChoreModal();
  void refreshModalForMode();
  void applyPendingChoreResult();
  void populateChoreRows(JsonArrayConst chores);
  void hideChoreRows();
  void ensureGroceryBoard();
  void destroyGroceryBoard();
  void populateGroceryBoard(JsonObjectConst grocery);
  void hideGroceryBoard();
  void buildSettingsPanel(_lv_obj_t* scr);
  void showSettingsPanel(const PanelStatus& status, bool show);
  void dismissSettingsKeyboard();
  void buildAppPanel(_lv_obj_t* scr);
  void showAppPanel(const PanelStatus& status, const JsonDocument& state, bool show);
#endif

  ScreenId current_ = ScreenId::Home;
  bool syncRequested_ = false;
  bool renderRequested_ = false;
  bool lastWriteOk_ = false;
  String lastWriteAction_;
  unsigned long lastWriteMs_ = 0;

  bool choreCompleteRequested_ = false;
  bool   choreResultPending_ = false;
  bool   choreResultOk_ = false;
  bool   choreResultOffline_ = false;
  String pendingChoreId_;
  String pendingChoreTitle_;

  bool groceryToggleRequested_ = false;
  String pendingGroceryId_;

  bool settingsSaveRequested_ = false;
  bool settingsBackRequested_ = false;
  String pendingSettingsHost_;
  uint16_t pendingSettingsPort_ = 0;
  String pendingSettingsToken_;
  String pendingSettingsWifiSsid_;
  String pendingSettingsWifiPass_;
};
