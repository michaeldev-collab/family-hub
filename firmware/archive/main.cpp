#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <utility>

#include "api_client.h"
#include "state_cache.h"
#include "ui_manager.h"
#include "display.h"
#include "config.h"
#include "child_focus_state.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

ApiClient api;
StateCache cache;
UiManager ui;

JsonDocument dashboardDoc;
JsonDocument displayHomeDoc;
JsonDocument childModeDoc;
ChildFocusRuntime childRuntime;
#ifdef WAVESHARE_7B
PanelImage childFocusImage;
PanelImage childHeaderImage;
PanelImage childProfileImages[8];
PanelImage childTaskImages[6];
PanelImage childRewardImages[6];
PanelImage childRoutineImages[4];
PanelImage childStripDailyImage;
PanelImage childStripTermImage;
#endif
RTC_DATA_ATTR bool rtcChildModeActive = false;
RTC_DATA_ATTR char rtcSelectedChildId[101] = {};
bool staleData = false;
bool bootCacheLoaded = false;
unsigned long lastPollMs = 0;
unsigned long lastRenderMs = 0;
unsigned long lastWifiAttemptMs = 0;
bool syncPending = false;
bool forceSyncPending = false;
unsigned long lastChildPollMs = 0;

bool persistentCacheIoAllowed();
void requestDashboardSync(bool force);
void logMemoryBoundary(const char* boundary);
void prepareChildFocusImage();
void prepareChildPanelImages(bool selection);

void clearChildPanelImages() {
#ifdef WAVESHARE_7B
  childHeaderImage.clear();
  for (auto& image : childProfileImages) image.clear();
  for (auto& image : childTaskImages) image.clear();
  for (auto& image : childRewardImages) image.clear();
  for (auto& image : childRoutineImages) image.clear();
  childStripDailyImage.clear();
  childStripTermImage.clear();
  ui.clearChildPanelImages();
#endif
}

void prepareChildPanelImages(bool selection) {
#ifdef WAVESHARE_7B
  clearChildPanelImages();
  if (selection) {
    JsonArrayConst children = displayHomeDoc["children"].as<JsonArrayConst>();
    int i = 0;
    for (JsonObjectConst child : children) {
      if (i >= 8) break;
      const String ref = child["profileMediaRef"].as<String>();
      if (!ref.isEmpty()) api.fetchPanelImage(ref, childProfileImages[i]);
      ui.setChildProfileImage(i, childProfileImages[i].pixels ? &childProfileImages[i] : nullptr);
      ++i;
    }
    return;
  }
  const String headerRef = childModeDoc["child"]["profileMediaRef"].as<String>();
  if (!headerRef.isEmpty() && api.fetchPanelImage(headerRef, childHeaderImage)) {
    ui.setChildHeaderImage(&childHeaderImage);
  }
  int i = 0;
  for (JsonObjectConst task : childModeDoc["tasks"].as<JsonArrayConst>()) {
    if (i >= 6) break;
    const String ref = task["mediaRef"].as<String>();
    if (!ref.isEmpty()) api.fetchPanelImage(ref, childTaskImages[i]);
    ui.setChildTaskImage(i, childTaskImages[i].pixels ? &childTaskImages[i] : nullptr);
    ++i;
  }
  i = 0;
  for (JsonObjectConst reward : childModeDoc["rewards"].as<JsonArrayConst>()) {
    if (i >= 6) break;
    const String ref = reward["mediaRef"].as<String>();
    if (!ref.isEmpty()) api.fetchPanelImage(ref, childRewardImages[i]);
    ui.setChildRewardImage(i, childRewardImages[i].pixels ? &childRewardImages[i] : nullptr);
    ++i;
  }
  i = 0;
  for (JsonObjectConst routine : childModeDoc["routines"].as<JsonArrayConst>()) {
    if (i >= 4) break;
    const String ref = routine["mediaRef"].as<String>();
    if (!ref.isEmpty()) api.fetchPanelImage(ref, childRoutineImages[i]);
    ui.setChildRoutineImage(i, childRoutineImages[i].pixels ? &childRoutineImages[i] : nullptr);
    ++i;
  }
  const String dailyRef = childModeDoc["daily"]["reward"]["mediaRef"] | "";
  const String termRef = childModeDoc["term"]["reward"]["mediaRef"] | "";
  if (!dailyRef.isEmpty()) api.fetchPanelImage(dailyRef, childStripDailyImage);
  if (!termRef.isEmpty()) api.fetchPanelImage(termRef, childStripTermImage);
  ui.setChildStripImages(childStripDailyImage.pixels ? &childStripDailyImage : nullptr,
                         childStripTermImage.pixels ? &childStripTermImage : nullptr);
#endif
}

