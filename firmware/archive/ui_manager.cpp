#include "ui_manager.h"

#ifdef WAVESHARE_7B
#include <lvgl.h>
#include "display.h"
#include "panel_config.h"
#endif

void UiManager::begin() {
  Serial.println("[ui] Family Hub panel UI ready (serial fallback / LVGL hook)");
}

#ifndef FAMILY_HUB_APP_CHILD
void UiManager::nextScreen() {
  int n = static_cast<int>(current_) + 1;
  if (n > static_cast<int>(ScreenId::Settings)) n = 0;
  current_ = static_cast<ScreenId>(n);
  renderRequested_ = true;
}
#endif

void UiManager::showWriteResult(bool ok, const char* action) {
  lastWriteOk_ = ok;
  lastWriteAction_ = action ? action : "";
  lastWriteMs_ = millis();
  renderRequested_ = true;
}

bool UiManager::consumeRenderRequest() {
  bool requested = renderRequested_;
  renderRequested_ = false;
  return requested;
}

void UiManager::requestSync() {
  syncRequested_ = true;
}

bool UiManager::consumeSyncRequest() {
  bool requested = syncRequested_;
  syncRequested_ = false;
  return requested;
}

#ifndef FAMILY_HUB_APP_CHILD
bool UiManager::consumeChoreCompleteRequest(String& outChoreId) {
  if (!choreCompleteRequested_) return false;
  choreCompleteRequested_ = false;   // dispatched once — modal stays in-flight
  outChoreId = pendingChoreId_;
  return true;
}
#endif

#ifndef FAMILY_HUB_APP_HOUSEHOLD
bool UiManager::consumeChildModeEnterRequest() { bool value=childModeEnterRequested_; childModeEnterRequested_=false; return value; }
bool UiManager::consumeChildExitRequest() { bool value=childExitRequested_; childExitRequested_=false; return value; }
bool UiManager::consumeChildSelectionRequest(String& childId) {
  if (!childSelectionRequested_) return false;
  childSelectionRequested_=false; childId=requestedChildId_; return true;
}
bool UiManager::consumeChildActionRequest(String& kind, String& targetId) {
  if (!childActionRequested_) return false;
  childActionRequested_=false; kind=childActionKind_; targetId=childActionTargetId_; return true;
}
bool UiManager::consumeChildPinRequest(String& pin) {
  if (!childPinRequested_) return false;
  childPinRequested_ = false; pin = requestedChildPin_; requestedChildPin_ = "";
  return true;
}
void UiManager::reportChildActionResult(bool ok, const char* action) {
  showWriteResult(ok, action);
#ifdef WAVESHARE_7B
  childScreenDirty_ = true;
#endif
}

void UiManager::renderChildHome(const PanelStatus& status, const JsonDocument& home,
                                const ChildFocusRuntime& runtime) {
#ifdef WAVESHARE_7B
  if (displayReady()) rebuildChildScreen(status, home, runtime, true);
#endif
  Serial.printf("== CHILD SELECTION == stale=%s profiles=%u\n", runtime.stale?"yes":"no",
                (unsigned)home["children"].as<JsonArrayConst>().size());
}

void UiManager::renderChildMode(const PanelStatus& status, const JsonDocument& state,
                                const ChildFocusRuntime& runtime) {
#ifdef WAVESHARE_7B
  if (displayReady()) {
    const String fingerprint = buildChildRenderFingerprint(state, runtime, false);
    if (childScreenDirty_ || fingerprint != childRenderFingerprint_) {
      rebuildChildScreen(status, state, runtime, false);
      childRenderFingerprint_ = fingerprint;
      childScreenDirty_ = false;
    }
  }
#endif
  Serial.printf("== CHILD MODE == child=%s page=%s stale=%s focus=%s\n",
                runtime.selectedChildId.c_str(), childFocusPageName(runtime.page), runtime.stale?"yes":"no",
                state["focus"]["type"] | "dashboard");
}

#endif

#ifndef FAMILY_HUB_APP_CHILD
void UiManager::reportChoreCompleteResult(bool ok, bool offline) {
  // Keep network completion free of LVGL calls. The next admitted full render
  // applies this modal result on the loop/LVGL task under the display lock.
  choreResultOk_ = ok;
  choreResultOffline_ = offline;
  choreResultPending_ = true;
  renderRequested_ = true;
}
#endif

const char* UiManager::badgeLabel(const PanelStatus& status, bool stale) const {
  if (status.conn == ConnState::WifiDisconnected) return "OFFLINE";
  if (status.conn == ConnState::ServerOffline) return "OFFLINE";
  if (stale || status.conn == ConnState::Stale) return "STALE DATA";
  return "ONLINE";
}

void UiManager::renderStatusBar(const PanelStatus& status, bool stale) {
  const char* badge = badgeLabel(status, stale);

  Serial.println("--- STATUS ---");
  Serial.printf("[badge] %s\n", badge);
  Serial.printf("conn=%s stale=%s rssi=%d host=%s:%u fw=%s device=%s\n",
                badge, stale ? "yes" : "no", status.wifiRssi,
                status.serverHost.c_str(), status.serverPort,
                status.firmwareVersion.c_str(), status.deviceId.c_str());
  if (status.lastError.length()) Serial.printf("lastError=%s\n", status.lastError.c_str());
  if (status.lastSyncMs) Serial.printf("lastSyncMs=%lu\n", status.lastSyncMs);

  if (lastWriteMs_ && millis() - lastWriteMs_ < 8000) {
    Serial.printf("lastWrite=%s %s\n", lastWriteOk_ ? "OK" : "FAILED", lastWriteAction_.c_str());
  }

#ifdef WAVESHARE_7B
  // Status badge + content are drawn via LVGL in lvglUpdate(); see render().
#elif defined(ELECROW_7)
  // LVGL (LovyanGFX): same badge/footer contract as Waveshare — init in panel bring-up, not here yet.
#endif
}

#ifndef FAMILY_HUB_APP_CHILD
void UiManager::renderHome(const JsonDocument& state) {
  Serial.println("== HOME ==");

  JsonArrayConst pinned = state["pinnedNotes"].as<JsonArrayConst>();
  if (!pinned.isNull() && pinned.size() > 0) {
    Serial.println("Pinned:");
    for (JsonObjectConst n : pinned) {
      Serial.printf(" * %s\n", n["text"].as<const char*>());
    }
  }

  const char* meal = state["dinner"]["meal"] | "No dinner planned";
  Serial.printf("Dinner today: %s\n", meal);

  JsonArrayConst week = state["weekDinner"].as<JsonArrayConst>();
  if (!week.isNull() && week.size() > 0) {
    Serial.println("Week ahead:");
    int shown = 0;
    for (JsonObjectConst d : week) {
      if (shown++ >= 3) break;
      Serial.printf(" %s: %s\n", d["date"].as<const char*>(), d["meal"].as<const char*>());
    }
  }

  JsonArrayConst chores = state["chores"].as<JsonArrayConst>();
  Serial.printf("Open chores: %u\n", chores.isNull() ? 0 : chores.size());
  for (JsonObjectConst c : chores) {
    Serial.printf(" - %s\n", c["title"].as<const char*>());
  }

  JsonArrayConst grocery = state["grocery"].as<JsonArrayConst>();
  Serial.printf("Grocery items: %u\n", grocery.isNull() ? 0 : grocery.size());
  for (JsonObjectConst g : grocery) {
    Serial.printf(" - %s\n", g["text"].as<const char*>());
  }
}

void UiManager::renderGrocery(const JsonDocument& state) {
  Serial.println("== GROCERY ==");
  for (JsonObjectConst g : state["grocery"].as<JsonArrayConst>()) {
    Serial.printf(" - %s\n", g["text"].as<const char*>());
  }
}

void UiManager::renderChores(const JsonDocument& state) {
  Serial.println("== CHORES ==");
  for (JsonObjectConst c : state["chores"].as<JsonArrayConst>()) {
    Serial.printf(" - %s\n", c["title"].as<const char*>());
  }
}

void UiManager::renderDinner(const JsonDocument& state) {
  Serial.println("== DINNER ==");
  Serial.printf("Today: %s\n", state["dinner"]["meal"] | "Unassigned");
  JsonArrayConst week = state["weekDinner"].as<JsonArrayConst>();
  if (!week.isNull()) {
    Serial.println("This week:");
    for (JsonObjectConst d : week) {
      Serial.printf(" %s: %s\n", d["date"].as<const char*>(), d["meal"].as<const char*>());
    }
  }
}

void UiManager::renderNotes(const JsonDocument& state) {
  Serial.println("== NOTES ==");
  JsonArrayConst pinned = state["pinnedNotes"].as<JsonArrayConst>();
  if (!pinned.isNull() && pinned.size() > 0) {
    Serial.println("Pinned:");
    for (JsonObjectConst n : pinned) {
      Serial.printf(" * %s\n", n["text"].as<const char*>());
    }
  }
  for (JsonObjectConst n : state["notes"].as<JsonArrayConst>()) {
    Serial.printf(" - %s\n", n["text"].as<const char*>());
  }
}

void UiManager::renderSettings(const PanelStatus& status) {
  Serial.println("== SETTINGS ==");
  Serial.printf("Device: %s\n", status.deviceId.c_str());
  Serial.printf("Firmware: %s\n", status.firmwareVersion.c_str());
  Serial.printf("Server: %s:%u\n", status.serverHost.c_str(), status.serverPort);
  Serial.printf("WiFi RSSI: %d dBm\n", status.wifiRssi);
  if (status.lastSyncMs) {
    Serial.printf("Last sync: %lu ms ago\n", millis() - status.lastSyncMs);
  }
  Serial.println("NVS: srv_host, srv_port, write_tok (serial h=reset host, w=set test token)");
}

void UiManager::render(const PanelStatus& status, const JsonDocument& state, bool stale) {
  renderStatusBar(status, stale);

#ifdef WAVESHARE_7B
  if (displayReady()) {
    lvglUpdate(status, state, stale);
  }
#elif defined(ELECROW_7)
  // LVGL screen hooks (LovyanGFX + GT911): mirror 7B ScreenId layout — see docs/panel-targets.md
  // Display init: LovyanGFX RGB bus + touch_init() per docs/elecrow-esp32-display-7-reference.md
#endif

  switch (current_) {
    case ScreenId::Home: renderHome(state); break;
    case ScreenId::Grocery: renderGrocery(state); break;
    case ScreenId::Chores: renderChores(state); break;
    case ScreenId::Dinner: renderDinner(state); break;
    case ScreenId::Notes: renderNotes(state); break;
    case ScreenId::Settings: renderSettings(status); break;
  }
  Serial.println();
}

#endif

#ifdef WAVESHARE_7B

#ifndef FAMILY_HUB_APP_CHILD
static const char* screenName(ScreenId s) {
  switch (s) {
    case ScreenId::Home:     return "HOME";
    case ScreenId::Grocery:  return "GROCERY";
    case ScreenId::Chores:   return "CHORES";
    case ScreenId::Dinner:   return "DINNER";
    case ScreenId::Notes:    return "NOTES";
    case ScreenId::Settings: return "SETUP";
  }
  return "?";
}

static const char* screenTitle(ScreenId s) {
  switch (s) {
    case ScreenId::Home:     return "Home";
    case ScreenId::Grocery:  return "Grocery List";
    case ScreenId::Chores:   return "Chores";
    case ScreenId::Dinner:   return "Dinner Plan";
    case ScreenId::Notes:    return "Notes";
    case ScreenId::Settings: return "Setup";
  }
  return "?";
}

static const char* navLabel(ScreenId s) {
  switch (s) {
    case ScreenId::Home:     return "Home";
    case ScreenId::Grocery:  return "Grocery";
    case ScreenId::Chores:   return "Chores";
    case ScreenId::Dinner:   return "Dinner";
    case ScreenId::Notes:    return "Notes";
    case ScreenId::Settings: return "Setup";
  }
  return "?";
}

static uint32_t badgeColor(const char* badge) {
  if (strcmp(badge, "ONLINE") == 0) return 0x00CC33;
  if (strstr(badge, "OFFLINE")) return 0xFF4444;
  return 0xF0A500;
}

static String formatLastSync(const PanelStatus& status) {
  if (!status.lastSyncMs) return "Not updated yet";
  unsigned long age = (millis() - status.lastSyncMs) / 1000;
  if (age < 60) return String("Updated ") + age + "s ago";
  return String("Updated ") + (age / 60) + "m ago";
}

static unsigned int jsonArraySize(JsonArrayConst arr) {
  return arr.isNull() ? 0 : arr.size();
}

static String listItems(JsonArrayConst arr, const char* field, int maxItems, const char* emptyText) {
  String s;
  if (arr.isNull() || arr.size() == 0) return String(emptyText);
  int shown = 0;
  for (JsonObjectConst item : arr) {
    if (shown++ >= maxItems) break;
    const char* value = item[field] | "";
    if (!value || !*value) continue;
    s += "- ";
    s += value;
    s += "\n";
  }
  if ((int)arr.size() > maxItems) {
    s += "+ ";
    s += String((int)arr.size() - maxItems);
    s += " more\n";
  }
  return s;
}

static String firstItems(JsonArrayConst arr, const char* field, int maxItems, const char* emptyText) {
  String s;
  if (arr.isNull() || arr.size() == 0) return String(emptyText);
  int shown = 0;
  for (JsonObjectConst item : arr) {
    if (shown++ >= maxItems) break;
    const char* value = item[field] | "";
    if (!value || !*value) continue;
    if (s.length()) s += "\n";
    s += value;
  }
  return s.length() ? s : String(emptyText);
}

