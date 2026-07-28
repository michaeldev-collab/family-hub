#include "ui_manager.h"

#ifdef WAVESHARE_7B
#include <cstring>
#include <lvgl.h>
#include "display.h"
#include "panel_config.h"
#endif




void UiManager::nextScreen() {
  // Cycle household tabs only — Settings is opened from Diagnostics, not nav.
  int n = static_cast<int>(current_) + 1;
  if (n > static_cast<int>(ScreenId::App)) n = 0;
  current_ = static_cast<ScreenId>(n);
  renderRequested_ = true;
}





bool UiManager::consumeChoreCompleteRequest(String& outChoreId) {
  if (!choreCompleteRequested_) return false;
  choreCompleteRequested_ = false;   // dispatched once — modal stays in-flight
  outChoreId = pendingChoreId_;
  return true;
}


void UiManager::reportChoreCompleteResult(bool ok, bool offline) {
  // Keep network completion free of LVGL calls. The next admitted full render
  // applies this modal result on the loop/LVGL task under the display lock.
  choreResultOk_ = ok;
  choreResultOffline_ = offline;
  choreResultPending_ = true;
  renderRequested_ = true;
}



void UiManager::renderHome(const JsonDocument& state) {
  Serial.println("== HOME ==");

  JsonObjectConst home = state["home"].as<JsonObjectConst>();
  JsonArrayConst pinned = home["pinned"].as<JsonArrayConst>();
  if (pinned.isNull()) pinned = state["notes"]["pinned"].as<JsonArrayConst>();
  if (!pinned.isNull() && pinned.size() > 0) {
    Serial.println("Pinned:");
    for (JsonObjectConst n : pinned) {
      Serial.printf(" * %s\n", n["text"].as<const char*>());
    }
  }

  Serial.printf("Dinner today: %s\n",
                home["dinner_today"].isNull() ? "No dinner planned"
                                              : (home["dinner_today"] | "No dinner planned"));

  JsonArrayConst week = state["dinner"]["week"].as<JsonArrayConst>();
  if (!week.isNull() && week.size() > 0) {
    Serial.println("Week ahead:");
    int shown = 0;
    for (JsonObjectConst d : week) {
      if (shown++ >= 3) break;
      Serial.printf(" %s: %s\n", d["date"].as<const char*>(), d["meal"].as<const char*>());
    }
  }

  JsonArrayConst chores = state["chores"]["items"].as<JsonArrayConst>();
  JsonArrayConst grocery = state["grocery"]["items"].as<JsonArrayConst>();
  const unsigned choreCount = home["open_chores_count"].is<int>()
                                  ? (unsigned)home["open_chores_count"].as<int>()
                                  : (chores.isNull() ? 0 : chores.size());
  const unsigned groceryCount = home["grocery_count"].is<int>()
                                    ? (unsigned)home["grocery_count"].as<int>()
                                    : (grocery.isNull() ? 0 : grocery.size());
  Serial.printf("Open chores: %u\n", choreCount);
  for (JsonObjectConst c : chores) {
    Serial.printf(" - %s\n", c["title"].as<const char*>());
  }

  Serial.printf("Grocery items: %u\n", groceryCount);
  for (JsonObjectConst g : grocery) {
    Serial.printf(" - %s\n", g["text"].as<const char*>());
  }
}

void UiManager::renderGrocery(const JsonDocument& state) {
  Serial.println("== GROCERY ==");
  for (JsonObjectConst g : state["grocery"]["items"].as<JsonArrayConst>()) {
    Serial.printf(" - %s\n", g["text"].as<const char*>());
  }
}

void UiManager::renderChores(const JsonDocument& state) {
  Serial.println("== CHORES ==");
  for (JsonObjectConst c : state["chores"]["items"].as<JsonArrayConst>()) {
    Serial.printf(" - %s\n", c["title"].as<const char*>());
  }
}

void UiManager::renderDinner(const JsonDocument& state) {
  Serial.println("== DINNER ==");
  JsonObjectConst today = state["dinner"]["today"].as<JsonObjectConst>();
  Serial.printf("Today: %s\n", today.isNull() ? "Unassigned" : (today["meal"] | "Unassigned"));
  JsonArrayConst week = state["dinner"]["week"].as<JsonArrayConst>();
  if (!week.isNull()) {
    Serial.println("This week:");
    for (JsonObjectConst d : week) {
      Serial.printf(" %s: %s\n", d["date"].as<const char*>(), d["meal"].as<const char*>());
    }
  }
}

void UiManager::renderNotes(const JsonDocument& state) {
  Serial.println("== NOTES ==");
  JsonArrayConst pinned = state["notes"]["pinned"].as<JsonArrayConst>();
  if (!pinned.isNull() && pinned.size() > 0) {
    Serial.println("Pinned:");
    for (JsonObjectConst n : pinned) {
      Serial.printf(" * %s\n", n["text"].as<const char*>());
    }
  }
  for (JsonObjectConst n : state["notes"]["recent"].as<JsonArrayConst>()) {
    Serial.printf(" - %s\n", n["text"].as<const char*>());
  }
}

void UiManager::renderApp(const PanelStatus& status) {
  Serial.println("== APP ==");
  Serial.printf("Web app (LAN fallback): http://%s:%u/\n", status.serverHost.c_str(),
                (unsigned)status.serverPort);
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
  Serial.println("Edit wifi/host/port/token on panel (Diagnostics → Open Settings); NVS: wifi_ssid, wifi_pass, srv_host, srv_port, write_tok");
}