void persistChildRuntime() {
  rtcChildModeActive = childRuntime.active;
  strlcpy(rtcSelectedChildId, childRuntime.selectedChildId.c_str(), sizeof(rtcSelectedChildId));
  if (persistentCacheIoAllowed()) api.saveChildRuntime(childRuntime);
}

bool syncDisplayHome() {
  JsonDocument candidate;
  if (!api.fetchDisplayHome(candidate)) return false;
  displayHomeDoc = std::move(candidate);
  childRuntime.enabled = displayHomeDoc["mode"]["childModeAvailable"] | false;
  childRuntime.exitChallengeRequired = displayHomeDoc["mode"]["exitChallengeRequired"] | true;
  childRuntime.childSwitchChallengeRequired =
      displayHomeDoc["mode"]["childSwitchChallengeRequired"] | true;
  return true;
}

bool syncChildMode(bool allowCache = true) {
  logMemoryBoundary("child sync begin");
  if (childRuntime.selectedChildId.isEmpty()) return false;
  JsonDocument candidate;
  bool fetched = api.fetchChildMode(childRuntime.selectedChildId, candidate);
  if (fetched) {
    childModeDoc = std::move(candidate); childRuntime.stale = false;
    childRuntime.lastSuccessfulSyncMs = millis();
    if (persistentCacheIoAllowed()) cache.saveChild(childModeDoc);
  } else if (allowCache && childModeDoc.isNull() && persistentCacheIoAllowed()) {
    fetched = cache.loadChild(candidate, childRuntime.selectedChildId);
    if (fetched) { childModeDoc = std::move(candidate); childRuntime.stale = true; }
  } else if (!childModeDoc.isNull()) childRuntime.stale = true;
  if (!fetched && childModeDoc.isNull()) return false;
  const String focus = childModeDoc["focus"]["type"] | "dashboard";
  const bool onLocalPage = childRuntime.page == ChildFocusPage::Goals ||
                           childRuntime.page == ChildFocusPage::Treats ||
                           childRuntime.page == ChildFocusPage::Pin;
  if (!onLocalPage) {
    const String taskId = childModeDoc["focus"]["task"]["assignmentId"] | "";
    if (focus == "correction") childRuntime.page = ChildFocusPage::Correction;
    else if (focus == "first_then") childRuntime.page = ChildFocusPage::FirstThen;
    else if (focus == "waiting") {
      childRuntime.page = ChildFocusPage::Waiting;
      childRuntime.selectedTargetId = taskId;
    } else if (focus == "task") {
      childRuntime.page = ChildFocusPage::Task;
      childRuntime.selectedTargetId = taskId;
    } else if (focus == "celebrate") {
      childRuntime.page = ChildFocusPage::Celebrate;
      childRuntime.selectedTargetId = taskId;
    } else if (focus == "treats") childRuntime.page = ChildFocusPage::Treats;
    else childRuntime.page = ChildFocusPage::Dashboard;
  }
  persistChildRuntime();
  prepareChildPanelImages(false);
  prepareChildFocusImage();
  logMemoryBoundary("child sync committed");
  return true;
}