static String dinnerWeek(JsonArrayConst arr, int maxItems) {
  String s;
  if (arr.isNull() || arr.size() == 0) return "No meals planned this week";
  int shown = 0;
  for (JsonObjectConst item : arr) {
    if (shown++ >= maxItems) break;
    s += item["date"] | "";
    s += "\n  ";
    s += item["meal"] | "Unassigned";
    s += "\n";
  }
  return s;
}

static String compactToday(JsonArrayConst chores, JsonArrayConst grocery, JsonArrayConst pinned) {
  String s;
  s += "CHORES\n";
  s += firstItems(chores, "title", 2, "All caught up");
  s += "\n\nGROCERY\n";
  s += firstItems(grocery, "text", 2, "List is empty");
  s += "\n\nPINNED\n";
  s += firstItems(pinned, "text", 1, "No pinned notes");
  return s;
}

static String homeNext(JsonArrayConst chores, JsonArrayConst grocery, JsonArrayConst pinned) {
  String s;
  s += "Chore: ";
  s += firstItems(chores, "title", 1, "All caught up");
  s += "\nGrocery: ";
  s += firstItems(grocery, "text", 1, "List is empty");
  s += "\nNote: ";
  s += firstItems(pinned, "text", 1, "No pinned notes");
  return s;
}

static void navEventCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (!ui) return;
  ui->handleNavTarget((_lv_obj_t*)lv_event_get_target(e));
}

static void syncEventCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (!ui) return;
  ui->requestSync();
}

static void choreRowCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (!ui) return;
  ui->handleChoreRowTap((_lv_obj_t*)lv_event_get_target(e));
}

static void modalPrimaryCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (!ui) return;
  ui->handleModalPrimary();
}

static void modalSecondaryCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (!ui) return;
  ui->handleModalSecondary();
}
#endif

#ifndef FAMILY_HUB_APP_HOUSEHOLD
static void childModeToggleCb(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) ((UiManager*)lv_event_get_user_data(e))->handleChildModeToggle();
}
static void childProfileCb(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) ((UiManager*)lv_event_get_user_data(e))->handleChildProfileTap((_lv_obj_t*)lv_event_get_target(e));
}
static void childTaskCb(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) ((UiManager*)lv_event_get_user_data(e))->handleChildTaskTap((_lv_obj_t*)lv_event_get_target(e));
}
static void childRewardCb(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) ((UiManager*)lv_event_get_user_data(e))->handleChildRewardTap((_lv_obj_t*)lv_event_get_target(e));
}
static void childExitCb(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_LONG_PRESSED) ((UiManager*)lv_event_get_user_data(e))->handleChildExitTap();
}
static void childChangeChildCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (ui) ui->requestChildNav("change-child");
}
static void childNavCb(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) ((UiManager*)lv_event_get_user_data(e))->handleChildNavTap((_lv_obj_t*)lv_event_get_target(e));
}

void UiManager::requestChildNav(const char* kind) {
  childActionKind_ = kind ? kind : "";
  childActionTargetId_ = "";
  childActionRequested_ = true;
}
static void childPinCb(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) ((UiManager*)lv_event_get_user_data(e))->handleChildPinTap((_lv_obj_t*)lv_event_get_target(e));
}

void UiManager::clearChildFocusScreen() {
  if (!displayReady()) return;
  displayLock(); lv_obj_clean(lv_scr_act()); resetLvglPointers(); displayUnlock();
  childScreenDirty_ = true;
  childRenderFingerprint_ = "";
}

void UiManager::clearChildPanelImages() {
  childHeaderImage_ = nullptr;
  for (int i = 0; i < kMaxChildProfiles; ++i) childProfileImages_[i] = nullptr;
  for (int i = 0; i < kMaxChildTasks; ++i) childTaskImages_[i] = nullptr;
  for (int i = 0; i < kMaxChildRewards; ++i) childRewardImages_[i] = nullptr;
  for (int i = 0; i < kMaxChildRoutines; ++i) childRoutineImages_[i] = nullptr;
  childStripDailyImage_ = nullptr;
  childStripTermImage_ = nullptr;
}

void UiManager::setChildProfileImage(int index, const PanelImage* image) {
  if (index >= 0 && index < kMaxChildProfiles) childProfileImages_[index] = image;
}

void UiManager::setChildTaskImage(int index, const PanelImage* image) {
  if (index >= 0 && index < kMaxChildTasks) childTaskImages_[index] = image;
}

void UiManager::setChildRewardImage(int index, const PanelImage* image) {
  if (index >= 0 && index < kMaxChildRewards) childRewardImages_[index] = image;
}

void UiManager::setChildRoutineImage(int index, const PanelImage* image) {
  if (index >= 0 && index < kMaxChildRoutines) childRoutineImages_[index] = image;
}

void UiManager::setChildStripImages(const PanelImage* daily, const PanelImage* term) {
  childStripDailyImage_ = daily;
  childStripTermImage_ = term;
}
#endif

#ifndef FAMILY_HUB_APP_CHILD
void UiManager::lvglEnsureScreen() {
  if (headerBar_) return;
  displayLock();
  lv_obj_t* scr = lv_scr_act();
  lv_obj_clean(scr);  // replace the retained boot diagnostic screen atomically
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A1A0E), 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* header = lv_obj_create(scr);
  lv_obj_set_size(header, PANEL_H_RES, 70);
  lv_obj_set_pos(header, 0, 0);
  lv_obj_set_style_bg_color(header, lv_color_hex(0x0D2212), 0);
  lv_obj_set_style_border_width(header, 1, 0);
  lv_obj_set_style_border_color(header, lv_color_hex(0x1F6B2D), 0);
  lv_obj_set_style_radius(header, 0, 0);
  lv_obj_set_style_pad_all(header, 0, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  headerBar_ = (_lv_obj_t*)header;

  lv_obj_t* mark = lv_obj_create(header);
  lv_obj_set_size(mark, 38, 38);
  lv_obj_set_pos(mark, 22, 16);
  lv_obj_set_style_bg_color(mark, lv_color_hex(0x00FF41), 0);
  lv_obj_set_style_border_width(mark, 0, 0);
  lv_obj_set_style_radius(mark, 6, 0);

  lv_obj_t* markText = lv_label_create(mark);
  lv_label_set_text(markText, "3D");
  lv_obj_set_style_text_color(markText, lv_color_hex(0x0A1A0E), 0);
  lv_obj_set_style_text_font(markText, &lv_font_montserrat_20, 0);
  lv_obj_center(markText);

  lv_obj_t* title = lv_label_create(header);
  lv_label_set_text(title, "Family Hub");
  lv_obj_set_pos(title, 74, 9);
  lv_obj_set_style_text_color(title, lv_color_hex(0xF2F5F0), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  brandTitle_ = (_lv_obj_t*)title;

  lv_obj_t* sub = lv_label_create(header);
  lv_label_set_text(sub, "3D DESIGN LABS");
  lv_obj_set_pos(sub, 76, 43);
  lv_obj_set_style_text_color(sub, lv_color_hex(0x8AAB8E), 0);
  lv_obj_set_style_text_font(sub, &lv_font_montserrat_20, 0);
  brandSub_ = (_lv_obj_t*)sub;

  lv_obj_t* sync = lv_btn_create(header);
  lv_obj_set_size(sync, 118, 40);
  lv_obj_set_pos(sync, 292, 15);
  lv_obj_set_style_radius(sync, 20, 0);
  lv_obj_set_style_bg_color(sync, lv_color_hex(0x132B18), 0);
  lv_obj_set_style_border_width(sync, 1, 0);
  lv_obj_set_style_border_color(sync, lv_color_hex(0x1F6B2D), 0);
  lv_obj_add_event_cb(sync, syncEventCb, LV_EVENT_CLICKED, this);
  syncButton_ = (_lv_obj_t*)sync;

  lv_obj_t* syncLabel = lv_label_create(sync);
  lv_label_set_text(syncLabel, "Sync");
  lv_obj_set_style_text_color(syncLabel, lv_color_hex(0x00FF41), 0);
  lv_obj_set_style_text_font(syncLabel, &lv_font_montserrat_20, 0);
  lv_obj_center(syncLabel);
  syncLabel_ = (_lv_obj_t*)syncLabel;

#ifndef FAMILY_HUB_APP_HOUSEHOLD
  lv_obj_t* childMode = lv_btn_create(header);
  lv_obj_set_size(childMode, 158, 40);
  lv_obj_set_pos(childMode, 424, 15);
  lv_obj_set_style_radius(childMode, 20, 0);
  lv_obj_set_style_bg_color(childMode, lv_color_hex(0x2C2147), 0);
  lv_obj_set_style_border_width(childMode, 1, 0);
  lv_obj_set_style_border_color(childMode, lv_color_hex(0xA985FF), 0);
  lv_obj_add_event_cb(childMode, childModeToggleCb, LV_EVENT_CLICKED, this);
  childModeButton_ = (_lv_obj_t*)childMode;
  lv_obj_t* childModeLabel = lv_label_create(childMode);
  lv_label_set_text(childModeLabel, LV_SYMBOL_EYE_OPEN "  Child");
  lv_obj_set_style_text_color(childModeLabel, lv_color_hex(0xE6DBFF), 0);
  lv_obj_set_style_text_font(childModeLabel, &lv_font_montserrat_20, 0);
  lv_obj_center(childModeLabel);
#endif

  lv_obj_t* pill = lv_obj_create(header);
  lv_obj_set_size(pill, 250, 40);
  lv_obj_align(pill, LV_ALIGN_RIGHT_MID, -24, 0);
  lv_obj_set_style_bg_color(pill, lv_color_hex(0x132B18), 0);
  lv_obj_set_style_border_width(pill, 1, 0);
  lv_obj_set_style_border_color(pill, lv_color_hex(0x1F6B2D), 0);
  lv_obj_set_style_radius(pill, 19, 0);
  lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
  statusPill_ = (_lv_obj_t*)pill;

  lv_obj_t* dot = lv_obj_create(pill);
  lv_obj_set_size(dot, 12, 12);
  lv_obj_set_pos(dot, 18, 14);
  lv_obj_set_style_radius(dot, 6, 0);
  lv_obj_set_style_border_width(dot, 0, 0);
  statusDot_ = (_lv_obj_t*)dot;

  lv_obj_t* status = lv_label_create(pill);
  lv_obj_set_pos(status, 40, 7);
  lv_obj_set_width(status, 190);
  lv_label_set_long_mode(status, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_font(status, &lv_font_montserrat_20, 0);
  statusText_ = (_lv_obj_t*)status;

  lv_obj_t* page = lv_label_create(scr);
  lv_obj_set_pos(page, 24, 84);
  lv_obj_set_width(page, 460);
  lv_label_set_long_mode(page, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_color(page, lv_color_hex(0xF2F5F0), 0);
  lv_obj_set_style_text_font(page, &lv_font_montserrat_28, 0);
  pageTitle_ = (_lv_obj_t*)page;

  lv_obj_t* meta = lv_label_create(scr);
  lv_obj_align(meta, LV_ALIGN_TOP_RIGHT, -24, 91);
  lv_obj_set_width(meta, 500);
  lv_label_set_long_mode(meta, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(meta, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_color(meta, lv_color_hex(0x8AAB8E), 0);
  lv_obj_set_style_text_font(meta, &lv_font_montserrat_20, 0);
  pageMeta_ = (_lv_obj_t*)meta;

  for (int i = 0; i < 4; ++i) {
    lv_obj_t* card = lv_obj_create(scr);
    lv_obj_set_size(card, 100, 100);
    lv_obj_set_pos(card, 24, 126);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x132B18), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0x1F6B2D), 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* cardTitle = lv_label_create(card);
    lv_obj_set_pos(cardTitle, 16, 12);
    lv_obj_set_width(cardTitle, 68);
    lv_obj_set_style_text_color(cardTitle, lv_color_hex(0x00FF41), 0);
    lv_obj_set_style_text_font(cardTitle, &lv_font_montserrat_20, 0);

    lv_obj_t* body = lv_label_create(card);
    lv_obj_set_pos(body, 16, 44);
    lv_obj_set_width(body, 68);
    lv_obj_set_height(body, 44);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(body, lv_color_hex(0xF2F5F0), 0);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_line_space(body, 4, 0);

    cards_[i] = (_lv_obj_t*)card;
    cardTitles_[i] = (_lv_obj_t*)cardTitle;
    cardBodies_[i] = (_lv_obj_t*)body;
  }

  lv_obj_t* navBg = lv_obj_create(scr);
  lv_obj_set_size(navBg, PANEL_H_RES, 86);
  lv_obj_set_pos(navBg, 0, PANEL_V_RES - 86);
  lv_obj_set_style_bg_color(navBg, lv_color_hex(0x0D2212), 0);
  lv_obj_set_style_border_width(navBg, 1, 0);
  lv_obj_set_style_border_color(navBg, lv_color_hex(0x1F6B2D), 0);
  lv_obj_set_style_radius(navBg, 0, 0);
  lv_obj_clear_flag(navBg, LV_OBJ_FLAG_SCROLLABLE);

  const int nav_y = PANEL_V_RES - 68;
  const int gap = 10;
  const int nav_w = (PANEL_H_RES - 48 - (gap * 5)) / 6;
  for (int i = 0; i < 6; ++i) {
    lv_obj_t* btn = lv_btn_create(scr);
    lv_obj_set_size(btn, nav_w, 50);
    lv_obj_set_pos(btn, 24 + i * (nav_w + gap), nav_y);
    lv_obj_set_style_radius(btn, 26, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x1F6B2D), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x132B18), 0);
    lv_obj_add_event_cb(btn, navEventCb, LV_EVENT_CLICKED, this);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, navLabel((ScreenId)i));
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_center(label);
    navButtons_[i] = (_lv_obj_t*)btn;
    navLabels_[i] = (_lv_obj_t*)label;
  }

  lv_obj_t* toast = lv_label_create(scr);
  lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, -96);
  lv_obj_set_style_text_color(toast, lv_color_hex(0x00FF41), 0);
  lv_obj_set_style_text_font(toast, &lv_font_montserrat_20, 0);
  lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN);
  toast_ = (_lv_obj_t*)toast;

  // Chores page: scrollable list of tappable rows, occupying the same area as
  // the big list card. Hidden unless the Chores page is active.
  lv_obj_t* list = lv_obj_create(scr);
  lv_obj_set_pos(list, 24, 126);
  lv_obj_set_size(list, 620, 374);
  lv_obj_set_style_bg_color(list, lv_color_hex(0x0A1A0E), 0);
  lv_obj_set_style_border_width(list, 0, 0);
  lv_obj_set_style_pad_all(list, 0, 0);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);
  choreList_ = (_lv_obj_t*)list;

  for (int i = 0; i < kMaxChoreRows; ++i) {
    lv_obj_t* row = lv_btn_create(list);
    lv_obj_set_size(row, 588, 56);
    lv_obj_set_pos(row, 0, i * 64);
    lv_obj_set_style_radius(row, 8, 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x132B18), 0);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x123F1A), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0x1F6B2D), 0);
    lv_obj_add_event_cb(row, choreRowCb, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* rl = lv_label_create(row);
    lv_obj_set_width(rl, 552);
    lv_label_set_long_mode(rl, LV_LABEL_LONG_DOT);
    lv_obj_align(rl, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_set_style_text_color(rl, lv_color_hex(0xF2F5F0), 0);
    lv_obj_set_style_text_font(rl, &lv_font_montserrat_20, 0);

    choreRowBtns_[i] = (_lv_obj_t*)row;
    choreRowLabels_[i] = (_lv_obj_t*)rl;
  }

  buildChoreModal();

  lvglUpdateNav();
  displayUnlock();
}