void UiManager::render(const PanelStatus& status, const JsonDocument& state, bool stale) {
  renderStatusBar(status, stale);

#ifdef WAVESHARE_7B
  if (displayReady()) {
    lvglUpdate(status, state, stale);
  }
#endif

  switch (current_) {
    case ScreenId::Home: renderHome(state); break;
    case ScreenId::Grocery: renderGrocery(state); break;
    case ScreenId::Chores: renderChores(state); break;
    case ScreenId::Dinner: renderDinner(state); break;
    case ScreenId::Notes: renderNotes(state); break;
    case ScreenId::App: renderApp(status); break;
    case ScreenId::Settings: renderSettings(status); break;
  }
  Serial.println();
}

#ifdef WAVESHARE_7B

static const char* screenName(ScreenId s) {
  switch (s) {
    case ScreenId::Home:     return "HOME";
    case ScreenId::Grocery:  return "GROCERY";
    case ScreenId::Chores:   return "CHORES";
    case ScreenId::Dinner:   return "DINNER";
    case ScreenId::Notes:    return "NOTES";
    case ScreenId::App:      return "APP";
    case ScreenId::Settings: return "SETTINGS";
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
    case ScreenId::App:      return "Web App";
    case ScreenId::Settings: return "Settings";
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
    case ScreenId::App:      return "App";
    case ScreenId::Settings: return "Settings";
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
    const char* main = item["main"] | "";
    if (!main || !*main) main = item["meal"] | "Unassigned";
    s += main && *main ? main : "Unassigned";
    s += "\n";
  }
  return s;
}

static String dinnerTonightLines(JsonObjectConst today, const char* fallbackMeal) {
  if (today.isNull()) {
    return String(fallbackMeal && *fallbackMeal ? fallbackMeal : "No dinner planned");
  }
  const char* main = today["main"] | "";
  const char* side = today["side"] | "";
  const char* side2 = today["side2"] | "";
  String s;
  if (main && *main) {
    s += main;
  } else {
    const char* meal = today["meal"] | "";
    if (meal && *meal) s += meal;
    else if (fallbackMeal && *fallbackMeal) s += fallbackMeal;
    else s += "No dinner planned";
  }
  if (side && *side) {
    s += "\n";
    s += side;
  }
  if (side2 && *side2) {
    s += "\n";
    s += side2;
  }
  return s;
}

static String dinnerManageOverview(JsonObjectConst today, const char* cookFallback) {
  if (today.isNull()) {
    String s = "Cook: ";
    s += cookFallback && *cookFallback ? cookFallback : "Unassigned";
    s += "\nNotes: —";
    return s;
  }
  const char* cook = today["cook_name"] | "";
  if (!cook || !*cook) cook = cookFallback && *cookFallback ? cookFallback : "Unassigned";
  const char* notes = today["notes"] | "";
  String s = "Cook: ";
  s += cook && *cook ? cook : "Unassigned";
  s += "\nNotes: ";
  s += notes && *notes ? notes : "—";
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

static String homeNext(JsonArrayConst chores, JsonArrayConst grocery,
                       JsonArrayConst pinned, JsonArrayConst notes) {
  String s;
  s += "CHORES\n";
  s += firstItems(chores, "title", 5, "All caught up");
  s += "\nGROCERY\n";
  s += firstItems(grocery, "text", 5, "List is empty");
  s += "\nNOTES\n";
  if (!pinned.isNull() && pinned.size() > 0) {
    s += firstItems(pinned, "text", 4, "No notes yet");
  } else {
    s += firstItems(notes, "text", 4, "No notes yet");
  }
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

static void groceryRowCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (!ui) return;
  ui->handleGroceryRowTap((_lv_obj_t*)lv_event_get_target(e));
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

static void settingsSaveCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (!ui) return;
  ui->handleSettingsSave();
}

static void settingsBackCb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  UiManager* ui = (UiManager*)lv_event_get_user_data(e);
  if (!ui) return;
  ui->handleSettingsBack();
}