void prepareChildFocusImage() {
#ifdef WAVESHARE_7B
  logMemoryBoundary("child image begin");
  if (childRuntime.page == ChildFocusPage::Dashboard) {
    childFocusImage.clear();
    ui.setChildFocusImage(nullptr);
    logMemoryBoundary("child image dashboard skip");
    return;
  }
  String ref;
  if (childRuntime.page == ChildFocusPage::Task || childRuntime.page == ChildFocusPage::Waiting) {
    for (JsonObjectConst task : childModeDoc["tasks"].as<JsonArrayConst>()) {
      if (task["assignmentId"].as<String>() == childRuntime.selectedTargetId) {
        ref = task["mediaRef"].as<String>();
        break;
      }
    }
  } else if (childRuntime.page == ChildFocusPage::Celebrate) {
    ref = childModeDoc["focus"]["task"]["mediaRef"].as<String>();
  } else if (childRuntime.page == ChildFocusPage::Reward) {
    for (JsonObjectConst reward : childModeDoc["rewards"].as<JsonArrayConst>()) {
      if (reward["id"].as<String>() == childRuntime.selectedTargetId) {
        ref = reward["mediaRef"].as<String>();
        break;
      }
    }
  } else if (childRuntime.page == ChildFocusPage::Correction) {
    ref = childModeDoc["activeCorrection"]["mediaRef"].as<String>();
  } else if (childRuntime.page == ChildFocusPage::FirstThen) {
    ref = childModeDoc["focus"]["first"]["mediaRef"].as<String>();
    const String thenRef = childModeDoc["focus"]["then"]["mediaRef"] | "";
    if (!thenRef.isEmpty()) {
      if (childRewardImages[0].mediaRef != thenRef) childRewardImages[0].clear();
      if (!childRewardImages[0].pixels) api.fetchPanelImage(thenRef, childRewardImages[0]);
    } else {
      childRewardImages[0].clear();
    }
    ui.setChildRewardImage(0, childRewardImages[0].pixels ? &childRewardImages[0] : nullptr);
  } else if (childRuntime.page == ChildFocusPage::Goals) {
    ref = childModeDoc["term"]["reward"]["mediaRef"].as<String>();
  }
  if(ref.isEmpty()){
    childFocusImage.clear();
    ui.setChildFocusImage(nullptr);
  }else if(childFocusImage.mediaRef!=ref){
    childFocusImage.clear();
    if(!api.fetchPanelImage(ref,childFocusImage))childFocusImage.clear();
  }
  ui.setChildFocusImage(childFocusImage.pixels?&childFocusImage:nullptr);
  logMemoryBoundary(childFocusImage.pixels?"child image ready":"child image unavailable");
#endif
}

void enterChildMode() {
  logMemoryBoundary("child mode enter begin");
  if (!syncDisplayHome() || !childRuntime.enabled) { ui.reportChildActionResult(false,"child mode"); return; }
  childRuntime.active = true; childRuntime.lastInteractionMs = millis();
  JsonArrayConst children = displayHomeDoc["children"].as<JsonArrayConst>();
  const bool direct = displayHomeDoc["mode"]["enterDirectlyWhenSingle"] | false;
  String entry = displayHomeDoc["mode"]["entryChildId"] | "";
  if (entry.isEmpty() && direct && children.size()==1) entry=children[0]["id"].as<String>();
  if (!entry.isEmpty()) { childRuntime.selectedChildId=entry; childRuntime.page=ChildFocusPage::Dashboard; syncChildMode(); ui.renderChildMode(api.status(),childModeDoc,childRuntime); }
  else { childRuntime.page=ChildFocusPage::Selection; persistChildRuntime(); prepareChildPanelImages(true); ui.renderChildHome(api.status(),displayHomeDoc,childRuntime); }
  logMemoryBoundary("child mode enter rendered");
}

void leaveChildMode() {
  logMemoryBoundary("child mode exit begin");
  childRuntime.active=false;childRuntime.page=ChildFocusPage::Selection;childRuntime.selectedTargetId="";
  childModeDoc.clear();displayHomeDoc.clear();
#ifdef WAVESHARE_7B
  ui.clearChildFocusScreen();childFocusImage.clear();ui.setChildFocusImage(nullptr);
  clearChildPanelImages();
#endif
  persistChildRuntime();ui.setScreen(ScreenId::Home);requestDashboardSync(true);
  logMemoryBoundary("child mode exit released");
}