// ---------------------------------------------------------------------------
// Chore-completion confirm modal + row interaction
// ---------------------------------------------------------------------------
void UiManager::buildChoreModal() {
  lv_obj_t* scr = lv_scr_act();

  // Full-screen dimmer that also blocks taps to the UI behind it.
  lv_obj_t* modal = lv_obj_create(scr);
  lv_obj_set_size(modal, PANEL_H_RES, PANEL_V_RES);
  lv_obj_set_pos(modal, 0, 0);
  lv_obj_set_style_bg_color(modal, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(modal, LV_OPA_70, 0);
  lv_obj_set_style_border_width(modal, 0, 0);
  lv_obj_set_style_radius(modal, 0, 0);
  lv_obj_clear_flag(modal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(modal, LV_OBJ_FLAG_CLICKABLE);   // swallow taps to background
  lv_obj_add_flag(modal, LV_OBJ_FLAG_HIDDEN);
  modal_ = (_lv_obj_t*)modal;

  lv_obj_t* panel = lv_obj_create(modal);
  lv_obj_set_size(panel, 620, 380);
  lv_obj_center(panel);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x0D2212), 0);
  lv_obj_set_style_border_width(panel, 2, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x1F6B2D), 0);
  lv_obj_set_style_radius(panel, 12, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* title = lv_label_create(panel);
  lv_obj_set_pos(title, 8, 4);
  lv_obj_set_width(title, 560);
  lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(title, lv_color_hex(0x00FF41), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  modalTitle_ = (_lv_obj_t*)title;

  lv_obj_t* body = lv_label_create(panel);
  lv_obj_set_pos(body, 8, 96);
  lv_obj_set_width(body, 560);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(body, lv_color_hex(0xF2F5F0), 0);
  lv_obj_set_style_text_font(body, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_line_space(body, 6, 0);
  modalBody_ = (_lv_obj_t*)body;

  lv_obj_t* status = lv_label_create(panel);
  lv_obj_set_pos(status, 8, 214);
  lv_obj_set_width(status, 560);
  lv_label_set_long_mode(status, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(status, lv_color_hex(0x8AAB8E), 0);
  lv_obj_set_style_text_font(status, &lv_font_montserrat_20, 0);
  modalStatus_ = (_lv_obj_t*)status;

  lv_obj_t* secondary = lv_btn_create(panel);
  lv_obj_set_size(secondary, 260, 66);
  lv_obj_align(secondary, LV_ALIGN_BOTTOM_LEFT, 4, -6);
  lv_obj_set_style_radius(secondary, 10, 0);
  lv_obj_set_style_bg_color(secondary, lv_color_hex(0x132B18), 0);
  lv_obj_set_style_border_width(secondary, 1, 0);
  lv_obj_set_style_border_color(secondary, lv_color_hex(0x1F6B2D), 0);
  lv_obj_add_event_cb(secondary, modalSecondaryCb, LV_EVENT_CLICKED, this);
  modalSecondaryBtn_ = (_lv_obj_t*)secondary;
  lv_obj_t* sl = lv_label_create(secondary);
  lv_label_set_text(sl, "Close");
  lv_obj_set_style_text_color(sl, lv_color_hex(0xF2F5F0), 0);
  lv_obj_set_style_text_font(sl, &lv_font_montserrat_20, 0);
  lv_obj_center(sl);
  modalSecondaryLabel_ = (_lv_obj_t*)sl;

  lv_obj_t* primary = lv_btn_create(panel);
  lv_obj_set_size(primary, 260, 66);
  lv_obj_align(primary, LV_ALIGN_BOTTOM_RIGHT, -4, -6);
  lv_obj_set_style_radius(primary, 10, 0);
  lv_obj_set_style_bg_color(primary, lv_color_hex(0x123F1A), 0);
  lv_obj_set_style_border_width(primary, 1, 0);
  lv_obj_set_style_border_color(primary, lv_color_hex(0x00FF41), 0);
  lv_obj_add_event_cb(primary, modalPrimaryCb, LV_EVENT_CLICKED, this);
  modalPrimaryBtn_ = (_lv_obj_t*)primary;
  lv_obj_t* pl = lv_label_create(primary);
  lv_label_set_text(pl, "Mark Complete");
  lv_obj_set_style_text_color(pl, lv_color_hex(0x00FF41), 0);
  lv_obj_set_style_text_font(pl, &lv_font_montserrat_20, 0);
  lv_obj_center(pl);
  modalPrimaryLabel_ = (_lv_obj_t*)pl;
}

void UiManager::hideChoreRows() {
  if (choreList_) lv_obj_add_flag((lv_obj_t*)choreList_, LV_OBJ_FLAG_HIDDEN);
}

void UiManager::applyPendingChoreResult() {
  if (!choreResultPending_ || !modal_) return;
  choreResultPending_ = false;
  if (choreResultOk_) {
    lv_label_set_text((lv_obj_t*)modalStatus_, "Chore completed");
    lv_obj_set_style_text_color((lv_obj_t*)modalStatus_, lv_color_hex(0x00FF41), 0);
    closeChoreModal();
    return;
  }
  modalMode_ = 2;
  lv_label_set_text((lv_obj_t*)modalStatus_,
                    choreResultOffline_ ? "Server offline - try again"
                                        : "Unable to complete chore");
  lv_obj_set_style_text_color((lv_obj_t*)modalStatus_, lv_color_hex(0xFF4444), 0);
  refreshModalForMode();
}

void UiManager::populateChoreRows(JsonArrayConst chores) {
  if (!choreList_) return;
  lv_obj_clear_flag((lv_obj_t*)choreList_, LV_OBJ_FLAG_HIDDEN);
  int n = 0;
  if (!chores.isNull()) {
    for (JsonObjectConst c : chores) {
      if (n >= kMaxChoreRows) break;
      const char* id = c["id"] | "";
      const char* title = c["title"] | "";
      if (!id || !*id) continue;
      choreRowIds_[n] = id;
      String rowText = String(title);
      const char* who = c["assignee"]["name"] | "";
      if (who && *who) { rowText += "  -  "; rowText += who; }
      lv_label_set_text((lv_obj_t*)choreRowLabels_[n], rowText.c_str());
      lv_obj_clear_flag((lv_obj_t*)choreRowBtns_[n], LV_OBJ_FLAG_HIDDEN);
      ++n;
    }
  }
  for (int i = n; i < kMaxChoreRows; ++i) {
    lv_obj_add_flag((lv_obj_t*)choreRowBtns_[i], LV_OBJ_FLAG_HIDDEN);
    choreRowIds_[i] = "";
  }
  choreRowCount_ = n;
}

void UiManager::handleChoreRowTap(_lv_obj_t* target) {
  if (modalMode_ == 3) return;   // in-flight — ignore
  for (int i = 0; i < kMaxChoreRows; ++i) {
    if (choreRowBtns_[i] == target) { openChoreDetail(i); return; }
  }
}

void UiManager::openChoreDetail(int rowIndex) {
  if (rowIndex < 0 || rowIndex >= choreRowCount_) return;
  pendingChoreId_ = choreRowIds_[rowIndex];
  if (pendingChoreId_.length() == 0) return;
  const char* txt = lv_label_get_text((lv_obj_t*)choreRowLabels_[rowIndex]);
  pendingChoreTitle_ = txt ? txt : "this chore";
  int cut = pendingChoreTitle_.indexOf("  -  ");
  if (cut > 0) pendingChoreTitle_ = pendingChoreTitle_.substring(0, cut);
  modalMode_ = 1;
  lv_label_set_text((lv_obj_t*)modalStatus_, "");
  lv_obj_set_style_text_color((lv_obj_t*)modalStatus_, lv_color_hex(0x8AAB8E), 0);
  refreshModalForMode();
  lv_obj_clear_flag((lv_obj_t*)modal_, LV_OBJ_FLAG_HIDDEN);
}

void UiManager::closeChoreModal() {
  modalMode_ = 0;
  pendingChoreId_ = "";
  pendingChoreTitle_ = "";
  if (modal_) lv_obj_add_flag((lv_obj_t*)modal_, LV_OBJ_FLAG_HIDDEN);
}

void UiManager::refreshModalForMode() {
  lv_obj_t* primary = (lv_obj_t*)modalPrimaryBtn_;
  if (modalMode_ == 1) {                      // read-only detail
    lv_label_set_text((lv_obj_t*)modalTitle_, pendingChoreTitle_.c_str());
    lv_label_set_text((lv_obj_t*)modalBody_,
                      "Chore details are managed in the Family Hub web app.\n"
                      "You can mark this chore complete here.");
    lv_label_set_text((lv_obj_t*)modalPrimaryLabel_, "Mark Complete");
    lv_label_set_text((lv_obj_t*)modalSecondaryLabel_, "Close");
    lv_obj_clear_state(primary, LV_STATE_DISABLED);
  } else if (modalMode_ == 2) {               // confirm
    lv_label_set_text((lv_obj_t*)modalTitle_, "Mark complete?");
    lv_label_set_text((lv_obj_t*)modalBody_,
                      (String("Mark \"") + pendingChoreTitle_ + "\" complete?").c_str());
    lv_label_set_text((lv_obj_t*)modalPrimaryLabel_, "Confirm");
    lv_label_set_text((lv_obj_t*)modalSecondaryLabel_, "Cancel");
    lv_obj_clear_state(primary, LV_STATE_DISABLED);
  } else if (modalMode_ == 3) {               // in-flight
    lv_label_set_text((lv_obj_t*)modalPrimaryLabel_, "Completing...");
    lv_label_set_text((lv_obj_t*)modalStatus_, "Contacting server...");
    lv_obj_set_style_text_color((lv_obj_t*)modalStatus_, lv_color_hex(0x8AAB8E), 0);
    lv_obj_add_state(primary, LV_STATE_DISABLED);
  }
}

void UiManager::handleModalPrimary() {
  if (modalMode_ == 1) {                       // detail -> confirm
    modalMode_ = 2;
    refreshModalForMode();
  } else if (modalMode_ == 2) {                // confirm -> dispatch
    modalMode_ = 3;
    refreshModalForMode();
    choreCompleteRequested_ = true;            // loop() performs the HTTP
  }
  // modalMode_ == 3: already dispatched — ignore duplicate taps
}

void UiManager::handleModalSecondary() {
  if (modalMode_ == 3) return;                 // can't cancel mid-request
  closeChoreModal();
}

void UiManager::handleNavTarget(_lv_obj_t* target) {
  for (int i = 0; i < 6; ++i) {
    if (navButtons_[i] == target) {
      current_ = (ScreenId)i;
      renderRequested_ = true;
      lvglUpdateNav();
      return;
    }
  }
}

void UiManager::lvglUpdateNav() {
  for (int i = 0; i < 6; ++i) {
    if (!navButtons_[i]) continue;
    bool active = (i == static_cast<int>(current_));
    lv_obj_set_style_bg_color((lv_obj_t*)navButtons_[i],
                              lv_color_hex(active ? 0x123F1A : 0x132B18), 0);
    lv_obj_set_style_border_color((lv_obj_t*)navButtons_[i],
                                  lv_color_hex(active ? 0x00FF41 : 0x1F6B2D), 0);
    if (navLabels_[i]) {
      lv_obj_set_style_text_color((lv_obj_t*)navLabels_[i],
                                  lv_color_hex(active ? 0x00FF41 : 0x8AAB8E), 0);
    }
  }
}

void UiManager::lvglLayoutCard(int index, int x, int y, int w, int h, bool bodyLarge) {
  if (index < 0 || index >= 4 || !cards_[index]) return;
  lv_obj_t* card = (lv_obj_t*)cards_[index];
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);

  lv_obj_t* title = (lv_obj_t*)cardTitles_[index];
  lv_obj_t* body = (lv_obj_t*)cardBodies_[index];
  lv_obj_set_pos(title, 18, 14);
  lv_obj_set_width(title, w - 36);
  lv_obj_set_pos(body, 18, bodyLarge ? 52 : 46);
  lv_obj_set_width(body, w - 36);
  lv_obj_set_height(body, h - (bodyLarge ? 66 : 58));
  lv_obj_set_style_text_font(body, bodyLarge ? &lv_font_montserrat_28 : &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_line_space(body, bodyLarge ? 6 : 4, 0);
}

void UiManager::lvglSetCard(int index, const char* title, const String& body, bool visible) {
  if (index < 0 || index >= 4 || !cards_[index]) return;
  lv_obj_t* card = (lv_obj_t*)cards_[index];
  if (!visible) {
    lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(card, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text((lv_obj_t*)cardTitles_[index], title ? title : "");
  lv_label_set_text((lv_obj_t*)cardBodies_[index], body.c_str());
}

void UiManager::lvglUpdate(const PanelStatus& status, const JsonDocument& state, bool stale) {
  lvglEnsureScreen();
  displayLock();
  applyPendingChoreResult();

  const char* b = badgeLabel(status, stale);
  const uint32_t stateColor = badgeColor(b);
  lv_obj_set_style_bg_color((lv_obj_t*)statusDot_, lv_color_hex(stateColor), 0);
  lv_obj_set_style_text_color((lv_obj_t*)statusText_, lv_color_hex(stateColor), 0);
  lv_label_set_text((lv_obj_t*)statusText_, b);

  lv_label_set_text((lv_obj_t*)pageTitle_, screenTitle(current_));
  String meta = formatLastSync(status);
  if (current_ == ScreenId::Settings) {
    meta += "   ";
    meta += status.serverHost;
    meta += ":";
    meta += status.serverPort;
  }
  lv_label_set_text((lv_obj_t*)pageMeta_, meta.c_str());
  lvglUpdateNav();

  for (int i = 0; i < 4; ++i) lvglSetCard(i, "", "", false);
  hideChoreRows();   // shown only on the Chores page below

  if (state.isNull() || state.size() == 0) {
    lvglLayoutCard(0, 24, 126, 620, 184, true);
    lvglLayoutCard(1, 668, 126, 332, 116);
    lvglLayoutCard(2, 668, 258, 332, 116);
    lvglLayoutCard(3, 24, 330, 976, 170);
    lvglSetCard(0, "Waiting for Family Hub",
                "No family info loaded yet.\nMake sure the hub is running,\nthen tap Sync.");
    lvglSetCard(1, "What to try", "Tap Sync\nor check Setup for details");
    lvglSetCard(2, "Status", String(b));
    lvglSetCard(3, "Last update", formatLastSync(status));
  } else {
    JsonArrayConst chores = state["chores"].as<JsonArrayConst>();
    JsonArrayConst grocery = state["grocery"].as<JsonArrayConst>();
    JsonArrayConst notes = state["notes"].as<JsonArrayConst>();
    JsonArrayConst pinned = state["pinnedNotes"].as<JsonArrayConst>();
    JsonArrayConst week = state["weekDinner"].as<JsonArrayConst>();
    const char* meal = state["dinner"]["meal"] | "No dinner planned";

    switch (current_) {
      case ScreenId::Home:
        // Keep Home on the same two-column grid as the other pages. The old
        // full-width bottom card visually cut across the right-side status
        // stack on the physical panel.
        lvglLayoutCard(0, 24, 126, 620, 374, true);
        lvglLayoutCard(1, 668, 126, 332, 116);
        lvglLayoutCard(2, 668, 258, 332, 116);
        lvglLayoutCard(3, 668, 390, 332, 110);
        lvglSetCard(0, "Tonight's Dinner",
                    String(meal) + "\nCook: " + (state["dinner"]["cook"]["name"] | "Unassigned"));
        lvglSetCard(1, "Today",
                    String(jsonArraySize(chores)) + " open chores\n" +
                    jsonArraySize(grocery) + " grocery items");
        lvglSetCard(2, "Updated",
                    formatLastSync(status));
        lvglSetCard(3, "Next Up", homeNext(chores, grocery, pinned));
        break;
      case ScreenId::Grocery:
        lvglLayoutCard(0, 24, 126, 620, 374);
        lvglLayoutCard(1, 668, 126, 332, 116);
        lvglLayoutCard(2, 668, 258, 332, 116);
        lvglLayoutCard(3, 668, 390, 332, 110);
        lvglSetCard(0, "Current List", listItems(grocery, "text", 8, "No groceries"));
        lvglSetCard(1, "List Count", String(jsonArraySize(grocery)) + " items");
        lvglSetCard(2, "Updated", formatLastSync(status));
        lvglSetCard(3, "Manage", "Add or edit items in the\nFamily Hub web app");
        break;
      case ScreenId::Chores:
        // Card 0's area is used by the tappable chore-row list instead of a
        // static label. Tap a row -> read-only detail -> Mark Complete.
        populateChoreRows(chores);
        lvglLayoutCard(1, 668, 126, 332, 116);
        lvglLayoutCard(2, 668, 258, 332, 116);
        lvglLayoutCard(3, 668, 390, 332, 110);
        lvglSetCard(1, "Open", String(jsonArraySize(chores)) + " chores");
        lvglSetCard(2, "Updated", formatLastSync(status));
        lvglSetCard(3, "Tap a chore", "Tap to view and mark complete.\nEdit & assign in the web app");
        break;
      case ScreenId::Dinner:
        lvglLayoutCard(0, 24, 126, 620, 160, true);
        lvglLayoutCard(1, 24, 306, 620, 194);
        lvglLayoutCard(2, 668, 126, 332, 160);
        lvglLayoutCard(3, 668, 306, 332, 194);
        lvglSetCard(0, "Tonight", String(meal));
        lvglSetCard(1, "Week Ahead", dinnerWeek(week, 5));
        lvglSetCard(2, "Updated", formatLastSync(status));
        lvglSetCard(3, "Manage", "Plan or change meals in the\nFamily Hub web app");
        break;
      case ScreenId::Notes:
        lvglLayoutCard(0, 24, 126, 620, 176);
        lvglLayoutCard(1, 24, 322, 620, 178);
        lvglLayoutCard(2, 668, 126, 332, 116);
        lvglLayoutCard(3, 668, 258, 332, 242);
        lvglSetCard(0, "Pinned", listItems(pinned, "text", 4, "No pinned notes"));
        lvglSetCard(1, "Recent Notes", listItems(notes, "text", 4, "No notes yet"));
        lvglSetCard(2, "Updated", formatLastSync(status));
        lvglSetCard(3, "Manage", "Add or edit notes in the\nFamily Hub web app");
        break;
      case ScreenId::Settings:
        lvglLayoutCard(0, 24, 126, 476, 178);
        lvglLayoutCard(1, 524, 126, 476, 178);
        lvglLayoutCard(2, 24, 324, 476, 176);
        lvglLayoutCard(3, 524, 324, 476, 176);
        lvglSetCard(0, "Connection",
                    String(b) + "\nRSSI " + status.wifiRssi + " dBm\n" + formatLastSync(status));
        lvglSetCard(1, "Server",
                    String(status.serverHost) + ":" + status.serverPort +
                    "\nHTTP dashboard preload");
        lvglSetCard(2, "Device",
                    String(status.deviceId) + "\nFirmware " + status.firmwareVersion);
        lvglSetCard(3, "Runtime Mode", "RGB display active\nNetwork writes deferred\nCache writes deferred");
        break;
    }
  }

  // Toast for recent write result
  if (toast_) {
    if (lastWriteMs_ && millis() - lastWriteMs_ < 8000) {
      String t = String(lastWriteOk_ ? "OK " : "FAILED ") + lastWriteAction_;
      lv_label_set_text((lv_obj_t*)toast_, t.c_str());
      lv_obj_set_style_text_color((lv_obj_t*)toast_,
          lv_color_hex(lastWriteOk_ ? 0xA3BE8C : 0xBF616A), 0);
      lv_obj_clear_flag((lv_obj_t*)toast_, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag((lv_obj_t*)toast_, LV_OBJ_FLAG_HIDDEN);
    }
  }
  displayUnlock();
}

#endif

void UiManager::resetLvglPointers() {
#ifndef FAMILY_HUB_APP_CHILD
  headerBar_=brandTitle_=brandSub_=syncButton_=syncLabel_=statusPill_=statusDot_=statusText_=nullptr;
#if !defined(FAMILY_HUB_APP_HOUSEHOLD)
  childModeButton_=nullptr;
#endif
  pageTitle_=pageMeta_=toast_=choreList_=modal_=modalTitle_=modalBody_=modalStatus_=nullptr;
  modalPrimaryBtn_=modalPrimaryLabel_=modalSecondaryBtn_=modalSecondaryLabel_=nullptr;
  for(int i=0;i<4;++i) cards_[i]=cardTitles_[i]=cardBodies_[i]=nullptr;
  for(int i=0;i<6;++i) navButtons_[i]=navLabels_[i]=nullptr;
  for(int i=0;i<kMaxChoreRows;++i) choreRowBtns_[i]=choreRowLabels_[i]=nullptr;
  modalMode_=0;
#endif
#ifndef FAMILY_HUB_APP_HOUSEHOLD
  for(int i=0;i<kMaxChildProfiles;++i){childProfileBtns_[i]=nullptr;childProfileIds_[i]="";}
  for(int i=0;i<kMaxChildTasks;++i){childTaskBtns_[i]=nullptr;childTaskIds_[i]="";}
  for(int i=0;i<kMaxChildRewards;++i){childRewardBtns_[i]=nullptr;childRewardIds_[i]="";childRewardGoalIds_[i]="";childRewardTypes_[i]="";}
  for(int i=0;i<12;++i){childPinBtns_[i]=nullptr;childPinKeys_[i]="";}
  for(int i=0;i<4;++i){childNavBtns_[i]=nullptr;childNavKinds_[i]="";}
  childExitBtn_=childPinMask_=nullptr;childProfileCount_=childTaskCount_=childRewardCount_=childPinCount_=childNavCount_=0;
#endif
}

#ifndef FAMILY_HUB_APP_HOUSEHOLD
String UiManager::buildChildRenderFingerprint(const JsonDocument& state, const ChildFocusRuntime& runtime,
                                              bool selection) const {
  String fp;
  fp.reserve(160);
  fp += selection ? 'S' : 'M';
  fp += ':';
  fp += (int)runtime.page;
  fp += ':';
  fp += (int)childDashboardTab_;
  fp += ':';
  fp += runtime.stale ? '1' : '0';
  fp += ':';
  fp += runtime.selectedTargetId;
  fp += ':';
  fp += state["focus"]["type"] | "";
  if (lastWriteMs_ && millis() - lastWriteMs_ < 8000) {
    fp += ':';
    fp += lastWriteOk_ ? 'O' : 'F';
    fp += lastWriteAction_;
  }
  if (!selection) {
    fp += ':';
    fp += (int)(state["starBalance"].isNull() ? -1 : (int)state["starBalance"]);
    fp += ':';
    fp += (int)(state["daily"]["progress"] | state["daily"]["current"] | 0);
    fp += ':';
    fp += (int)(state["term"]["successfulCredit"] | state["term"]["progress"]["successfulDays"] |
                state["term"]["current"] | 0);
    for (JsonObjectConst task : state["tasks"].as<JsonArrayConst>()) {
      fp += task["assignmentId"].as<const char*>();
      fp += '=';
      fp += task["completion"]["status"].as<const char*>();
      fp += ';';
    }
    for (JsonObjectConst routine : state["routines"].as<JsonArrayConst>()) {
      fp += routine["label"].as<const char*>();
      fp += '=';
      fp += String((int)(routine["done"] | 0));
      fp += '/';
      fp += String((int)(routine["total"] | 0));
      fp += ';';
    }
  }
  fp += ':';
  fp += childPinValue_;
  return fp;
}

static const uint32_t kChildBg = 0xFFF6E8;
static const uint32_t kChildText = 0x2D3436;
static const uint32_t kChildCardBlue = 0x5B9BD5;
static const uint32_t kChildCardGreen = 0x6BCB77;
static const uint32_t kChildCardPurple = 0x9B7EDE;
static const uint32_t kChildCardOrange = 0xFFB347;
static const uint32_t kChildCardPink = 0xFF8FAB;

static lv_obj_t* focusLabel(lv_obj_t* parent, const char* value, int x, int y, int width,
                            const lv_font_t* font, uint32_t color,
                            lv_text_align_t align = LV_TEXT_ALIGN_LEFT) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, value ? value : "");
  lv_obj_set_pos(label, x, y);
  lv_obj_set_width(label, width);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
  lv_obj_set_style_text_align(label, align, 0);
  return label;
}

static lv_obj_t* focusCard(lv_obj_t* parent, int x, int y, int w, int h, uint32_t color) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);
  lv_obj_set_style_bg_color(card, lv_color_hex(color), 0);
  lv_obj_set_style_border_width(card, 4, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_radius(card, 28, 0);
  lv_obj_set_style_shadow_width(card, 12, 0);
  lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  return card;
}

static lv_obj_t* childUiScrollPanel(lv_obj_t* parent, int x, int y, int w, int h) {
  lv_obj_t* panel = lv_obj_create(parent);
  lv_obj_set_pos(panel, x, y);
  lv_obj_set_size(panel, w, h);
  lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(panel, 0, 0);
  lv_obj_set_style_pad_all(panel, 0, 0);
  lv_obj_set_style_radius(panel, 0, 0);
  lv_obj_set_scroll_dir(panel, LV_DIR_VER);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLL_ELASTIC);
  return panel;
}

static void childUiStyleBtn(lv_obj_t* btn, uint32_t color, int radius = 28) {
  lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
  lv_obj_set_style_radius(btn, radius, 0);
  lv_obj_set_style_border_width(btn, 4, 0);
  lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_shadow_width(btn, 10, 0);
  lv_obj_set_style_shadow_opa(btn, LV_OPA_20, 0);
}

static bool childUiIsToddler(const JsonDocument& state) {
  const char* preset = state["config"]["preset"] | "toddler";
  return strcmp(preset, "independent") != 0;
}

static uint32_t childUiParseColor(const char* hex, uint32_t fallback) {
  if (!hex || hex[0] != '#' || strlen(hex) < 7) return fallback;
  char buf[8] = {0};
  strncpy(buf, hex + 1, 6);
  return (uint32_t)strtoul(buf, nullptr, 16);
}

static uint32_t childUiAccent(JsonObjectConst child) {
  return childUiParseColor(child["displayColor"] | "#6EC6FF", 0x6EC6FF);
}

static String childUiAvatar(JsonObjectConst child) {
  String name = child["displayName"].as<String>();
  if (name.length()) {
    char letter = (char)toupper(name.charAt(0));
    return String(letter);
  }
  return String("?");
}

static char childUiTaskLetter(JsonObjectConst task, int index) {
  String label = task["label"] | "";
  if (label.length()) return (char)toupper(label.charAt(0));
  static const char fallback[] = {'T', 'O', 'Y', 'R', 'A', 'B'};
  return fallback[index % 6];
}

static String childUiTruncate(const String& text, int maxLen) {
  if ((int)text.length() <= maxLen) return text;
  return text.substring(0, maxLen);
}

static String childUiShortLabel(const char* label, int maxLen = 12) {
  if (!label || !label[0]) return String("Task");
  return childUiTruncate(String(label), maxLen);
}

static void childUiDrawProgressRow(lv_obj_t* parent, int x, int y, int done, int total, int maxShow = 8,
                                   int maxWidth = 0) {
  int t = total > 0 ? total : 5;
  if (t > maxShow) t = maxShow;
  const int dotSize = 26;
  const int gap = 10;
  if (maxWidth > 0) {
    const int fit = (maxWidth + gap) / (dotSize + gap);
    if (fit > 0 && t > fit) t = fit;
  }
  int d = done < 0 ? 0 : (done > t ? t : done);
  for (int i = 0; i < t; ++i) {
    lv_obj_t* dot = lv_obj_create(parent);
    lv_obj_set_pos(dot, x + i * (dotSize + gap), y);
    lv_obj_set_size(dot, dotSize, dotSize);
    lv_obj_set_style_radius(dot, dotSize / 2, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(i < d ? 0x9B7EDE : 0xDFE6E9), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
  }
}

static void childUiDrawStarBadge(lv_obj_t* parent, int x, int y, int balance) {
  lv_obj_t* badge = lv_obj_create(parent);
  lv_obj_set_pos(badge, x, y);
  lv_obj_set_size(badge, 110, 52);
  lv_obj_set_style_radius(badge, 26, 0);
  lv_obj_set_style_bg_color(badge, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(badge, LV_OPA_60, 0);
  lv_obj_set_style_border_width(badge, 0, 0);
  lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* star = lv_obj_create(badge);
  lv_obj_set_pos(star, 8, 10);
  lv_obj_set_size(star, 32, 32);
  lv_obj_set_style_radius(star, 16, 0);
  lv_obj_set_style_bg_color(star, lv_color_hex(0xFFD166), 0);
  lv_obj_set_style_border_width(star, 0, 0);
  focusLabel(badge, String(balance >= 0 ? balance : 0).c_str(), 48, 10, 56, &lv_font_montserrat_28, 0xFFFFFF,
             LV_TEXT_ALIGN_LEFT);
}

static String childUiStarLine(int balance, bool visualOnly) {
  if (!visualOnly) return String(balance) + " stars";
  String s;
  const int cap = 5;
  int show = balance < 0 ? 0 : (balance > cap ? cap : balance);
  for (int i = 0; i < cap; ++i) s += (i < show) ? "* " : "- ";
  return s;
}

static void childUiStatusDot(lv_obj_t* scr, bool stale) {
  lv_obj_t* dot = lv_obj_create(scr);
  lv_obj_set_size(dot, 20, 20);
  lv_obj_set_pos(dot, 24, 18);
  lv_obj_set_style_radius(dot, 10, 0);
  lv_obj_set_style_border_width(dot, 0, 0);
  lv_obj_set_style_bg_color(dot, lv_color_hex(stale ? 0xFFB347 : 0x6BCB77), 0);
}

static void childUiPlacePhoto(lv_obj_t* parent, const PanelImage* image, int x, int y, int size) {
  if (!image || !image->pixels) return;
  lv_obj_t* img = lv_img_create(parent);
  lv_img_set_src(img, &image->descriptor);
  lv_obj_set_pos(img, x, y);
  lv_obj_set_size(img, size, size);
  lv_img_set_zoom(img, (uint16_t)((size * 256) / 128));
}

static void childUiPhotoOrPlaceholder(lv_obj_t* parent, const PanelImage* image, int x, int y, int size) {
  if (image && image->pixels) {
    childUiPlacePhoto(parent, image, x, y, size);
    return;
  }
  lv_obj_t* slot = lv_obj_create(parent);
  lv_obj_set_pos(slot, x, y);
  lv_obj_set_size(slot, size, size);
  lv_obj_set_style_radius(slot, size / 5, 0);
  lv_obj_set_style_bg_color(slot, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_bg_opa(slot, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(slot, 3, 0);
  lv_obj_set_style_border_color(slot, lv_color_hex(0xB8C5D6), 0);
  lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
  focusLabel(slot, LV_SYMBOL_IMAGE, 0, (size - 28) / 2, size, &lv_font_montserrat_28, 0x5B9BD5,
             LV_TEXT_ALIGN_CENTER);
}

static void childUiLetterTile(lv_obj_t* parent, int x, int y, int size, char letter, uint32_t color) {
  lv_obj_t* slot = lv_obj_create(parent);
  lv_obj_set_pos(slot, x, y);
  lv_obj_set_size(slot, size, size);
  lv_obj_set_style_radius(slot, size / 4, 0);
  lv_obj_set_style_bg_color(slot, lv_color_hex(color), 0);
  lv_obj_set_style_border_width(slot, 3, 0);
  lv_obj_set_style_border_color(slot, lv_color_hex(0xFFFFFF), 0);
  lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
  char buf[2] = {letter, '\0'};
  focusLabel(slot, buf, 0, (size - 36) / 2, size, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
}

static lv_obj_t* childUiBackButton(lv_obj_t* scr, UiManager* ui, bool toddler, int y = 510) {
  lv_obj_t* back = lv_btn_create(scr);
  lv_obj_set_pos(back, 24, y);
  lv_obj_set_size(back, toddler ? 120 : 180, 72);
  childUiStyleBtn(back, 0xDFE6E9, 36);
  lv_obj_add_event_cb(back, childTaskCb, LV_EVENT_CLICKED, ui);
  focusLabel(back, toddler ? LV_SYMBOL_LEFT : "Back", 0, 22, toddler ? 120 : 180, &lv_font_montserrat_28,
             kChildText, LV_TEXT_ALIGN_CENTER);
  return back;
}

void UiManager::addChildTaskLink(lv_obj_t* parent, int x, int y, int w, int h, const char* kind) {
  if (childTaskCount_ >= kMaxChildTasks) return;
  lv_obj_t* tap = lv_btn_create(parent);
  lv_obj_set_pos(tap, x, y);
  lv_obj_set_size(tap, w, h);
  lv_obj_set_style_bg_opa(tap, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(tap, 0, 0);
  lv_obj_add_event_cb(tap, childTaskCb, LV_EVENT_CLICKED, this);
  childTaskBtns_[childTaskCount_] = (_lv_obj_t*)tap;
  childTaskIds_[childTaskCount_] = kind;
  ++childTaskCount_;
}

void UiManager::buildChildTabBar(lv_obj_t* scr, ChildDashboardTab active) {
  const int y = kChildNavBarY;
  const int h = kChildNavBarH;
  lv_obj_t* bar = lv_obj_create(scr);
  lv_obj_set_pos(bar, 0, y);
  lv_obj_set_size(bar, PANEL_H_RES, h);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0xF0E6FF), 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_set_style_pad_all(bar, 0, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  struct TabItem { const char* kind; const char* icon; const char* label; };
  const TabItem items[] = {
      {"tab-home", LV_SYMBOL_HOME, "Home"},
      {"tab-tasks", LV_SYMBOL_LIST, "Tasks"},
      {"tab-routines", LV_SYMBOL_REFRESH, "Routines"},
      {"tab-rewards", LV_SYMBOL_IMAGE, "Rewards"},
  };
  const int count = 4;
  const int gap = 12;
  const int btnW = (PANEL_H_RES - 32 - gap * (count - 1)) / count;
  for (int i = 0; i < count; ++i) {
    const bool selected = (static_cast<int>(active) == i);
    lv_obj_t* b = lv_btn_create(bar);
    lv_obj_set_pos(b, 16 + i * (btnW + gap), 8);
    lv_obj_set_size(b, btnW, h - 16);
    childUiStyleBtn(b, selected ? 0x9B7EDE : 0xFFFFFF, 28);
    lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(b, childNavCb, LV_EVENT_CLICKED, this);
    childNavBtns_[i] = (_lv_obj_t*)b;
    childNavKinds_[i] = items[i].kind;
    const uint32_t fg = selected ? 0xFFFFFF : kChildText;
    focusLabel(b, items[i].icon, 0, 8, btnW, &lv_font_montserrat_28, fg, LV_TEXT_ALIGN_CENTER);
    focusLabel(b, items[i].label, 0, 38, btnW, &lv_font_montserrat_20, fg, LV_TEXT_ALIGN_CENTER);
  }
  childNavCount_ = count;
  lv_obj_move_foreground(bar);
}

static void childUiBuildDashboardHeader(lv_obj_t* scr, JsonObjectConst child, int starBalance,
                                        const PanelImage* profileImage, UiManager* ui, bool toddler) {
  const uint32_t accent = childUiAccent(child);
  lv_obj_t* bar = lv_obj_create(scr);
  lv_obj_set_pos(bar, 0, 0);
  lv_obj_set_size(bar, PANEL_H_RES, 88);
  lv_obj_set_style_bg_color(bar, lv_color_hex(accent), 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* avatar = lv_obj_create(bar);
  lv_obj_set_pos(avatar, 20, 12);
  lv_obj_set_size(avatar, 64, 64);
  lv_obj_set_style_radius(avatar, 32, 0);
  lv_obj_set_style_bg_color(avatar, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(avatar, 3, 0);
  lv_obj_set_style_border_color(avatar, lv_color_hex(0xFFFFFF), 0);
  if (profileImage && profileImage->pixels) {
    childUiPlacePhoto(avatar, profileImage, 4, 4, 56);
  } else {
    focusLabel(avatar, childUiAvatar(child).c_str(), 0, 14, 64, &lv_font_montserrat_36, accent,
               LV_TEXT_ALIGN_CENTER);
  }
  String name = child["displayName"] | "Friend";
  if (toddler) {
    int sp = name.indexOf(' ');
    if (sp > 0) name = name.substring(0, sp);
  }
  focusLabel(bar, name.c_str(), 100, 16, 420, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_LEFT);
  if (starBalance >= 0) childUiDrawStarBadge(bar, 560, 18, starBalance);
  lv_obj_t* childBtn = lv_btn_create(bar);
  lv_obj_set_pos(childBtn, 820, 14);
  lv_obj_set_size(childBtn, 60, 60);
  childUiStyleBtn(childBtn, 0xFFFFFF, 30);
  lv_obj_set_style_bg_opa(childBtn, LV_OPA_40, 0);
  lv_obj_add_event_cb(childBtn, childChangeChildCb, LV_EVENT_CLICKED, ui);
  focusLabel(childBtn, LV_SYMBOL_REFRESH, 0, 14, 60, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
  lv_obj_t* exitBtn = lv_btn_create(bar);
  lv_obj_set_pos(exitBtn, 900, 14);
  lv_obj_set_size(exitBtn, 60, 60);
  childUiStyleBtn(exitBtn, 0xFFFFFF, 30);
  lv_obj_set_style_bg_opa(exitBtn, LV_OPA_40, 0);
  lv_obj_add_event_cb(exitBtn, childExitCb, LV_EVENT_LONG_PRESSED, ui);
  focusLabel(exitBtn, LV_SYMBOL_SETTINGS, 0, 14, 60, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
}

static void childUiTaskBadge(lv_obj_t* parent, const char* symbol, uint32_t color) {
  lv_obj_t* badge = lv_obj_create(parent);
  lv_obj_set_size(badge, 36, 36);
  lv_obj_align(badge, LV_ALIGN_TOP_RIGHT, -8, 8);
  lv_obj_set_style_radius(badge, 18, 0);
  lv_obj_set_style_bg_color(badge, lv_color_hex(color), 0);
  lv_obj_set_style_border_width(badge, 0, 0);
  focusLabel(badge, symbol, 0, 4, 36, &lv_font_montserrat_20, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
}

static void childUiRewardPathStrip(lv_obj_t* scr, int y, const JsonDocument& state, bool visualStars,
                                   const PanelImage* dailyImage, const PanelImage* termImage) {
  lv_obj_t* strip = focusCard(scr, 24, y, 976, 56, 0xFFFFFF);
  const int dailyProgress = (int)(state["daily"]["progress"] | state["daily"]["current"] | 0);
  const int successful = (int)(state["term"]["successfulCredit"] | state["term"]["progress"]["successfulDays"] |
                               state["term"]["current"] | 0);
  const int required = (int)(state["term"]["requiredSuccessfulDays"] |
                             state["term"]["progress"]["requiredSuccessfulDays"] | state["term"]["target"] | 10);
  lv_obj_t* dailyIcon = lv_obj_create(strip);
  lv_obj_set_pos(dailyIcon, 20, 8);
  lv_obj_set_size(dailyIcon, 40, 40);
  lv_obj_set_style_radius(dailyIcon, 20, 0);
  lv_obj_set_style_bg_color(dailyIcon, lv_color_hex(0xFFE8A3), 0);
  lv_obj_set_style_border_width(dailyIcon, 0, 0);
  childUiPhotoOrPlaceholder(dailyIcon, dailyImage, 2, 2, 36);
  childUiDrawProgressRow(strip, 88, 14, dailyProgress, 5, 5);
  focusLabel(strip, ">", 300, 12, 40, &lv_font_montserrat_28, kChildText, LV_TEXT_ALIGN_CENTER);
  lv_obj_t* termIcon = lv_obj_create(strip);
  lv_obj_set_pos(termIcon, 372, 8);
  lv_obj_set_size(termIcon, 40, 40);
  lv_obj_set_style_radius(termIcon, 20, 0);
  lv_obj_set_style_bg_color(termIcon, lv_color_hex(0xC8E6C9), 0);
  lv_obj_set_style_border_width(termIcon, 0, 0);
  childUiPhotoOrPlaceholder(termIcon, termImage, 2, 2, 36);
  childUiDrawProgressRow(strip, 428, 14, successful, required > 0 ? required : 10, 8, 520);
}

static void childUiHeader(lv_obj_t* scr, JsonObjectConst child, int starBalance, bool visualStars,
                          const PanelImage* profileImage, bool visualOnly) {
  const uint32_t accent = childUiAccent(child);
  lv_obj_t* bar = lv_obj_create(scr);
  lv_obj_set_pos(bar, 0, 0);
  lv_obj_set_size(bar, PANEL_H_RES, 108);
  lv_obj_set_style_bg_color(bar, lv_color_hex(accent), 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

  if (profileImage && profileImage->pixels) {
    childUiPlacePhoto(bar, profileImage, 20, 14, 80);
  } else {
    focusLabel(bar, childUiAvatar(child).c_str(), 28, 12, 90, &lv_font_montserrat_36, 0xFFFFFF,
               LV_TEXT_ALIGN_CENTER);
  }
  if (!visualOnly) {
    String greeting = String("Hi ") + (child["displayName"] | "friend") + "!";
    focusLabel(bar, greeting.c_str(), 118, 18, 520, &lv_font_montserrat_36, 0xFFFFFF);
  }
  if (starBalance >= 0) {
    focusLabel(bar, childUiStarLine(starBalance, visualStars).c_str(), visualOnly ? 118 : 118,
               visualOnly ? 34 : 62, 520, &lv_font_montserrat_28, 0xFFFFFF);
  }
}

void UiManager::rebuildChildScreen(const PanelStatus& status, const JsonDocument& state,
                                   const ChildFocusRuntime& runtime, bool selection) {
  (void)status;
  displayLock();
  lv_obj_t* scr = lv_scr_act();
  lv_obj_clean(scr);
  resetLvglPointers();
  renderedChildPage_ = runtime.page;
  const bool toddler = !selection && childUiIsToddler(state);
  const bool visualStars = strcmp(state["config"]["starCountStyle"] | "visual", "exact") != 0;
  const int starBalance = state["starBalance"].isNull() ? -1 : (int)state["starBalance"];

  lv_obj_set_style_bg_color(scr, lv_color_hex(kChildBg), 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  childUiStatusDot(scr, runtime.stale);

  if (runtime.page != ChildFocusPage::Pin && runtime.page != ChildFocusPage::Dashboard) {
    lv_obj_t* exit = lv_btn_create(scr);
    lv_obj_set_size(exit, 76, 76);
    lv_obj_align(exit, LV_ALIGN_TOP_RIGHT, -18, 14);
    childUiStyleBtn(exit, 0xDFE6E9, 38);
    lv_obj_add_event_cb(exit, childExitCb, LV_EVENT_LONG_PRESSED, this);
    childExitBtn_ = (_lv_obj_t*)exit;
    focusLabel(exit, LV_SYMBOL_HOME, 0, 22, 76, &lv_font_montserrat_28, kChildText, LV_TEXT_ALIGN_CENTER);
  }
  if (lastWriteMs_ && millis() - lastWriteMs_ < 8000 && !toddler) {
    focusLabel(scr, lastWriteOk_ ? "Nice job!" : "Try again", 300, 560, 424, &lv_font_montserrat_28,
               lastWriteOk_ ? 0x2E7D4F : 0xC0392B, LV_TEXT_ALIGN_CENTER);
  }

  if (runtime.page == ChildFocusPage::Pin) {
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x2D3436), 0);
    focusLabel(scr, "Grown-up PIN", 300, 78, 424, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    String masked;
    for (size_t n = 0; n < childPinValue_.length(); ++n) masked += "*";
    childPinMask_ = (_lv_obj_t*)focusLabel(scr, masked.c_str(), 350, 118, 324, &lv_font_montserrat_36,
                                           0xFFD166, LV_TEXT_ALIGN_CENTER);
    const char* keys[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "BACK", "0", "GO"};
    for (int i = 0; i < 12; ++i) {
      int col = i % 3;
      int row = i / 3;
      lv_obj_t* b = lv_btn_create(scr);
      lv_obj_set_pos(b, 310 + col * 140, 175 + row * 92);
      lv_obj_set_size(b, 120, 72);
      childUiStyleBtn(b, i == 11 ? 0x6BCB77 : 0x5B9BD5, 18);
      lv_obj_add_event_cb(b, childPinCb, LV_EVENT_CLICKED, this);
      childPinBtns_[i] = (_lv_obj_t*)b;
      childPinKeys_[i] = keys[i];
      focusLabel(b, keys[i], 0, 22, 120, &lv_font_montserrat_20, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    }
    childPinCount_ = 12;
    displayUnlock();
    return;
  }

  if (selection) {
    JsonArrayConst children = state["children"].as<JsonArrayConst>();
    const int count = children.isNull() ? 0 : (int)children.size();
    int i = 0;
    for (JsonObjectConst child : children) {
      if (i >= kMaxChildProfiles) break;
      if (count <= 2) {
        const int w = count == 1 ? 420 : 360;
        const int h = 400;
        lv_obj_t* btn = lv_btn_create(scr);
        lv_obj_set_size(btn, w, h);
        lv_obj_align(btn, count == 1 ? LV_ALIGN_CENTER : LV_ALIGN_TOP_MID,
                     count == 1 ? 0 : (i == 0 ? -200 : 200), count == 1 ? 40 : 90);
        childUiStyleBtn(btn, childUiAccent(child), 36);
        lv_obj_add_event_cb(btn, childProfileCb, LV_EVENT_CLICKED, this);
        childProfileBtns_[i] = (_lv_obj_t*)btn;
        childProfileIds_[i] = child["id"].as<String>();
        if (childProfileImages_[i] && childProfileImages_[i]->pixels) {
          childUiPlacePhoto(btn, childProfileImages_[i], (w - 180) / 2, 60, 180);
        } else {
          focusLabel(btn, childUiAvatar(child).c_str(), 0, 80, w, &lv_font_montserrat_36, 0xFFFFFF,
                     LV_TEXT_ALIGN_CENTER);
        }
        focusLabel(btn, LV_SYMBOL_OK, 0, 320, w, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
      } else if (count == 3) {
        const int w = 280;
        const int h = 400;
        lv_obj_t* btn = lv_btn_create(scr);
        lv_obj_set_pos(btn, 72 + i * 320, 100);
        lv_obj_set_size(btn, w, h);
        childUiStyleBtn(btn, childUiAccent(child), 36);
        lv_obj_add_event_cb(btn, childProfileCb, LV_EVENT_CLICKED, this);
        childProfileBtns_[i] = (_lv_obj_t*)btn;
        childProfileIds_[i] = child["id"].as<String>();
        if (childProfileImages_[i] && childProfileImages_[i]->pixels) {
          childUiPlacePhoto(btn, childProfileImages_[i], (w - 180) / 2, 60, 180);
        } else {
          focusLabel(btn, childUiAvatar(child).c_str(), 0, 80, w, &lv_font_montserrat_36, 0xFFFFFF,
                     LV_TEXT_ALIGN_CENTER);
        }
        focusLabel(btn, LV_SYMBOL_OK, 0, 320, w, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
      } else {
        const int col = i % 2;
        const int row = i / 2;
        const int w = 460;
        const int h = 220;
        lv_obj_t* btn = lv_btn_create(scr);
        lv_obj_set_pos(btn, 32 + col * 500, 60 + row * 250);
        lv_obj_set_size(btn, w, h);
        childUiStyleBtn(btn, childUiAccent(child), 32);
        lv_obj_add_event_cb(btn, childProfileCb, LV_EVENT_CLICKED, this);
        childProfileBtns_[i] = (_lv_obj_t*)btn;
        childProfileIds_[i] = child["id"].as<String>();
        if (childProfileImages_[i] && childProfileImages_[i]->pixels) {
          childUiPlacePhoto(btn, childProfileImages_[i], 24, 24, 140);
        } else {
          focusLabel(btn, childUiAvatar(child).c_str(), 0, 24, 140, &lv_font_montserrat_36, 0xFFFFFF,
                     LV_TEXT_ALIGN_CENTER);
        }
      }
      ++i;
    }
    childProfileCount_ = i;
    displayUnlock();
    return;
  }

  if (runtime.page == ChildFocusPage::Goals) {
    JsonObjectConst childObj = state["child"].as<JsonObjectConst>();
    childUiHeader(scr, childObj, starBalance, visualStars, childHeaderImage_, toddler);
    const int successful = (int)(state["term"]["successfulCredit"] | state["term"]["progress"]["successfulDays"] |
                                 state["term"]["current"] | 0);
    const int required = (int)(state["term"]["requiredSuccessfulDays"] |
                               state["term"]["progress"]["requiredSuccessfulDays"] | state["term"]["target"] | 0);
    const int dailyProgress = (int)(state["daily"]["progress"] | state["daily"]["current"] | 0);
    lv_obj_t* hero = focusCard(scr, 72, 118, 880, 300, kChildCardGreen);
    if (childFocusImage_ && childFocusImage_->pixels) {
      lv_obj_t* image = lv_img_create(hero);
      lv_img_set_src(image, &childFocusImage_->descriptor);
      lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 24);
    } else {
      childUiPhotoOrPlaceholder(hero, nullptr, 340, 60, 200);
    }
    childUiDrawProgressRow(hero, 300, 220, successful, required > 0 ? required : 10, 8);
    lv_obj_t* daily = focusCard(scr, 72, 436, 420, 64, kChildCardBlue);
    focusLabel(daily, childUiStarLine(dailyProgress, visualStars).c_str(), 0, 16, 420, &lv_font_montserrat_28,
               0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    buildChildTabBar(scr, childDashboardTab_);
    displayUnlock();
    return;
  }

  if (runtime.page == ChildFocusPage::Waiting) {
    JsonObjectConst childObj = state["child"].as<JsonObjectConst>();
    childUiHeader(scr, childObj, starBalance, visualStars, childHeaderImage_, toddler);
    focusLabel(scr, LV_SYMBOL_BELL, 0, 150, PANEL_H_RES, &lv_font_montserrat_36, kChildText, LV_TEXT_ALIGN_CENTER);
    focusLabel(scr, LV_SYMBOL_OK, 0, 260, PANEL_H_RES, &lv_font_montserrat_36, kChildText, LV_TEXT_ALIGN_CENTER);
    focusLabel(scr, LV_SYMBOL_IMAGE, 0, 380, PANEL_H_RES, &lv_font_montserrat_36, kChildText, LV_TEXT_ALIGN_CENTER);
    childTaskBtns_[0] = (_lv_obj_t*)childUiBackButton(scr, this, toddler, 430);
    childTaskIds_[0] = "__back";
    childTaskCount_ = 1;
    displayUnlock();
    return;
  }

  if (runtime.page == ChildFocusPage::Celebrate) {
    JsonObjectConst childObj = state["child"].as<JsonObjectConst>();
    childUiHeader(scr, childObj, starBalance, visualStars, childHeaderImage_, toddler);
    focusLabel(scr, LV_SYMBOL_OK, 0, 140, PANEL_H_RES, &lv_font_montserrat_36, kChildText, LV_TEXT_ALIGN_CENTER);
    focusLabel(scr, childUiStarLine(starBalance >= 0 ? starBalance : 3, true).c_str(), 0, 300, PANEL_H_RES,
               &lv_font_montserrat_28, kChildText, LV_TEXT_ALIGN_CENTER);
    focusLabel(scr, LV_SYMBOL_PLAY, 0, 400, PANEL_H_RES, &lv_font_montserrat_36, kChildText, LV_TEXT_ALIGN_CENTER);
    lv_obj_t* dismiss = lv_btn_create(scr);
    lv_obj_set_size(dismiss, PANEL_H_RES, PANEL_V_RES);
    lv_obj_set_pos(dismiss, 0, 0);
    lv_obj_set_style_bg_opa(dismiss, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dismiss, 0, 0);
    lv_obj_add_event_cb(dismiss, childTaskCb, LV_EVENT_CLICKED, this);
    childTaskBtns_[0] = (_lv_obj_t*)dismiss;
    childTaskIds_[0] = "__back";
    childTaskCount_ = 1;
    displayUnlock();
    return;
  }

  if (runtime.page == ChildFocusPage::Treats) {
    JsonObjectConst childObj = state["child"].as<JsonObjectConst>();
    childUiHeader(scr, childObj, starBalance, visualStars, childHeaderImage_, toddler);
    const bool interactions = state["config"]["interactionsEnabled"] | false;
    const bool canSelect = interactions && (state["rewardActions"]["canSelectReward"] | false);
    int i = 0;
    for (JsonObjectConst reward : state["rewards"].as<JsonArrayConst>()) {
      if (i >= 2) break;
      if (!reward["goal"].isNull()) continue;
      const int x = i == 0 ? 72 : 552;
      lv_obj_t* card = focusCard(scr, x, 108, 400, 400, kChildCardPurple);
      focusLabel(card, LV_SYMBOL_IMAGE, 0, 12, 400, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
      childUiPhotoOrPlaceholder(card, childRewardImages_[i], 100, 80, 200);
      if (canSelect) {
        lv_obj_t* hit = lv_btn_create(card);
        lv_obj_set_size(hit, 400, 400);
        lv_obj_set_pos(hit, 0, 0);
        lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hit, 0, 0);
        lv_obj_add_event_cb(hit, childRewardCb, LV_EVENT_CLICKED, this);
        childRewardBtns_[i] = (_lv_obj_t*)hit;
        childRewardIds_[i] = reward["id"].as<String>();
        childRewardGoalIds_[i] = "";
        childRewardTypes_[i] = reward["rewardType"].as<String>();
      }
      ++i;
    }
    childRewardCount_ = i;
    if (i == 0) {
      for (int p = 0; p < 2; ++p) {
        const int x = p == 0 ? 72 : 552;
        lv_obj_t* card = focusCard(scr, x, 108, 400, 400, kChildCardPurple);
        focusLabel(card, LV_SYMBOL_IMAGE, 0, 12, 400, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
        childUiPhotoOrPlaceholder(card, nullptr, 100, 80, 200);
      }
    }
    childTaskBtns_[0] = (_lv_obj_t*)childUiBackButton(scr, this, toddler, 430);
    childTaskIds_[0] = "__back";
    childTaskCount_ = 1;
    displayUnlock();
    return;
  }

  JsonObjectConst childObj = state["child"].as<JsonObjectConst>();
  if (runtime.page != ChildFocusPage::Dashboard) {
    childUiHeader(scr, childObj, starBalance, visualStars, childHeaderImage_, toddler);
  }

  const char* focusType = state["focus"]["type"] | "dashboard";
  if (strcmp(focusType, "correction") == 0 || runtime.page == ChildFocusPage::Correction) {
    lv_obj_t* card = focusCard(scr, 252, 100, 520, 400, 0xE8DAEF);
    if (toddler) {
      focusLabel(card, LV_SYMBOL_STOP, 0, 60, 520, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
      focusLabel(card, LV_SYMBOL_OK, 0, 220, 520, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    } else {
      focusLabel(card, "Take a break", 30, 24, 460, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
      if (childFocusImage_ && childFocusImage_->pixels) {
        lv_obj_t* image = lv_img_create(card);
        lv_img_set_src(image, &childFocusImage_->descriptor);
        lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 72);
      } else {
        childUiPhotoOrPlaceholder(card, nullptr, 190, 72, 140);
      }
      focusLabel(card, state["focus"]["correction"]["label"] | "Ask a grown-up for help", 40, 250, 440,
                 &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    }
    childTaskBtns_[0] = (_lv_obj_t*)childUiBackButton(scr, this, toddler, 430);
    childTaskIds_[0] = "__back";
    childTaskCount_ = 1;
    displayUnlock();
    return;
  }

  if (strcmp(focusType, "first_then") == 0 || runtime.page == ChildFocusPage::FirstThen) {
    lv_obj_t* first = focusCard(scr, 48, 108, 380, 380, kChildCardBlue);
    lv_obj_t* then = focusCard(scr, 596, 108, 380, 380, kChildCardGreen);
    if (!toddler) {
      focusLabel(first, "First...", 20, 20, 340, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
      focusLabel(then, "Then...", 20, 20, 340, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    }
    if (childFocusImage_ && childFocusImage_->pixels) {
      lv_obj_t* image = lv_img_create(first);
      lv_img_set_src(image, &childFocusImage_->descriptor);
      lv_obj_align(image, LV_ALIGN_CENTER, 0, 10);
    } else {
      childUiLetterTile(first, 110, 110, 160, '1', kChildCardBlue);
    }
    childUiPhotoOrPlaceholder(then, childRewardImages_[0], 110, 110, 160);
    focusLabel(first, "1", 16, 16, 56, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    focusLabel(then, "2", 16, 16, 56, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    if (!toddler) {
      focusLabel(first, state["focus"]["first"]["label"] | "Do this first", 24, 110, 332,
                 &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
      focusLabel(then, state["focus"]["then"]["label"] | "You get this!", 24, 110, 332,
                 &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    }
    if (!toddler) focusLabel(scr, LV_SYMBOL_RIGHT, 460, 280, 104, &lv_font_montserrat_36, kChildText, LV_TEXT_ALIGN_CENTER);
    lv_obj_t* start = lv_btn_create(scr);
    lv_obj_set_pos(start, 412, 500);
    lv_obj_set_size(start, 200, 80);
    childUiStyleBtn(start, kChildCardGreen, 40);
    lv_obj_add_event_cb(start, childTaskCb, LV_EVENT_CLICKED, this);
    childTaskBtns_[0] = (_lv_obj_t*)start;
    childTaskIds_[0] = "__start-first-then";
    childTaskCount_ = 1;
    focusLabel(start, toddler ? LV_SYMBOL_PLAY : "Start", 0, 22, 200, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    childTaskBtns_[1] = (_lv_obj_t*)childUiBackButton(scr, this, toddler, 510);
    childTaskIds_[1] = "__back";
    childTaskCount_ = 2;
    displayUnlock();
    return;
  }

  if (runtime.page == ChildFocusPage::Task) {
    JsonObjectConst chosen;
    for (JsonObjectConst task : state["tasks"].as<JsonArrayConst>()) {
      if (task["assignmentId"].as<String>() == runtime.selectedTargetId) {
        chosen = task;
        break;
      }
    }
    const String statusText = chosen["completion"]["status"].as<String>();
    const bool waiting = statusText == "awaiting_parent";
    (void)waiting;
    lv_obj_t* card = focusCard(scr, 232, 108, 560, 340, kChildCardBlue);
    if (childFocusImage_ && childFocusImage_->pixels) {
      lv_obj_t* image = lv_img_create(card);
      lv_img_set_src(image, &childFocusImage_->descriptor);
      lv_obj_align(image, LV_ALIGN_CENTER, 0, -20);
    } else if (toddler) {
      childUiLetterTile(card, 180, 70, 200, childUiTaskLetter(chosen, 0), kChildCardBlue);
    } else {
      focusLabel(card, chosen["label"] | "Your job", 35, 100, 490, &lv_font_montserrat_36, 0xFFFFFF,
                 LV_TEXT_ALIGN_CENTER);
    }
    if (state["taskActions"]["canRequestTask"] | false) {
      lv_obj_t* b = lv_btn_create(scr);
      lv_obj_set_pos(b, 372, 470);
      lv_obj_set_size(b, 280, 100);
      childUiStyleBtn(b, kChildCardGreen, 50);
      lv_obj_add_event_cb(b, childTaskCb, LV_EVENT_CLICKED, this);
      childTaskBtns_[0] = (_lv_obj_t*)b;
      childTaskIds_[0] = runtime.selectedTargetId;
      childTaskCount_ = 1;
      focusLabel(b, toddler ? LV_SYMBOL_OK : "I did it!", 0, 26, 280, &lv_font_montserrat_36, 0xFFFFFF,
                 LV_TEXT_ALIGN_CENTER);
    }
    childTaskBtns_[childTaskCount_] = (_lv_obj_t*)childUiBackButton(scr, this, toddler, 510);
    childTaskIds_[childTaskCount_] = "__back";
    ++childTaskCount_;
    displayUnlock();
    return;
  }

  if (runtime.page == ChildFocusPage::Reward) {
    JsonObjectConst chosen;
    for (JsonObjectConst reward : state["rewards"].as<JsonArrayConst>()) {
      if (reward["id"].as<String>() == runtime.selectedTargetId) {
        chosen = reward;
        break;
      }
    }
    const bool ready = chosen["goal"]["ready"] | false;
    lv_obj_t* card = focusCard(scr, 120, 120, 784, 380, ready ? kChildCardGreen : kChildCardPurple);
    focusLabel(card, "My treat!", 30, 16, 724, &lv_font_montserrat_36, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    if (childFocusImage_ && childFocusImage_->pixels) {
      lv_obj_t* image = lv_img_create(card);
      lv_img_set_src(image, &childFocusImage_->descriptor);
      lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 58);
    } else {
      childUiPhotoOrPlaceholder(card, nullptr, 292, 70, 200);
    }
    const String goalId = chosen["goal"]["id"].as<String>();
    const bool canSelect = state["config"]["interactionsEnabled"] | false
                               ? state["rewardActions"]["canSelectReward"] | false
                               : false;
    const bool canRequest = state["config"]["interactionsEnabled"] | false
                                ? state["rewardActions"]["canRequestReward"] | false
                                : false;
    const bool actionable = goalId.isEmpty() ? canSelect : (ready && canRequest);
    const char* hint = goalId.isEmpty() ? "Want this one?" : (ready ? "You earned it!" : "Keep going!");
    focusLabel(card, hint, 30, 220, 724, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    if (actionable) {
      lv_obj_t* action = lv_btn_create(card);
      lv_obj_set_size(action, 420, 88);
      lv_obj_align(action, LV_ALIGN_BOTTOM_MID, 0, -18);
      childUiStyleBtn(action, kChildCardOrange, 44);
      lv_obj_add_event_cb(action, childRewardCb, LV_EVENT_CLICKED, this);
      childRewardBtns_[0] = (_lv_obj_t*)action;
      childRewardIds_[0] = chosen["id"].as<String>();
      childRewardGoalIds_[0] = goalId;
      childRewardTypes_[0] = chosen["rewardType"].as<String>();
      childRewardCount_ = 1;
      focusLabel(action, goalId.isEmpty() ? "Yes!" : "Ask grown-up", 0, 26, 420, &lv_font_montserrat_36,
                 0xFFFFFF, LV_TEXT_ALIGN_CENTER);
    }
    lv_obj_t* back = lv_btn_create(scr);
    lv_obj_set_pos(back, 24, 510);
    lv_obj_set_size(back, 180, 72);
    childUiStyleBtn(back, 0xDFE6E9, 36);
    lv_obj_add_event_cb(back, childRewardCb, LV_EVENT_CLICKED, this);
    childRewardBtns_[1] = (_lv_obj_t*)back;
    childRewardIds_[1] = "__back";
    childRewardCount_ = 2;
    focusLabel(back, "Back", 0, 22, 180, &lv_font_montserrat_28, kChildText, LV_TEXT_ALIGN_CENTER);
    displayUnlock();
    return;
  }

  const bool showTasks = state["config"]["taskGridVisible"] | false;
  const bool interactions = state["config"]["interactionsEnabled"] | false;
  childUiBuildDashboardHeader(scr, childObj, starBalance, childHeaderImage_, this, toddler);

  const int contentY = kChildContentTop;
  const int contentBottom = kChildNavBarY;

  if (childDashboardTab_ == ChildDashboardTab::Home) {
    const int dailyProgress = (int)(state["daily"]["progress"] | state["daily"]["current"] | 0);
    const int successful = (int)(state["term"]["successfulCredit"] | state["term"]["progress"]["successfulDays"] |
                                 state["term"]["current"] | 0);
    const int required = (int)(state["term"]["requiredSuccessfulDays"] |
                               state["term"]["progress"]["requiredSuccessfulDays"] | state["term"]["target"] | 10);
    const int dailyH = 100;
    lv_obj_t* daily = focusCard(scr, 24, contentY, 976, dailyH, 0xFFFFFF);
    childUiPhotoOrPlaceholder(daily, childStripDailyImage_, 20, 12, 76);
    focusLabel(daily, "Today", 116, 12, 200, &lv_font_montserrat_28, kChildText, LV_TEXT_ALIGN_LEFT);
    childUiDrawProgressRow(daily, 116, 48, dailyProgress, 5, 5, 220);
    focusLabel(daily, "more to go!", 116, 78, 200, &lv_font_montserrat_20, kChildText, LV_TEXT_ALIGN_LEFT);
    const int tileW = 220;
    const int tileH = 140;
    const int tileY = contentY + dailyH + 12;
    int preview = 0;
    int taskBtnCount = 0;
    for (JsonObjectConst task : state["tasks"].as<JsonArrayConst>()) {
      if (preview >= 4) break;
      const int x = 24 + preview * (tileW + 16);
      lv_obj_t* card = focusCard(scr, x, tileY, tileW, tileH, 0xFFFFFF);
      if (childTaskImages_[preview] && childTaskImages_[preview]->pixels) {
        childUiPlacePhoto(card, childTaskImages_[preview], 50, 12, 120);
      } else {
        childUiLetterTile(card, 70, 12, 80, childUiTaskLetter(task, preview), kChildCardBlue);
      }
      focusLabel(card, childUiShortLabel(task["label"] | "Task").c_str(), 10, 98, tileW - 20,
                 &lv_font_montserrat_20, kChildText, LV_TEXT_ALIGN_CENTER);
      if (interactions) {
        lv_obj_t* hit = lv_btn_create(card);
        lv_obj_set_size(hit, tileW, tileH);
        lv_obj_set_pos(hit, 0, 0);
        lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hit, 0, 0);
        lv_obj_add_event_cb(hit, childTaskCb, LV_EVENT_CLICKED, this);
        childTaskBtns_[taskBtnCount] = (_lv_obj_t*)hit;
        childTaskIds_[taskBtnCount] = task["assignmentId"].as<String>();
        ++taskBtnCount;
      }
      ++preview;
    }
    while (preview < 4) {
      const int x = 24 + preview * (tileW + 16);
      lv_obj_t* card = focusCard(scr, x, tileY, tileW, tileH, 0xFFFFFF);
      static const char letters[] = {'T', 'O', 'Y', 'R'};
      childUiLetterTile(card, 70, 12, 80, letters[preview], 0xDFE6E9);
      focusLabel(card, "Task", 10, 98, tileW - 20, &lv_font_montserrat_20, kChildText, LV_TEXT_ALIGN_CENTER);
      ++preview;
    }
    childTaskCount_ = taskBtnCount;
    const int termY = tileY + tileH + 12;
    const int termH = contentBottom - termY - 8;
    lv_obj_t* term = focusCard(scr, 24, termY, 976, termH > 96 ? termH : 96, 0xFFFFFF);
    childUiPhotoOrPlaceholder(term, childStripTermImage_, 20, 12, 76);
    focusLabel(term, "Big reward", 116, 12, 240, &lv_font_montserrat_28, kChildText, LV_TEXT_ALIGN_LEFT);
    childUiDrawProgressRow(term, 116, 48, successful, required > 0 ? required : 10, 8, 760);
    if (interactions) addChildTaskLink(term, 0, 0, 976, termH > 96 ? termH : 96, "__open-goals");
  } else if (childDashboardTab_ == ChildDashboardTab::Tasks) {
    focusLabel(scr, "Pick one!", 28, contentY, 300, &lv_font_montserrat_28, kChildText, LV_TEXT_ALIGN_LEFT);
    const int listY = contentY + 36;
    const int listH = contentBottom - listY - 8;
    lv_obj_t* list = childUiScrollPanel(scr, 24, listY, 976, listH);
    int i = 0;
    const int tileW = 220;
    const int tileH = 168;
    const int tileGap = 16;
    const int cols = 4;
    if (showTasks) {
      for (JsonObjectConst task : state["tasks"].as<JsonArrayConst>()) {
        if (i >= kMaxChildTasks) break;
        const int col = i % cols;
        const int row = i / cols;
        const int x = col * (tileW + tileGap);
        const int y = row * (tileH + tileGap);
        lv_obj_t* card = focusCard(list, x, y, tileW, tileH, 0xFFFFFF);
        const String statusText = task["completion"]["status"].as<String>();
        const uint32_t taskColor = statusText == "awaiting_parent" ? kChildCardOrange
                                   : (statusText == "approved" || statusText == "completed" ? kChildCardGreen
                                                                                            : kChildCardBlue);
        if (childTaskImages_[i] && childTaskImages_[i]->pixels) {
          childUiPlacePhoto(card, childTaskImages_[i], 50, 12, 120);
        } else {
          childUiLetterTile(card, 70, 12, 80, childUiTaskLetter(task, i), taskColor);
        }
        focusLabel(card, childUiShortLabel(task["label"] | "Task").c_str(), 10, 124, tileW - 20,
                   &lv_font_montserrat_20, kChildText, LV_TEXT_ALIGN_CENTER);
        if (interactions) {
          lv_obj_t* hit = lv_btn_create(card);
          lv_obj_set_size(hit, tileW, tileH);
          lv_obj_set_pos(hit, 0, 0);
          lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);
          lv_obj_set_style_border_width(hit, 0, 0);
          lv_obj_add_event_cb(hit, childTaskCb, LV_EVENT_CLICKED, this);
          childTaskBtns_[i] = (_lv_obj_t*)hit;
          childTaskIds_[i] = task["assignmentId"].as<String>();
        }
        if (statusText == "approved" || statusText == "completed") childUiTaskBadge(card, LV_SYMBOL_OK, kChildCardGreen);
        else if (statusText == "awaiting_parent") childUiTaskBadge(card, LV_SYMBOL_BELL, kChildCardOrange);
        ++i;
      }
    }
    if (i == 0) {
      for (int p = 0; p < 4; ++p) {
        const int x = p * (tileW + tileGap);
        lv_obj_t* card = focusCard(list, x, 0, tileW, tileH, 0xFFFFFF);
        static const char letters[] = {'T', 'O', 'Y', 'R'};
        childUiLetterTile(card, 70, 20, 80, letters[p], 0xDFE6E9);
        focusLabel(card, "Task", 10, 124, tileW - 20, &lv_font_montserrat_20, kChildText, LV_TEXT_ALIGN_CENTER);
      }
    }
    childTaskCount_ = i;
  } else if (childDashboardTab_ == ChildDashboardTab::Routines) {
    focusLabel(scr, "Routines", 28, contentY, 300, &lv_font_montserrat_28, kChildText, LV_TEXT_ALIGN_LEFT);
    const int listY = contentY + 36;
    const int listH = contentBottom - listY - 8;
    lv_obj_t* list = childUiScrollPanel(scr, 24, listY, 976, listH);
    int r = 0;
    const int cardH = 112;
    const int cardGap = 12;
    for (JsonObjectConst routine : state["routines"].as<JsonArrayConst>()) {
      if (r >= kMaxChildRoutines) break;
      const int y = r * (cardH + cardGap);
      lv_obj_t* card = focusCard(list, 0, y, 976, cardH, kChildCardBlue);
      if (childRoutineImages_[r] && childRoutineImages_[r]->pixels) {
        childUiPlacePhoto(card, childRoutineImages_[r], 20, 16, 80);
      } else {
        childUiLetterTile(card, 20, 16, 80, (char)('A' + r), kChildCardPurple);
      }
      focusLabel(card, childUiShortLabel(routine["label"] | "Routine", 20).c_str(), 120, 18, 500,
                 &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_LEFT);
      const int done = (int)(routine["done"] | 0);
      const int total = (int)(routine["total"] | 0);
      childUiDrawProgressRow(card, 120, 64, done, total > 0 ? total : 4, 6, 820);
      ++r;
    }
    while (r < 2) {
      const int y = r * (cardH + cardGap);
      lv_obj_t* card = focusCard(list, 0, y, 976, cardH, kChildCardBlue);
      childUiLetterTile(card, 20, 16, 80, (char)('A' + r), 0xDFE6E9);
      focusLabel(card, "Routine", 120, 18, 500, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_LEFT);
      childUiDrawProgressRow(card, 120, 64, 0, 4, 4, 820);
      ++r;
    }
  } else if (childDashboardTab_ == ChildDashboardTab::Rewards) {
    const int successful = (int)(state["term"]["successfulCredit"] | state["term"]["progress"]["successfulDays"] |
                                 state["term"]["current"] | 0);
    const int required = (int)(state["term"]["requiredSuccessfulDays"] |
                               state["term"]["progress"]["requiredSuccessfulDays"] | state["term"]["target"] | 10);
    const int heroH = 168;
    lv_obj_t* hero = focusCard(scr, 24, contentY, 976, heroH, kChildCardGreen);
    childUiPhotoOrPlaceholder(hero, childStripTermImage_, 24, 20, 128);
    focusLabel(hero, "Big reward", 176, 18, 300, &lv_font_montserrat_28, 0xFFFFFF, LV_TEXT_ALIGN_LEFT);
    childUiDrawProgressRow(hero, 176, 64, successful, required > 0 ? required : 10, 8, 760);
    if (interactions) addChildTaskLink(hero, 0, 0, 976, heroH, "__open-goals");
    const int cardY = contentY + heroH + 12;
    const int cardH = 168;
    int i = 0;
    for (JsonObjectConst reward : state["rewards"].as<JsonArrayConst>()) {
      if (i >= 2 || !reward["goal"].isNull()) continue;
      const int x = i == 0 ? 24 : 520;
      lv_obj_t* card = focusCard(scr, x, cardY, 480, cardH, kChildCardPurple);
      childUiPhotoOrPlaceholder(card, childRewardImages_[i], 140, 16, 200);
      if (interactions && (state["rewardActions"]["canSelectReward"] | false)) {
        lv_obj_t* hit = lv_btn_create(card);
        lv_obj_set_size(hit, 480, cardH);
        lv_obj_set_pos(hit, 0, 0);
        lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hit, 0, 0);
        lv_obj_add_event_cb(hit, childRewardCb, LV_EVENT_CLICKED, this);
        childRewardBtns_[i] = (_lv_obj_t*)hit;
        childRewardIds_[i] = reward["id"].as<String>();
        childRewardGoalIds_[i] = "";
        childRewardTypes_[i] = reward["rewardType"].as<String>();
      }
      ++i;
    }
    childRewardCount_ = i;
    if (interactions && (state["rewardActions"]["canSelectReward"] | false) && starBalance > 0) {
      const int treatsY = cardY + cardH + 12;
      const int treatsH = contentBottom - treatsY - 8;
      if (treatsH >= 56) {
        lv_obj_t* treats = lv_btn_create(scr);
        lv_obj_set_pos(treats, 24, treatsY);
        lv_obj_set_size(treats, 976, treatsH);
        childUiStyleBtn(treats, kChildCardOrange, 32);
        lv_obj_add_event_cb(treats, childTaskCb, LV_EVENT_CLICKED, this);
        childTaskBtns_[0] = (_lv_obj_t*)treats;
        childTaskIds_[0] = "__open-treats";
        childTaskCount_ = 1;
        focusLabel(treats, "Pick a treat!", 0, (treatsH - 28) / 2, 976, &lv_font_montserrat_28, 0xFFFFFF,
                   LV_TEXT_ALIGN_CENTER);
      }
    }
  }

  buildChildTabBar(scr, childDashboardTab_);
  displayUnlock();
}

void UiManager::handleChildModeToggle(){childModeEnterRequested_=true;}
void UiManager::handleChildProfileTap(_lv_obj_t* target){for(int i=0;i<childProfileCount_;++i)if(childProfileBtns_[i]==target){requestedChildId_=childProfileIds_[i];childSelectionRequested_=true;return;}}
void UiManager::handleChildTaskTap(_lv_obj_t* target){for(int i=0;i<childTaskCount_;++i)if(childTaskBtns_[i]==target){const String&id=childTaskIds_[i];if(id=="__back"){childActionKind_="back-dashboard";childActionTargetId_="";}else if(id=="__open-goals"){childActionKind_="open-goals";childActionTargetId_="";}else if(id=="__open-treats"){childActionKind_="open-treats";childActionTargetId_="";}else if(id=="__start-first-then"){childActionKind_="start-first-then";childActionTargetId_="";}else{childActionKind_=renderedChildPage_==ChildFocusPage::Task?"complete-task":"open-task";childActionTargetId_=id;}childActionRequested_=true;return;}}
void UiManager::handleChildRewardTap(_lv_obj_t* target){for(int i=0;i<childRewardCount_;++i)if(childRewardBtns_[i]==target){if(childRewardIds_[i]=="__back"){childActionKind_="back-dashboard";childActionTargetId_="";}else if(renderedChildPage_==ChildFocusPage::Treats){childActionKind_=String("select-reward:")+childRewardTypes_[i];childActionTargetId_=childRewardIds_[i];}else if(renderedChildPage_!=ChildFocusPage::Reward){childActionKind_="open-reward";childActionTargetId_=childRewardIds_[i];}else{childActionKind_=childRewardGoalIds_[i].isEmpty()?String("select-reward:")+childRewardTypes_[i]:"request-reward";childActionTargetId_=childRewardGoalIds_[i].isEmpty()?childRewardIds_[i]:childRewardGoalIds_[i];}childActionRequested_=true;return;}}
void UiManager::handleChildNavTap(_lv_obj_t* target) {
  for (int i = 0; i < childNavCount_; ++i) {
    if (childNavBtns_[i] == target) {
      childActionKind_ = childNavKinds_[i];
      childActionTargetId_ = "";
      childActionRequested_ = true;
      return;
    }
  }
}
void UiManager::handleChildExitTap(){childExitRequested_=true;}
void UiManager::handleChildPinTap(_lv_obj_t* target){for(int i=0;i<childPinCount_;++i)if(childPinBtns_[i]==target){const String key=childPinKeys_[i];if(key=="BACK"){if(!childPinValue_.isEmpty())childPinValue_.remove(childPinValue_.length()-1);}else if(key=="GO"){if(!childPinValue_.isEmpty()){requestedChildPin_=childPinValue_;childPinValue_="";childPinRequested_=true;}}else if(childPinValue_.length()<12)childPinValue_+=key;if(childPinMask_){String masked;for(size_t n=0;n<childPinValue_.length();++n)masked+="*";lv_label_set_text((lv_obj_t*)childPinMask_,masked.c_str());}return;}}

#endif // FAMILY_HUB_APP_HOUSEHOLD
#endif // WAVESHARE_7B