static void settingsTaEventCb(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_t* kb = (lv_obj_t*)lv_event_get_user_data(e);
  if (!kb || !ta) return;
  if (code == LV_EVENT_FOCUSED) {
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(kb);
  } else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_READY ||
             code == LV_EVENT_CANCEL) {
    lv_keyboard_set_textarea(kb, nullptr);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(ta, LV_STATE_FOCUSED);
  }
}


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

    // Scroll viewport: labels alone are not clickable, so finger-drag never
    // scrolls them. A clickable container owns the gesture; the label sizes to
    // content inside it.
    lv_obj_t* bodyScroll = lv_obj_create(card);
    lv_obj_set_pos(bodyScroll, 16, 44);
    lv_obj_set_size(bodyScroll, 68, 44);
    lv_obj_set_style_bg_opa(bodyScroll, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bodyScroll, 0, 0);
    lv_obj_set_style_pad_all(bodyScroll, 0, 0);
    lv_obj_set_style_radius(bodyScroll, 0, 0);
    lv_obj_add_flag(bodyScroll, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(bodyScroll, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(bodyScroll, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(bodyScroll, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t* body = lv_label_create(bodyScroll);
    lv_obj_set_width(body, 68);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(body, lv_color_hex(0xF2F5F0), 0);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_line_space(body, 4, 0);
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 0);

    cards_[i] = (_lv_obj_t*)card;
    cardTitles_[i] = (_lv_obj_t*)cardTitle;
    cardBodies_[i] = (_lv_obj_t*)bodyScroll;
    cardBodyLabels_[i] = (_lv_obj_t*)body;
  }

  lv_obj_t* navBg = lv_obj_create(scr);
  lv_obj_set_size(navBg, PANEL_H_RES, 86);
  lv_obj_set_pos(navBg, 0, PANEL_V_RES - 86);
  lv_obj_set_style_bg_color(navBg, lv_color_hex(0x0D2212), 0);
  lv_obj_set_style_border_width(navBg, 1, 0);
  lv_obj_set_style_border_color(navBg, lv_color_hex(0x1F6B2D), 0);
  lv_obj_set_style_radius(navBg, 0, 0);
  lv_obj_clear_flag(navBg, LV_OBJ_FLAG_SCROLLABLE);
  navBg_ = (_lv_obj_t*)navBg;

  const int nav_y = PANEL_V_RES - 68;
  const int gap = 10;
  const int nav_w = (PANEL_H_RES - 48 - (gap * (kNavTabCount - 1))) / kNavTabCount;
  for (int i = 0; i < kNavTabCount; ++i) {
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

  buildSettingsPanel((_lv_obj_t*)scr);
  buildAppPanel((_lv_obj_t*)scr);

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
  // Grocery board is built lazily on first Grocery visit (saves LVGL heap).

  lvglUpdateNav();
  displayUnlock();
}

void UiManager::destroyGroceryBoard() {
  if (!groceryBoard_) return;
  lv_obj_del((lv_obj_t*)groceryBoard_);
  groceryBoard_ = nullptr;
  for (int c = 0; c < kGroceryCols; ++c) {
    groceryColTitles_[c] = nullptr;
    for (int r = 0; r < kGroceryRowsPerCol; ++r) {
      groceryRowBtns_[c][r] = nullptr;
      groceryRowLabels_[c][r] = nullptr;
      groceryRowIds_[c][r][0] = '\0';
    }
  }
}

void UiManager::ensureGroceryBoard() {
  if (groceryBoard_) return;
  lv_obj_t* scr = lv_scr_act();
  if (!scr) return;

  lv_mem_monitor_t monBefore;
  lv_mem_monitor(&monBefore);
  Serial.printf("[ui] grocery board build lv_free=%u\n", (unsigned)monBefore.free_size);

  lv_obj_t* board = lv_obj_create(scr);
  if (!board) {
    Serial.println("[ui] grocery board OOM (root)");
    return;
  }
  lv_obj_set_pos(board, 24, 126);
  lv_obj_set_size(board, PANEL_H_RES - 48, 374);
  lv_obj_set_style_bg_opa(board, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(board, 0, 0);
  lv_obj_set_style_pad_all(board, 0, 0);
  lv_obj_clear_flag(board, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(board, LV_OBJ_FLAG_HIDDEN);
  groceryBoard_ = (_lv_obj_t*)board;

  const int colW = (PANEL_H_RES - 48 - 24) / kGroceryCols;
  const char* defaults[kGroceryCols] = {"Constant", "Main", "Other"};
  for (int c = 0; c < kGroceryCols; ++c) {
    lv_obj_t* col = lv_obj_create(board);
    if (!col) {
      Serial.println("[ui] grocery board OOM (col)");
      destroyGroceryBoard();
      return;
    }
    lv_obj_set_pos(col, c * (colW + 12), 0);
    lv_obj_set_size(col, colW, 374);
    lv_obj_set_style_bg_color(col, lv_color_hex(0x0A1A0E), 0);
    lv_obj_set_style_border_width(col, 1, 0);
    lv_obj_set_style_border_color(col, lv_color_hex(0x1F6B2D), 0);
    lv_obj_set_style_radius(col, 8, 0);
    lv_obj_set_style_pad_all(col, 8, 0);
    lv_obj_set_scroll_dir(col, LV_DIR_VER);

    lv_obj_t* title = lv_label_create(col);
    if (!title) {
      Serial.println("[ui] grocery board OOM (title)");
      destroyGroceryBoard();
      return;
    }
    lv_label_set_text(title, defaults[c]);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00FF41), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(title, 0, 0);
    groceryColTitles_[c] = (_lv_obj_t*)title;

    for (int r = 0; r < kGroceryRowsPerCol; ++r) {
      // Plain clickable objs are lighter than lv_btn (less style/state overhead).
      lv_obj_t* row = lv_obj_create(col);
      if (!row) {
        Serial.println("[ui] grocery board OOM (row)");
        destroyGroceryBoard();
        return;
      }
      lv_obj_set_size(row, colW - 24, 40);
      lv_obj_set_pos(row, 0, 36 + r * 46);
      lv_obj_set_style_radius(row, 6, 0);
      lv_obj_set_style_bg_color(row, lv_color_hex(0x132B18), 0);
      lv_obj_set_style_bg_color(row, lv_color_hex(0x123F1A), LV_STATE_PRESSED);
      lv_obj_set_style_border_width(row, 1, 0);
      lv_obj_set_style_border_color(row, lv_color_hex(0x1F6B2D), 0);
      lv_obj_set_style_pad_all(row, 0, 0);
      lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(row, groceryRowCb, LV_EVENT_CLICKED, this);
      lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);

      lv_obj_t* rl = lv_label_create(row);
      if (!rl) {
        Serial.println("[ui] grocery board OOM (label)");
        destroyGroceryBoard();
        return;
      }
      lv_label_set_long_mode(rl, LV_LABEL_LONG_CLIP);
      lv_obj_set_width(rl, colW - 40);
      lv_obj_set_style_text_color(rl, lv_color_hex(0xF2F5F0), 0);
      lv_obj_set_style_text_font(rl, &lv_font_montserrat_20, 0);
      lv_obj_align(rl, LV_ALIGN_LEFT_MID, 8, 0);
      groceryRowBtns_[c][r] = (_lv_obj_t*)row;
      groceryRowLabels_[c][r] = (_lv_obj_t*)rl;
      groceryRowIds_[c][r][0] = '\0';
    }
  }
}

void UiManager::hideGroceryBoard() {
  if (groceryBoard_) lv_obj_add_flag((lv_obj_t*)groceryBoard_, LV_OBJ_FLAG_HIDDEN);
}

void UiManager::populateGroceryBoard(JsonObjectConst grocery) {
  ensureGroceryBoard();
  if (!groceryBoard_) {
    // Fallback: three static cards if LVGL heap is too tight for the board.
    lvglLayoutCard(0, 24, 126, 320, 374, true);
    lvglLayoutCard(1, 352, 126, 320, 374, true);
    lvglLayoutCard(3, 680, 126, 320, 374, true);
    auto colText = [](JsonArrayConst arr, bool constantCol) -> String {
      if (arr.isNull() || arr.size() == 0) return String("Empty");
      String s;
      int n = 0;
      for (JsonObjectConst item : arr) {
        if (n >= 8) break;
        const char* text = item["text"] | "";
        if (constantCol) {
          s += (item["needed"] | false) ? "[!] " : "[ ] ";
        } else {
          s += (item["checked"] | false) ? "[x] " : "[ ] ";
        }
        s += text;
        s += "\n";
        ++n;
      }
      return s;
    };
    const char* otherTitle = grocery.isNull() ? "Other" : (grocery["other_title"] | "Other");
    lvglSetCard(0, "Constant", colText(grocery["constant"].as<JsonArrayConst>(), true));
    lvglSetCard(1, "Main", colText(grocery["main"].as<JsonArrayConst>(), false));
    lvglSetCard(3, otherTitle, colText(grocery["other"].as<JsonArrayConst>(), false));
    return;
  }
  lv_obj_clear_flag((lv_obj_t*)groceryBoard_, LV_OBJ_FLAG_HIDDEN);

  const char* otherTitle = grocery.isNull() ? "Other" : (grocery["other_title"] | "Other");
  if (groceryColTitles_[0]) lv_label_set_text((lv_obj_t*)groceryColTitles_[0], "Constant");
  if (groceryColTitles_[1]) lv_label_set_text((lv_obj_t*)groceryColTitles_[1], "Main");
  if (groceryColTitles_[2]) lv_label_set_text((lv_obj_t*)groceryColTitles_[2], otherTitle);

  JsonArrayConst cols[kGroceryCols] = {
      grocery["constant"].as<JsonArrayConst>(),
      grocery["main"].as<JsonArrayConst>(),
      grocery["other"].as<JsonArrayConst>(),
  };

  for (int c = 0; c < kGroceryCols; ++c) {
    int n = 0;
    if (!cols[c].isNull()) {
      for (JsonObjectConst item : cols[c]) {
        if (n >= kGroceryRowsPerCol) break;
        const char* id = item["id"] | "";
        const char* text = item["text"] | "";
        if (!id || !*id) continue;
        strncpy(groceryRowIds_[c][n], id, sizeof(groceryRowIds_[c][n]) - 1);
        groceryRowIds_[c][n][sizeof(groceryRowIds_[c][n]) - 1] = '\0';
        String label = text;
        if (c == 0) {
          const bool needed = item["needed"] | false;
          label = String(needed ? "[!] " : "[ ] ") + text;
        } else {
          const bool checked = item["checked"] | false;
          label = String(checked ? "[x] " : "[ ] ") + text;
        }
        if (groceryRowLabels_[c][n]) {
          lv_label_set_text((lv_obj_t*)groceryRowLabels_[c][n], label.c_str());
        }
        if (groceryRowBtns_[c][n]) {
          lv_obj_clear_flag((lv_obj_t*)groceryRowBtns_[c][n], LV_OBJ_FLAG_HIDDEN);
        }
        ++n;
      }
    }
    for (int r = n; r < kGroceryRowsPerCol; ++r) {
      if (groceryRowBtns_[c][r]) {
        lv_obj_add_flag((lv_obj_t*)groceryRowBtns_[c][r], LV_OBJ_FLAG_HIDDEN);
      }
      groceryRowIds_[c][r][0] = '\0';
    }
  }
}

void UiManager::handleGroceryRowTap(_lv_obj_t* target) {
  for (int c = 0; c < kGroceryCols; ++c) {
    for (int r = 0; r < kGroceryRowsPerCol; ++r) {
      if (groceryRowBtns_[c][r] == target) {
        if (groceryRowIds_[c][r][0] == '\0') return;
        pendingGroceryId_ = groceryRowIds_[c][r];
        groceryToggleRequested_ = true;
        return;
      }
    }
  }
}

bool UiManager::consumeGroceryToggleRequest(String& outGroceryId) {
  if (!groceryToggleRequested_) return false;
  groceryToggleRequested_ = false;
  outGroceryId = pendingGroceryId_;
  pendingGroceryId_ = "";
  return outGroceryId.length() > 0;
}

void UiManager::buildSettingsPanel(_lv_obj_t* scrRaw) {
  lv_obj_t* scr = (lv_obj_t*)scrRaw;

  lv_obj_t* panel = lv_obj_create(scr);
  lv_obj_set_pos(panel, 24, 126);
  lv_obj_set_size(panel, PANEL_H_RES - 48, PANEL_V_RES - 126 - 24);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x0D2212), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x1F6B2D), 0);
  lv_obj_set_style_radius(panel, 8, 0);
  lv_obj_set_style_pad_all(panel, 16, 0);
  // Scrollable so Wi-Fi + server fields fit above the on-screen keyboard.
  lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(panel, LV_DIR_VER);
  lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
  settingsPanel_ = (_lv_obj_t*)panel;

  lv_obj_t* status = lv_label_create(panel);
  lv_obj_set_width(status, PANEL_H_RES - 96);
  lv_label_set_long_mode(status, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(status, lv_color_hex(0x8AAB8E), 0);
  lv_obj_set_style_text_font(status, &lv_font_montserrat_20, 0);
  lv_obj_align(status, LV_ALIGN_TOP_LEFT, 0, 0);
  settingsStatus_ = (_lv_obj_t*)status;

  auto makeField = [&](const char* caption, lv_coord_t y, bool password) -> lv_obj_t* {
    lv_obj_t* lab = lv_label_create(panel);
    lv_label_set_text(lab, caption);
    lv_obj_set_style_text_color(lab, lv_color_hex(0x00FF41), 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(lab, 0, y);

    lv_obj_t* ta = lv_textarea_create(panel);
    lv_obj_set_size(ta, PANEL_H_RES - 120, 44);
    lv_obj_set_pos(ta, 0, y + 28);
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, password ? 64 : 64);
    if (password) lv_textarea_set_password_mode(ta, true);
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x132B18), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x1F6B2D), 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(0xF2F5F0), 0);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_20, 0);
    return ta;
  };

  settingsWifiSsid_ = (_lv_obj_t*)makeField("Wi-Fi SSID", 56, false);
  settingsWifiPass_ = (_lv_obj_t*)makeField("Wi-Fi password", 140, true);
  settingsHost_ = (_lv_obj_t*)makeField("Server host", 224, false);
  settingsPort_ = (_lv_obj_t*)makeField("Port", 308, false);
  settingsToken_ = (_lv_obj_t*)makeField("Write / panel token", 392, true);

  lv_obj_t* saveBtn = lv_btn_create(panel);
  lv_obj_set_size(saveBtn, 200, 52);
  lv_obj_set_pos(saveBtn, 0, 490);
  lv_obj_set_style_bg_color(saveBtn, lv_color_hex(0x123F1A), 0);
  lv_obj_set_style_border_width(saveBtn, 1, 0);
  lv_obj_set_style_border_color(saveBtn, lv_color_hex(0x00FF41), 0);
  lv_obj_add_event_cb(saveBtn, settingsSaveCb, LV_EVENT_CLICKED, this);
  lv_obj_t* saveLab = lv_label_create(saveBtn);
  lv_label_set_text(saveLab, "Save");
  lv_obj_center(saveLab);

  lv_obj_t* backBtn = lv_btn_create(panel);
  lv_obj_set_size(backBtn, 200, 52);
  lv_obj_set_pos(backBtn, 220, 490);
  lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x132B18), 0);
  lv_obj_set_style_border_width(backBtn, 1, 0);
  lv_obj_set_style_border_color(backBtn, lv_color_hex(0x1F6B2D), 0);
  lv_obj_add_event_cb(backBtn, settingsBackCb, LV_EVENT_CLICKED, this);
  lv_obj_t* backLab = lv_label_create(backBtn);
  lv_label_set_text(backLab, "Back");
  lv_obj_center(backLab);

  lv_obj_t* kb = lv_keyboard_create(scr);
  lv_obj_set_size(kb, PANEL_H_RES, 200);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  settingsKeyboard_ = (_lv_obj_t*)kb;

  // Do not use LV_EVENT_ALL — VALUE_CHANGED storms during typing were a
  // freeze/crash risk with the keyboard attached.
  const lv_event_code_t taEvents[] = {
      LV_EVENT_FOCUSED, LV_EVENT_DEFOCUSED, LV_EVENT_READY, LV_EVENT_CANCEL};
  for (lv_event_code_t ev : taEvents) {
    lv_obj_add_event_cb((lv_obj_t*)settingsWifiSsid_, settingsTaEventCb, ev, kb);
    lv_obj_add_event_cb((lv_obj_t*)settingsWifiPass_, settingsTaEventCb, ev, kb);
    lv_obj_add_event_cb((lv_obj_t*)settingsHost_, settingsTaEventCb, ev, kb);
    lv_obj_add_event_cb((lv_obj_t*)settingsPort_, settingsTaEventCb, ev, kb);
    lv_obj_add_event_cb((lv_obj_t*)settingsToken_, settingsTaEventCb, ev, kb);
  }
}