enum class RefreshPhase {
  Idle,
  Syncing,
  StateReady,
  Rendering,
};

RefreshPhase refreshPhase = RefreshPhase::Idle;

const char* refreshPhaseName(RefreshPhase phase) {
  switch (phase) {
    case RefreshPhase::Idle: return "IDLE";
    case RefreshPhase::Syncing: return "SYNCING";
    case RefreshPhase::StateReady: return "STATE_READY";
    case RefreshPhase::Rendering: return "RENDERING";
  }
  return "?";
}

void logMemoryBoundary(const char* boundary) {
  Serial.printf("[mem] %s phase=%s free8=%u min8=%u largest8=%u psram=%u "
                "stack_hwm=%u stack_unit=%u\n",
                boundary, refreshPhaseName(refreshPhase),
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                (unsigned)ESP.getFreePsram(),
                (unsigned)uxTaskGetStackHighWaterMark(nullptr),
                (unsigned)sizeof(StackType_t));
}

String makeDeviceId() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[32];
  snprintf(buf, sizeof(buf), "%s-%04X", DEVICE_ID_PREFIX, (uint16_t)(mac & 0xFFFF));
  return String(buf);
}

bool persistentCacheIoAllowed() {
#ifdef FAMILY_HUB_LAUNCHER_BUILD
  return false;
#elif defined(WAVESHARE_7B)
  return !displayReady();
#else
  return true;
#endif
}

bool runtimeDashboardIoAllowed() {
  return true;
}

bool runtimeWriteIoAllowed() {
#ifdef WAVESHARE_7B
  return !displayReady();
#else
  return true;
#endif
}

bool runtimeWifiReconnectAllowed() {
  // Reconnection is network I/O only. Keeping it enabled is required for the
  // panel to recover after an AP/server interruption.
  return true;
}

bool syncDashboard(bool force = false) {
  if (refreshPhase != RefreshPhase::Idle) {
    Serial.printf("[sync] deferred: lifecycle=%s\n", refreshPhaseName(refreshPhase));
    return false;
  }
  refreshPhase = RefreshPhase::Syncing;
  logMemoryBoundary("before synchronization");

  if (!runtimeDashboardIoAllowed()) {
    if (!dashboardDoc.isNull() && dashboardDoc.size() > 0) {
      if (api.status().conn != ConnState::Online) {
        staleData = true;
        api.status().conn = ConnState::Stale;
      }
      refreshPhase = RefreshPhase::Idle;
      return true;
    }
    staleData = true;
    api.status().conn = ConnState::WifiDisconnected;
    Serial.println("[net] skip network fetch while RGB panel is active");
    refreshPhase = RefreshPhase::Idle;
    return false;
  }

  JsonDocument candidate;
  const bool fetched = api.fetchDashboardState(candidate, force);
  bool candidateReady = fetched;
  bool fromCache = false;
  logMemoryBoundary("after compact state creation");

  if (fetched && persistentCacheIoAllowed()) {
    cache.save(candidate);  // stream compact state; never create a raw JSON String
  } else if (!fetched && (dashboardDoc.isNull() || dashboardDoc.size() == 0) &&
             persistentCacheIoAllowed()) {
    candidate.clear();
    candidateReady = cache.load(candidate);
    fromCache = candidateReady;
    bootCacheLoaded = candidateReady;
  } else if (!persistentCacheIoAllowed()) {
    Serial.println("[cache] runtime cache I/O disabled");
  }

  // HTTPClient, network stream, parser state, and cache File are all gone at
  // this boundary. Only the compact candidate remains. Move-assigning swaps
  // ArduinoJson pools, so no JSON-backed pointers escape the candidate.
  logMemoryBoundary("after synchronization temporaries released");
  if (candidateReady) {
    dashboardDoc = std::move(candidate);
    staleData = fromCache;
    if (fromCache) api.status().conn = ConnState::Stale;
    refreshPhase = RefreshPhase::StateReady;
    logMemoryBoundary("after atomic state commit");
    return true;
  }

  // Never clear or reparse the live state on failure. LVGL owns copies of the
  // last rendered labels, and dashboardDoc remains the last valid app state.
  staleData = true;
  if (!dashboardDoc.isNull() && dashboardDoc.size() > 0) {
    api.status().conn = ConnState::Stale;
  }
  refreshPhase = RefreshPhase::Idle;
  Serial.println("[sync] failed; preserving previous dashboard state and UI");
  return false;
}

void requestDashboardSync(bool force = false) {
  syncPending = true;
  forceSyncPending = forceSyncPending || force;
}

bool processDashboardSyncRequest() {
  if (!syncPending || refreshPhase != RefreshPhase::Idle) return false;
  const bool force = forceSyncPending;
  syncPending = false;
  forceSyncPending = false;
  return syncDashboard(force);
}

bool renderDashboard(const char* reason) {
  if (refreshPhase == RefreshPhase::Syncing || refreshPhase == RefreshPhase::Rendering) {
    Serial.printf("[render] deferred (%s): lifecycle=%s\n", reason,
                  refreshPhaseName(refreshPhase));
    return false;
  }

  // The compact serialized state is an upper bound for any single generated
  // card string. A 1 KiB floor covers fixed labels/status text when no state is
  // loaded. displayRenderMemoryAvailable requires both the Arduino String copy
  // and LVGL label copy to fit in their respective allocators.
  const size_t dynamicStatusBytes = api.status().serverHost.length() +
      api.status().deviceId.length() + api.status().lastError.length() + 128U;
  const size_t stateBytes = measureJson(dashboardDoc);
  const size_t requiredTextBytes = max((size_t)1024U, stateBytes + dynamicStatusBytes);
  logMemoryBoundary("before rendering");
  if (!displayRenderMemoryAvailable(requiredTextBytes, reason)) {
    Serial.printf("[render] deferred (%s): insufficient contiguous render memory\n", reason);
    return false;
  }

  refreshPhase = RefreshPhase::Rendering;
  ui.render(api.status(), dashboardDoc, staleData);
  refreshPhase = RefreshPhase::Idle;
  logMemoryBoundary("after rendering");
  return true;
}

// The panel's ONLY server mutation: mark an existing chore complete. Everything
// else (create/edit/delete/reassign, and all non-chore resources) is managed
// exclusively through the Family Hub web UI. Uses the dedicated, scoped
// endpoint via ApiClient::completeChore — there is no generic mutation path.
bool completeChoreById(const char* choreId) {
  if (!choreId || !*choreId) return false;
  // Chore completion is a pure network operation. The follow-up GET is queued
  // only after this POST has fully returned, so their allocations cannot overlap.
  // It is the same class of I/O as the dashboard fetch that runs with the RGB
  // panel active — so it is NOT gated by displayReady(). It requires only a live
  // WiFi link; SPI-flash cache writes stay gated elsewhere (persistentCacheIoAllowed).
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[write] chore completion unavailable — WiFi/server offline");
    return false;   // caller reports 'offline' to the modal; no optimistic update
  }
  if (!api.completeChore(choreId)) {
    Serial.println("[write] chore complete FAILED — server did not confirm");
    ui.showWriteResult(false, "chore");
    return false;
  }
  Serial.println("[write] chore complete OK — server confirmed");
  ui.showWriteResult(true, "chore");
  return true;
}

void tryWifiReconnect(unsigned long now) {
  if (!runtimeDashboardIoAllowed()) return;
  if (!runtimeWifiReconnectAllowed()) return;
  if (WiFi.status() == WL_CONNECTED) return;
  if (now - lastWifiAttemptMs < WIFI_RECONNECT_INTERVAL_MS) return;
  lastWifiAttemptMs = now;
  Serial.println("[wifi] reconnecting...");
  if (api.connectWiFi()) {
    Serial.println("[wifi] connected");
    requestDashboardSync();
  }
}