void UiManager::dismissSettingsKeyboard() {
  if (!settingsKeyboard_) return;
  lv_obj_t* kb = (lv_obj_t*)settingsKeyboard_;
  lv_keyboard_set_textarea(kb, nullptr);
  lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
  if (settingsWifiSsid_) lv_obj_clear_state((lv_obj_t*)settingsWifiSsid_, LV_STATE_FOCUSED);
  if (settingsWifiPass_) lv_obj_clear_state((lv_obj_t*)settingsWifiPass_, LV_STATE_FOCUSED);
  if (settingsHost_) lv_obj_clear_state((lv_obj_t*)settingsHost_, LV_STATE_FOCUSED);
  if (settingsPort_) lv_obj_clear_state((lv_obj_t*)settingsPort_, LV_STATE_FOCUSED);
  if (settingsToken_) lv_obj_clear_state((lv_obj_t*)settingsToken_, LV_STATE_FOCUSED);
}

void UiManager::showSettingsPanel(const PanelStatus& status, bool show) {
  if (!settingsPanel_) return;
  if (!show) {
    lv_obj_add_flag((lv_obj_t*)settingsPanel_, LV_OBJ_FLAG_HIDDEN);
    dismissSettingsKeyboard();
    settingsSeeded_ = false;
    if (navBg_) lv_obj_clear_flag((lv_obj_t*)navBg_, LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < kNavTabCount; ++i) {
      if (navButtons_[i]) lv_obj_clear_flag((lv_obj_t*)navButtons_[i], LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  lv_obj_clear_flag((lv_obj_t*)settingsPanel_, LV_OBJ_FLAG_HIDDEN);
  if (navBg_) lv_obj_add_flag((lv_obj_t*)navBg_, LV_OBJ_FLAG_HIDDEN);
  for (int i = 0; i < kNavTabCount; ++i) {
    if (navButtons_[i]) lv_obj_add_flag((lv_obj_t*)navButtons_[i], LV_OBJ_FLAG_HIDDEN);
  }

  char line[192];
  const String liveSsid = WiFi.SSID();
  const char* shownSsid = liveSsid.length()
                              ? liveSsid.c_str()
                              : (status.wifiSsid.length() ? status.wifiSsid.c_str() : "(none)");
  snprintf(line, sizeof(line), "RSSI %d dBm · wifi %s · device %s · FW %s",
           status.wifiRssi, shownSsid, status.deviceId.c_str(), status.firmwareVersion.c_str());
  lv_label_set_text((lv_obj_t*)settingsStatus_, line);

  if (!settingsSeeded_) {
    if (settingsWifiSsid_) {
      const char* seedSsid = liveSsid.length()
                                 ? liveSsid.c_str()
                                 : status.wifiSsid.c_str();
      lv_textarea_set_text((lv_obj_t*)settingsWifiSsid_, seedSsid ? seedSsid : "");
      lv_textarea_set_placeholder_text((lv_obj_t*)settingsWifiSsid_, "network name");
    }
    if (settingsWifiPass_) {
      // Never echo stored Wi-Fi password into the UI.
      lv_textarea_set_text((lv_obj_t*)settingsWifiPass_, "");
      lv_textarea_set_placeholder_text((lv_obj_t*)settingsWifiPass_, "leave blank to keep");
    }
    lv_textarea_set_text((lv_obj_t*)settingsHost_, status.serverHost.c_str());
    char portBuf[8];
    snprintf(portBuf, sizeof(portBuf), "%u", (unsigned)status.serverPort);
    lv_textarea_set_text((lv_obj_t*)settingsPort_, portBuf);
    // Token stays blank on open — never echo stored secret into the UI.
    lv_textarea_set_text((lv_obj_t*)settingsToken_, "");
    lv_textarea_set_placeholder_text((lv_obj_t*)settingsToken_, "leave blank to keep");
    settingsSeeded_ = true;
  }
}

void UiManager::handleSettingsSave() {
  if (!settingsHost_ || !settingsPort_) return;
  dismissSettingsKeyboard();
  pendingSettingsWifiSsid_ =
      settingsWifiSsid_ ? lv_textarea_get_text((lv_obj_t*)settingsWifiSsid_) : "";
  pendingSettingsWifiSsid_.trim();
  pendingSettingsWifiPass_ =
      settingsWifiPass_ ? lv_textarea_get_text((lv_obj_t*)settingsWifiPass_) : "";
  // Do not trim password — trailing spaces can be intentional.
  pendingSettingsHost_ = lv_textarea_get_text((lv_obj_t*)settingsHost_);
  pendingSettingsHost_.trim();
  // Strip accidental URL prefix / path from paste.
  if (pendingSettingsHost_.startsWith("http://")) {
    pendingSettingsHost_.remove(0, 7);
  } else if (pendingSettingsHost_.startsWith("https://")) {
    pendingSettingsHost_.remove(0, 8);
  }
  const int slash = pendingSettingsHost_.indexOf('/');
  if (slash >= 0) pendingSettingsHost_.remove(slash);
  const int colon = pendingSettingsHost_.indexOf(':');
  // host:port pasted into host field — split if port field still default-ish.
  if (colon > 0 && pendingSettingsHost_.indexOf(':', colon + 1) < 0) {
    // Only treat as port when the suffix is all digits.
    bool digits = true;
    for (int i = colon + 1; i < (int)pendingSettingsHost_.length(); ++i) {
      if (!isDigit(pendingSettingsHost_[i])) {
        digits = false;
        break;
      }
    }
    if (digits) {
      const String portPart = pendingSettingsHost_.substring(colon + 1);
      pendingSettingsHost_ = pendingSettingsHost_.substring(0, colon);
      if (portPart.length()) {
        lv_textarea_set_text((lv_obj_t*)settingsPort_, portPart.c_str());
      }
    }
  }
  const char* portText = lv_textarea_get_text((lv_obj_t*)settingsPort_);
  long port = portText ? strtol(portText, nullptr, 10) : 0;
  if (port <= 0 || port > 65535) port = 3020;
  pendingSettingsPort_ = (uint16_t)port;
  pendingSettingsToken_ = settingsToken_ ? lv_textarea_get_text((lv_obj_t*)settingsToken_) : "";
  pendingSettingsToken_.trim();
  if (pendingSettingsHost_.length() == 0) {
    Serial.println("[settings] save ignored: empty host");
    if (settingsStatus_) {
      lv_label_set_text((lv_obj_t*)settingsStatus_, "Host required — enter IP or hostname");
    }
    return;
  }
  settingsSaveRequested_ = true;
  // Leave Settings immediately so Save → sync does not rebuild this panel
  // under the keyboard (that path looked like a crash/freeze).
  current_ = ScreenId::Home;
  settingsSeeded_ = false;
  renderRequested_ = true;
}

void UiManager::handleSettingsBack() {
  settingsBackRequested_ = true;
  dismissSettingsKeyboard();
  renderRequested_ = true;
}

bool UiManager::consumeSettingsSave(String& host, uint16_t& port, String& token,
                                    String& wifiSsid, String& wifiPass) {
  if (!settingsSaveRequested_) return false;
  settingsSaveRequested_ = false;
  host = pendingSettingsHost_;
  port = pendingSettingsPort_;
  token = pendingSettingsToken_;
  wifiSsid = pendingSettingsWifiSsid_;
  wifiPass = pendingSettingsWifiPass_;
  return true;
}

bool UiManager::consumeSettingsBack() {
  if (!settingsBackRequested_) return false;
  settingsBackRequested_ = false;
  return true;
}

void UiManager::buildAppPanel(_lv_obj_t* scrRaw) {
  lv_obj_t* scr = (lv_obj_t*)scrRaw;

  lv_obj_t* panel = lv_obj_create(scr);
  lv_obj_set_pos(panel, 24, 126);
  lv_obj_set_size(panel, PANEL_H_RES - 48, PANEL_V_RES - 126 - 96);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x0D2212), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0x1F6B2D), 0);
  lv_obj_set_style_radius(panel, 8, 0);
  lv_obj_set_style_pad_all(panel, 20, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
  appPanel_ = (_lv_obj_t*)panel;

  lv_obj_t* title = lv_label_create(panel);
  lv_label_set_text(title, "Family Hub web app");
  lv_obj_set_style_text_color(title, lv_color_hex(0x00FF41), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  lv_obj_set_pos(title, 0, 0);

  lv_obj_t* hint = lv_label_create(panel);
  lv_obj_set_width(hint, 480);
  lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
  lv_label_set_text(hint, "Scan the QR code on your phone, or open this URL in any browser.");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x8AAB8E), 0);
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_20, 0);
  lv_obj_set_pos(hint, 0, 48);
  appHintLabel_ = (_lv_obj_t*)hint;

  lv_obj_t* url = lv_label_create(panel);
  lv_obj_set_width(url, 480);
  lv_label_set_long_mode(url, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(url, lv_color_hex(0xF2F5F0), 0);
  lv_obj_set_style_text_font(url, &lv_font_montserrat_20, 0);
  lv_obj_set_pos(url, 0, 140);
  appUrlLabel_ = (_lv_obj_t*)url;

#if LV_USE_QRCODE
  lv_obj_t* qr = lv_qrcode_create(panel, 240, lv_color_hex(0x0A1A0E), lv_color_hex(0xF2F5F0));
  lv_obj_align(qr, LV_ALIGN_RIGHT_MID, -12, 10);
  appQr_ = (_lv_obj_t*)qr;
#else
  appQr_ = nullptr;
#endif
}