void handleSerialCommand(char c) {
  switch (c) {
    case 'n':
      ui.nextScreen();
      break;
    case 'r':
      requestDashboardSync(true);
      break;
    case 'c': {
      // Panel's only write: complete the first open chore (test hook; the
      // touch flow calls completeChoreById() the same way).
      JsonArrayConst chores = dashboardDoc["chores"].as<JsonArrayConst>();
      if (!chores.isNull() && chores.size() > 0) {
        const bool ok = completeChoreById(chores[0]["id"].as<const char*>());
        ui.reportChoreCompleteResult(ok, WiFi.status() != WL_CONNECTED);
        if (ok) requestDashboardSync(true);
      } else {
        Serial.println("[write] no open chore to complete");
      }
      break;
    }
    case 'h':
      api.setServer(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT, persistentCacheIoAllowed());
      Serial.printf("[settings] server reset to %s:%u%s\n",
                    DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT,
                    persistentCacheIoAllowed() ? "" : " (runtime only; NVS skipped while RGB active)");
      break;
    case 'w':
      api.setWriteToken("panel-test-token", persistentCacheIoAllowed());
      Serial.printf("[settings] write token set%s (panel-test-token)\n",
                    persistentCacheIoAllowed() ? " in NVS" : " runtime only; NVS skipped while RGB active");
      break;
    default:
      return;
  }
  if (c == 'h' || c == 'w') ui.setScreen(ui.screen());
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("\n[boot] Family Hub panel firmware %s\n", FIRMWARE_VERSION);
  Serial.printf("[boot] reset_reason=%d\n", (int)esp_reset_reason());

  const String deviceId = makeDeviceId();
  Serial.printf("[boot] device ID: %s\n", deviceId.c_str());
  api.begin(deviceId);
  api.loadChildRuntime(childRuntime);
  if (rtcChildModeActive) { childRuntime.active=true; childRuntime.selectedChildId=rtcSelectedChildId; }
  if (!cache.begin()) {
#ifdef FAMILY_HUB_LAUNCHER_BUILD
    Serial.println("[cache] persistent cache disabled for Launcher app image");
#else
    Serial.println("[cache] persistent cache unavailable");
#endif
  }

  const bool wifiReady = api.connectWiFi();
  if (!wifiReady) {
    Serial.println("[boot] WiFi not connected before display init — panel will use cache/offline state");
  }

  if (syncDashboard()) {
    Serial.printf("[cache] boot preload %s\n", bootCacheLoaded ? "from SPIFFS" : "from server");
  } else {
    Serial.println("[cache] no boot cache/server state available before display init");
  }
  ui.begin();
  const bool displayStarted = displayBegin();

#ifdef WAVESHARE_7B
  if (displayStarted && displayReady()) {
    Serial.println("[boot] Waveshare RGB active — dashboard sync and WiFi recovery enabled");
  } else
#endif
  {
    Serial.println("[boot] display initialization failed; backlight remains off");
    if (WiFi.status() != WL_CONNECTED && !api.connectWiFi()) {
      Serial.println("[boot] WiFi not connected — will use cache if available");
    }
  }

  if (childRuntime.active && syncDisplayHome() && childRuntime.enabled) {
    if (!childRuntime.selectedChildId.isEmpty() && syncChildMode()) ui.renderChildMode(api.status(),childModeDoc,childRuntime);
    else { prepareChildPanelImages(true); ui.renderChildHome(api.status(),displayHomeDoc,childRuntime); }
  } else renderDashboard("initial dashboard");
  lastPollMs = millis();
  // Fire the first WiFi reconnect on loop 1 (HTTP no longer self-connects), so
  // first data arrives promptly without any blocking connect during setup.
  lastWifiAttemptMs = 0;

  Serial.println("[help] keys: n=next r=refresh c=complete-first-chore h=reset-host w=set-token");
  Serial.println("[help] panel is view-only except chore completion; manage everything else in the web UI");
}