void UiManager::showAppPanel(const PanelStatus& status, const JsonDocument& state, bool show) {
  if (!appPanel_) return;
  if (!show) {
    lv_obj_add_flag((lv_obj_t*)appPanel_, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_clear_flag((lv_obj_t*)appPanel_, LV_OBJ_FLAG_HIDDEN);

  // Prefer public tunnel URL from the server; then compile-time default; then LAN.
  String url;
  const char* fromState = state["public_app_url"] | "";
  if (fromState && *fromState) {
    url = fromState;
  }
#if defined(DEFAULT_PUBLIC_APP_URL)
  if (url.length() == 0) {
    url = DEFAULT_PUBLIC_APP_URL;
  }
#endif
  if (url.length() == 0) {
    url = String("http://") + status.serverHost + ":" + String(status.serverPort) + "/";
  }

  if (appUrlLabel_) {
    lv_label_set_text((lv_obj_t*)appUrlLabel_, url.c_str());
  }

#if LV_USE_QRCODE
  if (appQr_ && url != appQrUrl_) {
    lv_qrcode_update((lv_obj_t*)appQr_, url.c_str(), url.length());
    appQrUrl_ = url;
  }
#endif
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
  for (int i = 0; i < kNavTabCount; ++i) {
    if (navButtons_[i] == target) {
      current_ = (ScreenId)i;
      settingsSeeded_ = false;
      renderRequested_ = true;
      lvglUpdateNav();
      return;
    }
  }
}

void UiManager::lvglUpdateNav() {
  for (int i = 0; i < kNavTabCount; ++i) {
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

void UiManager::lvglLayoutCard(int index, int x, int y, int w, int h, bool bodyLarge,
                               bool bodyScroll) {
  if (index < 0 || index >= 4 || !cards_[index]) return;
  lv_obj_t* card = (lv_obj_t*)cards_[index];
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, w, h);

  lv_obj_t* title = (lv_obj_t*)cardTitles_[index];
  lv_obj_t* viewport = (lv_obj_t*)cardBodies_[index];
  lv_obj_t* body = (lv_obj_t*)cardBodyLabels_[index];
  if (!viewport || !body) return;

  const int bodyY = bodyLarge ? 52 : 46;
  const int bodyH = h - (bodyLarge ? 66 : 58);
  const int bodyW = w - 36;

  lv_obj_set_pos(title, 18, 14);
  lv_obj_set_width(title, bodyW);

  lv_obj_set_pos(viewport, 18, bodyY);
  lv_obj_set_size(viewport, bodyW, bodyH);
  lv_obj_set_scroll_dir(viewport, LV_DIR_VER);
  lv_obj_scroll_to_y(viewport, 0, LV_ANIM_OFF);

  lv_obj_set_width(body, bodyW);
  lv_obj_set_style_text_font(body, bodyLarge ? &lv_font_montserrat_28 : &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_line_space(body, bodyLarge ? 6 : 4, 0);
  lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
  lv_obj_set_height(body, LV_SIZE_CONTENT);
  lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 0);

  if (bodyScroll) {
    lv_obj_add_flag(viewport, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(viewport, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_scrollbar_mode(viewport, LV_SCROLLBAR_MODE_ACTIVE);
  } else {
    lv_obj_clear_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(viewport, LV_SCROLLBAR_MODE_OFF);
    // Non-scroll cards clip to the viewport height.
    lv_obj_set_height(body, bodyH);
  }
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
  if (cardBodyLabels_[index]) {
    lv_obj_t* label = (lv_obj_t*)cardBodyLabels_[index];
    lv_label_set_text(label, body.c_str());
    // After text change, grow to content so the parent viewport can scroll.
    if (cardBodies_[index] &&
        lv_obj_has_flag((lv_obj_t*)cardBodies_[index], LV_OBJ_FLAG_SCROLLABLE)) {
      lv_obj_set_height(label, LV_SIZE_CONTENT);
      lv_obj_scroll_to_y((lv_obj_t*)cardBodies_[index], 0, LV_ANIM_OFF);
    }
  }
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
  if (current_ != ScreenId::Grocery) {
    // Free grocery widgets while on other screens so LVGL heap recovers.
    destroyGroceryBoard();
  } else {
    hideGroceryBoard();
  }
  const bool onSettings = (current_ == ScreenId::Settings);
  const bool onApp = (current_ == ScreenId::App);
  showSettingsPanel(status, onSettings);
  showAppPanel(status, state, onApp);
  if (onSettings || onApp) {
    displayUnlock();
    return;
  }

  if (state.isNull() || state.size() == 0) {
    lvglLayoutCard(0, 24, 126, 620, 374, true);
    lvglLayoutCard(1, 668, 126, 332, 184);
    lvglLayoutCard(2, 668, 326, 332, 184);
    lvglSetCard(0, "Waiting for Family Hub",
                "No family info loaded yet.\nMake sure the hub is running,\nthen tap Sync.");
    lvglSetCard(1, "What to try", "Tap Sync\nor open Diagnostics → Settings");
    lvglSetCard(2, "Status", String(b));
  } else {
    JsonObjectConst home = state["home"].as<JsonObjectConst>();
    JsonArrayConst chores = state["chores"]["items"].as<JsonArrayConst>();
    JsonArrayConst grocery = state["grocery"]["items"].as<JsonArrayConst>();
    JsonArrayConst notes = state["notes"]["recent"].as<JsonArrayConst>();
    JsonArrayConst pinned = state["notes"]["pinned"].as<JsonArrayConst>();
    if (pinned.isNull()) pinned = home["pinned"].as<JsonArrayConst>();
    JsonArrayConst week = state["dinner"]["week"].as<JsonArrayConst>();
    JsonObjectConst dinnerToday = state["dinner"]["today"].as<JsonObjectConst>();
    const char* meal = dinnerToday.isNull()
                           ? (home["dinner_today"].isNull() ? "No dinner planned"
                                                           : (home["dinner_today"] | "No dinner planned"))
                           : (dinnerToday["meal"] | "No dinner planned");
    const char* cook = dinnerToday.isNull()
                           ? (home["dinner_cook"].isNull() ? "Unassigned"
                                                          : (home["dinner_cook"] | "Unassigned"))
                           : (dinnerToday["cook_name"] | "Unassigned");
    const String tonightLines = dinnerTonightLines(dinnerToday, meal);
    const String manageOverview = dinnerManageOverview(dinnerToday, cook);
    const unsigned choreCount = home["open_chores_count"].is<int>()
                                    ? (unsigned)home["open_chores_count"].as<int>()
                                    : jsonArraySize(chores);
    const unsigned groceryCount = home["grocery_count"].is<int>()
                                      ? (unsigned)home["grocery_count"].as<int>()
                                      : jsonArraySize(grocery);

    switch (current_) {
      case ScreenId::Home:
        // Next Up is the primary left column (scroll); dinner sits in the
        // right stack under Today so the glanceable plan doesn't dominate.
        lvglLayoutCard(0, 24, 126, 620, 374, false, true);  // Next Up (scroll)
        lvglLayoutCard(1, 668, 126, 332, 116);
        lvglLayoutCard(3, 668, 258, 332, 252);
        lvglSetCard(0, "Next Up", homeNext(chores, grocery, pinned, notes));
        lvglSetCard(1, "Today",
                    String(choreCount) + " open chores\n" +
                    groceryCount + " grocery items");
        lvglSetCard(3, "Tonight's Dinner", tonightLines + "\nCook: " + cook);
        break;
      case ScreenId::Grocery:
        // Three equal columns: Constant | Main | Other (tap toggles).
        populateGroceryBoard(state["grocery"].as<JsonObjectConst>());
        break;
      case ScreenId::Chores:
        // Card 0's area is used by the tappable chore-row list instead of a
        // static label. Tap a row -> read-only detail -> Mark Complete.
        populateChoreRows(chores);
        lvglLayoutCard(1, 668, 126, 332, 116);
        lvglLayoutCard(3, 668, 258, 332, 252);
        lvglSetCard(1, "Open", String(jsonArraySize(chores)) + " chores");
        lvglSetCard(3, "Tap a chore", "Tap to view and mark complete.\nEdit & assign in the web app");
        break;
      case ScreenId::Dinner:
        lvglLayoutCard(0, 24, 126, 620, 160, true);
        lvglLayoutCard(1, 24, 306, 620, 194);
        lvglLayoutCard(3, 668, 126, 332, 374);
        lvglSetCard(0, "Tonight", tonightLines);
        lvglSetCard(1, "Week Ahead", dinnerWeek(week, 5));
        lvglSetCard(3, "Manage", manageOverview);
        break;
      case ScreenId::Notes:
        lvglLayoutCard(0, 24, 126, 620, 176);
        lvglLayoutCard(1, 24, 322, 620, 178);
        lvglLayoutCard(3, 668, 126, 332, 374);
        lvglSetCard(0, "Pinned", listItems(pinned, "text", 4, "No pinned notes"));
        lvglSetCard(1, "Recent Notes", listItems(notes, "text", 4, "No notes yet"));
        lvglSetCard(3, "Manage", "Add or edit notes in the\nFamily Hub web app");
        break;
      case ScreenId::App:
        // URL + QR live in appPanel_ (showAppPanel / early return).
        break;
      case ScreenId::Settings:
        // Editable fields live in settingsPanel_ (showSettingsPanel / early return).
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

#endif // WAVESHARE_7B