void loop() {
  const unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    api.status().conn = ConnState::WifiDisconnected;
    tryWifiReconnect(now);
  } else if (api.status().conn == ConnState::WifiDisconnected) {
    api.status().conn = ConnState::Online;
  }

  if (!childRuntime.active && now - lastPollMs >= POLL_INTERVAL_MS) {
    lastPollMs = now;
    requestDashboardSync();
  }

  if (ui.consumeSyncRequest()) {
    lastPollMs = now;
    requestDashboardSync(true);
  }

  if (ui.consumeChildModeEnterRequest() && !childRuntime.active) enterChildMode();
  String selectedChild;
  if (ui.consumeChildSelectionRequest(selectedChild) && childRuntime.active) {
    childRuntime.selectedChildId=selectedChild;childRuntime.page=ChildFocusPage::Dashboard;childRuntime.lastInteractionMs=now;
    ui.setChildDashboardTab(UiManager::ChildDashboardTab::Home);
    if(syncChildMode(false))ui.renderChildMode(api.status(),childModeDoc,childRuntime);
  }
  if (ui.consumeChildExitRequest() && childRuntime.active) {
    if (childRuntime.exitChallengeRequired) {
      ui.setChildPinPurpose("exit");
      childRuntime.page = ChildFocusPage::Pin;
      ui.renderChildMode(api.status(), childModeDoc, childRuntime);
    } else leaveChildMode();
  }
  String childPin;
  if (ui.consumeChildPinRequest(childPin) && childRuntime.active) {
    const char* purpose = ui.childPinPurpose();
    const bool ok = api.verifyParentPin(childPin, purpose);
    ui.reportChildActionResult(ok, "parent pin");
    if (ok) {
      if (strcmp(purpose, "change-child") == 0) {
        childRuntime.page = ChildFocusPage::Selection;
        childRuntime.selectedChildId = "";
        childRuntime.selectedTargetId = "";
        persistChildRuntime();
        syncDisplayHome();
        prepareChildPanelImages(true);
        ui.renderChildHome(api.status(), displayHomeDoc, childRuntime);
      } else leaveChildMode();
    } else ui.renderChildMode(api.status(), childModeDoc, childRuntime);
  }
  String childAction, childTarget;
  if (ui.consumeChildActionRequest(childAction, childTarget) && childRuntime.active) {
    childRuntime.lastInteractionMs = now;
    if (childAction == "open-goals") {
      childRuntime.page = ChildFocusPage::Goals;
      prepareChildFocusImage();
      ui.renderChildMode(api.status(), childModeDoc, childRuntime);
    } else if (childAction == "open-treats") {
      childRuntime.page = ChildFocusPage::Treats;
      prepareChildPanelImages(false);
      prepareChildFocusImage();
      ui.renderChildMode(api.status(), childModeDoc, childRuntime);
    } else if (childAction == "start-first-then") {
      const String taskId = childModeDoc["focus"]["first"]["assignmentId"] | "";
      if (!taskId.isEmpty()) {
        childRuntime.page = ChildFocusPage::Task;
        childRuntime.selectedTargetId = taskId;
        prepareChildFocusImage();
        ui.renderChildMode(api.status(), childModeDoc, childRuntime);
      }
    } else if (childAction == "back-dashboard") {
      childRuntime.page = ChildFocusPage::Dashboard;
      childRuntime.selectedTargetId = "";
      ui.setChildDashboardTab(UiManager::ChildDashboardTab::Home);
      prepareChildPanelImages(false);
      ui.renderChildMode(api.status(), childModeDoc, childRuntime);
    } else if (childAction == "tab-home" || childAction == "tab-tasks" ||
               childAction == "tab-routines" || childAction == "tab-rewards") {
      childRuntime.page = ChildFocusPage::Dashboard;
      childRuntime.selectedTargetId = "";
      if (childAction == "tab-home") ui.setChildDashboardTab(UiManager::ChildDashboardTab::Home);
      else if (childAction == "tab-tasks") ui.setChildDashboardTab(UiManager::ChildDashboardTab::Tasks);
      else if (childAction == "tab-routines") ui.setChildDashboardTab(UiManager::ChildDashboardTab::Routines);
      else ui.setChildDashboardTab(UiManager::ChildDashboardTab::Rewards);
      prepareChildPanelImages(false);
      ui.renderChildMode(api.status(), childModeDoc, childRuntime);
    } else if (childAction == "exit-home") {
      if (childRuntime.exitChallengeRequired) {
        ui.setChildPinPurpose("exit");
        childRuntime.page = ChildFocusPage::Pin;
        ui.renderChildMode(api.status(), childModeDoc, childRuntime);
      } else leaveChildMode();
    } else if (childAction == "change-child") {
      if (childRuntime.childSwitchChallengeRequired) {
        ui.setChildPinPurpose("change-child");
        childRuntime.page = ChildFocusPage::Pin;
        ui.renderChildMode(api.status(), childModeDoc, childRuntime);
      } else {
        childRuntime.page = ChildFocusPage::Selection;
        childRuntime.selectedChildId = "";
        childRuntime.selectedTargetId = "";
        persistChildRuntime();
        syncDisplayHome();
        prepareChildPanelImages(true);
        ui.renderChildHome(api.status(), displayHomeDoc, childRuntime);
      }
    } else if (childAction == "open-task") {childRuntime.page=ChildFocusPage::Task;childRuntime.selectedTargetId=childTarget;prepareChildFocusImage();ui.renderChildMode(api.status(),childModeDoc,childRuntime);}
    else if(childAction=="open-reward"){childRuntime.page=ChildFocusPage::Reward;childRuntime.selectedTargetId=childTarget;prepareChildFocusImage();ui.renderChildMode(api.status(),childModeDoc,childRuntime);}
    else {
      bool ok=false;
      if(childAction=="complete-task") ok=api.requestTaskCompletion(childRuntime.selectedChildId,childTarget,String());
      else if(childAction.startsWith("select-reward:")) ok=api.selectReward(childRuntime.selectedChildId,childTarget,childAction.substring(14));
      else if(childAction=="request-reward") ok=api.requestReward(childRuntime.selectedChildId,childTarget);
      ui.reportChildActionResult(ok,childAction.c_str());
      if(ok){if(!syncChildMode(false))childRuntime.stale=true;}
      ui.renderChildMode(api.status(),childModeDoc,childRuntime);
    }
  }
  if(childRuntime.active){
    const unsigned long refreshSeconds=displayHomeDoc["mode"]["refreshSeconds"]|60;
    if(now-lastChildPollMs>=refreshSeconds*1000UL){lastChildPollMs=now;if(syncChildMode(false))ui.renderChildMode(api.status(),childModeDoc,childRuntime);}
    const unsigned long idleSeconds=displayHomeDoc["mode"]["idleReturnSeconds"]|300;
    if (childRuntime.page != ChildFocusPage::Dashboard && childRuntime.page != ChildFocusPage::Goals &&
        childRuntime.page != ChildFocusPage::Treats &&
        now - childRuntime.lastInteractionMs >= idleSeconds * 1000UL) {
      childRuntime.page = ChildFocusPage::Dashboard;
      ui.setChildDashboardTab(UiManager::ChildDashboardTab::Home);
      prepareChildPanelImages(false);
      ui.renderChildMode(api.status(), childModeDoc, childRuntime);
    }
  }

  // Chore completion dispatched from the LVGL confirm button. HTTP runs here in
  // loop() — never inside the button callback — then the outcome is reported
  // back to the modal. On success, one forced sync is coalesced below.
  {
    String choreId;
    if (ui.consumeChoreCompleteRequest(choreId)) {
      const bool wifiUp = (WiFi.status() == WL_CONNECTED);
      const bool ok = completeChoreById(choreId.c_str());
      ui.reportChoreCompleteResult(ok, !wifiUp);
      if (ok) requestDashboardSync(true);
    }
  }

  const bool stateReady = childRuntime.active ? false : processDashboardSyncRequest();
  const bool renderRequested = ui.consumeRenderRequest();
  if (!childRuntime.active && (stateReady || renderRequested || now - lastRenderMs >= 5000)) {
    // This timestamp is an attempt cadence as well as a success timestamp, so
    // low-memory deferral retries on the existing 5 s schedule, not a hot loop.
    lastRenderMs = now;
    renderDashboard(stateReady ? "synchronized state" : "scheduled refresh");
  }

  if (Serial.available()) {
    handleSerialCommand(Serial.read());
  }

  displayTick();   // LVGL timers + touch; no-op if panel not ready
  delay(5);
}
