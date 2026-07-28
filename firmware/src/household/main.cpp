#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>

#include "api_client.h"
#include "state_cache.h"
#include "ui_manager.h"
#include "display.h"
#include "config.h"
#include "shared/diagnostics/memory.h"
#include "dashboard_state.h"
#include "shell_gestures.h"
#include "diagnostics_ui.h"
#include "sleep_button.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

ApiClient api;
StateCache cache;
UiManager ui;

JsonDocument dashboardDoc;
bool staleData = false;
bool bootCacheLoaded = false;
unsigned long lastPollMs = 0;
unsigned long lastRenderMs = 0;
unsigned long lastWifiAttemptMs = 0;
bool syncPending = false;
bool forceSyncPending = false;
bool dashboardUnchanged = false;
String pendingCompleteChoreId;
String pendingCompleteIdemKey;

RefreshPhase refreshPhase = RefreshPhase::Idle;

bool persistentCacheIoAllowed();
void requestDashboardSync(bool force = false);
bool renderDashboard(const char* reason);
bool openSettingsFromDiag();
void refreshDiagnosticsOverlay();
void toggleDisplaySleepFromShell();

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

bool runtimeWifiReconnectAllowed() {
  return true;
}

bool openSettingsFromDiag() {
  ui.setScreen(ScreenId::Settings);
  return true;
}

void refreshDiagnosticsOverlay() {
  if (!diagnosticsUiVisible() || displayIsSleeping()) return;
  diagnosticsUiRender(api.status(), dashboardStateValid(dashboardDoc),
                      dashboardSchemaVersion(dashboardDoc), openSettingsFromDiag);
}

void toggleDisplaySleepFromShell() {
  if (displayIsSleeping()) {
    displayWake();
    if (diagnosticsUiVisible()) {
      refreshDiagnosticsOverlay();
    } else {
      renderDashboard("wake");
    }
  } else {
    diagnosticsUiHide();
    displayEnterSleep();
  }
}

bool syncDashboard(bool force = false) {
  if (refreshPhase != RefreshPhase::Idle) {
    Serial.printf("[sync] deferred: lifecycle=%s\n", refreshPhaseName(refreshPhase));
    return false;
  }
  refreshPhase = RefreshPhase::Syncing;
  logMemoryBoundary("before synchronization", refreshPhase);

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
  if (fetched && api.lastFetchNotModified()) {
    dashboardUnchanged = true;
    staleData = false;
    refreshPhase = RefreshPhase::Idle;
    logMemoryBoundary("after 304 keep-last", refreshPhase);
    return false;  // not StateReady — skip full UI rebuild
  }

  bool candidateReady = fetched;
  bool fromCache = false;
  dashboardUnchanged = false;
  logMemoryBoundary("after compact state creation", refreshPhase);

  if (fetched && persistentCacheIoAllowed()) {
    cache.save(candidate);
  } else if (!fetched && (dashboardDoc.isNull() || dashboardDoc.size() == 0) &&
             persistentCacheIoAllowed()) {
    candidate.clear();
    candidateReady = cache.load(candidate);
    fromCache = candidateReady;
    bootCacheLoaded = candidateReady;
  } else if (!persistentCacheIoAllowed()) {
    Serial.println("[cache] runtime cache I/O disabled");
  }

  logMemoryBoundary("after synchronization temporaries released", refreshPhase);
  if (candidateReady) {
    dashboardDoc = std::move(candidate);
    staleData = fromCache;
    if (fromCache) api.status().conn = ConnState::Stale;
    refreshPhase = RefreshPhase::StateReady;
    logMemoryBoundary("after atomic state commit", refreshPhase);
    return true;
  }

  staleData = api.status().conn != ConnState::Online;
  if (!dashboardDoc.isNull() && dashboardDoc.size() > 0) {
    if (staleData) api.status().conn = ConnState::Stale;
  }
  refreshPhase = RefreshPhase::Idle;
  Serial.println("[sync] failed; preserving previous dashboard state and UI");
  return false;
}

void requestDashboardSync(bool force) {
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
  if (displayIsSleeping()) return false;
  if (refreshPhase == RefreshPhase::Syncing || refreshPhase == RefreshPhase::Rendering) {
    Serial.printf("[render] deferred (%s): lifecycle=%s\n", reason,
                  refreshPhaseName(refreshPhase));
    return false;
  }

  const size_t dynamicStatusBytes = api.status().serverHost.length() +
      api.status().deviceId.length() + api.status().lastError.length() + 128U;
  const size_t stateBytes = measureJson(dashboardDoc);
  const size_t requiredTextBytes = max((size_t)1024U, stateBytes + dynamicStatusBytes);
  logMemoryBoundary("before rendering", refreshPhase);
  if (!displayRenderMemoryAvailable(requiredTextBytes, reason)) {
    Serial.printf("[render] deferred (%s): insufficient contiguous render memory\n", reason);
    return false;
  }

  refreshPhase = RefreshPhase::Rendering;
  ui.render(api.status(), dashboardDoc, staleData);
  refreshPhase = RefreshPhase::Idle;
  logMemoryBoundary("after rendering", refreshPhase);
  refreshDiagnosticsOverlay();
  return true;
}

bool completeChoreById(const char* choreId) {
  if (!choreId || !*choreId) return false;
  if (displayIsSleeping()) return false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[write] chore completion unavailable — WiFi/server offline");
    return false;
  }

  // Reuse idempotency key across retries of the same chore so duplicate POSTs
  // are safe if the first response was lost.
  String key;
  if (pendingCompleteChoreId == choreId && pendingCompleteIdemKey.length() > 0) {
    key = pendingCompleteIdemKey;
  } else {
    char buf[48];
    snprintf(buf, sizeof(buf), "p-%lu-%08lx",
             static_cast<unsigned long>(millis()),
             static_cast<unsigned long>(esp_random()));
    key = buf;
    pendingCompleteChoreId = choreId;
    pendingCompleteIdemKey = key;
  }

  const int expectedSv = api.lastStateVersion();
  if (!api.completeChore(choreId, key, expectedSv)) {
    const bool conflict = api.status().lastError == "version-conflict";
    Serial.printf("[write] chore complete FAILED — %s\n",
                  conflict ? "409 version-conflict (will resync)" : "server did not confirm");
    if (conflict) {
      // Stale expected_state_version — drop key so a fresh attempt can proceed
      // after dashboard catch-up; request forced sync.
      pendingCompleteChoreId = "";
      pendingCompleteIdemKey = "";
      requestDashboardSync(true);
    }
    ui.showWriteResult(false, "chore");
    return false;
  }
  pendingCompleteChoreId = "";
  pendingCompleteIdemKey = "";
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
      JsonArrayConst chores = dashboardDoc["chores"]["items"].as<JsonArrayConst>();
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
    case 'z':
      toggleDisplaySleepFromShell();
      break;
    case 'u':
      displayForceBacklightOn();
      if (!displayIsSleeping()) {
        renderDashboard("force-bl");
      }
      break;
    case 'B':
      displayBacklightProbe();
      break;
    case 'd':
      diagnosticsUiToggle();
      refreshDiagnosticsOverlay();
      break;
    default:
      return;
  }
  if (c == 'h') ui.setScreen(ui.screen());
}

String makeDeviceId() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[32];
  snprintf(buf, sizeof(buf), "%s-%04X", DEVICE_ID_PREFIX, (uint16_t)(mac & 0xFFFF));
  return String(buf);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("\n[boot] Family Hub household firmware %s\n", FIRMWARE_VERSION);
  Serial.printf("[boot] reset_reason=%d\n", (int)esp_reset_reason());

  const String deviceId = makeDeviceId();
  Serial.printf("[boot] device ID: %s\n", deviceId.c_str());
  api.begin(deviceId);

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
    shellGesturesBegin();
    sleepButtonInit();
    diagnosticsUiBegin();
  } else
#endif
  {
    Serial.println("[boot] display initialization failed; backlight remains off");
    if (WiFi.status() != WL_CONNECTED && !api.connectWiFi()) {
      Serial.println("[boot] WiFi not connected — will use cache if available");
    }
  }

  const bool dashboardOk = dashboardStateValid(dashboardDoc);
#ifdef WAVESHARE_7B
  if (displayReady() && !dashboardOk && !bootCacheLoaded) {
    Serial.println("[boot] no valid dashboard — opening Diagnostics");
    diagnosticsUiToggle();
    refreshDiagnosticsOverlay();
  } else
#endif
  {
    renderDashboard("initial dashboard");
  }

  lastPollMs = millis();
  lastWifiAttemptMs = 0;
  logMemoryBoundary("boot complete", refreshPhase);

  Serial.println("[help] keys: n=next r=refresh c=complete-first-chore h=reset-host z=sleep u=unstick-BL B=BL-probe d=diag");
  Serial.println("[help] household panel — GP6 button or header double-tap = sleep/wake");
}

void loop() {
  const unsigned long now = millis();

  // Physical sleep button first — works while backlight/touch are off.
#ifdef WAVESHARE_7B
  sleepButtonUpdate();
  if (sleepButtonConsumeToggle()) {
    toggleDisplaySleepFromShell();
  }
#endif

  // Gestures: sleep toggle + diagnostics hold.
#ifdef WAVESHARE_7B
  if (displayReady()) {
    const bool modalBlocks = ui.modalOpen();
    const ShellGesture gest = shellGesturesPoll(modalBlocks && !displayIsSleeping());
    if (gest == ShellGesture::SleepToggle) {
      toggleDisplaySleepFromShell();
    } else if (gest == ShellGesture::DiagnosticsToggle && !displayIsSleeping()) {
      diagnosticsUiToggle();
      refreshDiagnosticsOverlay();
    }
  }
#endif

  if (displayIsSleeping()) {
    // Keep Wi-Fi reconnect; skip poll/render/chore while asleep.
    if (WiFi.status() != WL_CONNECTED) {
      api.status().conn = ConnState::WifiDisconnected;
      tryWifiReconnect(now);
    }
    if (Serial.available()) {
      handleSerialCommand(Serial.read());
    }
    displayTick();
    delay(5);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    api.status().conn = ConnState::WifiDisconnected;
    tryWifiReconnect(now);
  } else if (api.status().conn == ConnState::WifiDisconnected) {
    api.status().conn = ConnState::Online;
  }

  if (now - lastPollMs >= POLL_INTERVAL_MS) {
    lastPollMs = now;
    requestDashboardSync();
  }

  if (ui.consumeSyncRequest()) {
    lastPollMs = now;
    requestDashboardSync(true);
  }

  {
    String choreId;
    if (ui.consumeChoreCompleteRequest(choreId)) {
      const bool wifiUp = (WiFi.status() == WL_CONNECTED);
      const bool ok = completeChoreById(choreId.c_str());
      ui.reportChoreCompleteResult(ok, !wifiUp);
      if (ok) requestDashboardSync(true);
    }
  }

  {
    String groceryId;
    if (ui.consumeGroceryToggleRequest(groceryId)) {
      const bool ok = api.toggleGrocery(groceryId);
      ui.showWriteResult(ok, "grocery");
      if (ok) requestDashboardSync(true);
    }
  }

  {
    String host, token, wifiSsid, wifiPass;
    uint16_t port = 0;
    if (ui.consumeSettingsSave(host, port, token, wifiSsid, wifiPass)) {
      if (host.length() > 0) {
        api.setServer(host, port, true);
        Serial.printf("[settings] saved server %s:%u\n", host.c_str(), (unsigned)port);
      }
      if (token.length() > 0) {
        api.setWriteToken(token, true);
        Serial.println("[settings] write token updated (NVS; value not logged)");
      }
      bool wifiChanged = false;
      if (wifiSsid.length() > 0) {
        // Blank password field means keep existing NVS password.
        String passToStore = wifiPass;
        if (wifiPass.length() == 0) {
          // Re-read current stored pass without exposing it to UI.
          // Empty SSID save with blank pass is ignored above; here SSID is set.
          // If no prior NVS pass exists, store empty (open AP / user will re-enter).
          Preferences prefs;
          prefs.begin(NVS_NAMESPACE, true);
          passToStore = prefs.getString(NVS_KEY_WIFI_PASS, "");
          prefs.end();
        }
        api.setWiFiCredentials(wifiSsid, passToStore, true);
        Serial.printf("[settings] wifi ssid saved (%s); reconnecting\n", wifiSsid.c_str());
        wifiChanged = true;
      }
      if (wifiChanged) {
        WiFi.disconnect(false, false);
        delay(100);
        const bool ok = api.connectWiFi();
        ui.showWriteResult(ok, ok ? "wifi" : "wifi-fail");
      } else {
        ui.showWriteResult(true, "settings");
      }
      requestDashboardSync(true);
    }
    if (ui.consumeSettingsBack()) {
      ui.setScreen(ScreenId::Home);
    }
  }

  const bool stateReady = processDashboardSyncRequest();
  const bool renderRequested = ui.consumeRenderRequest();
  // Idle 304 polls skip rebuild; still honor explicit render requests and
  // periodic refresh only when content may have changed.
  const bool scheduled =
      !dashboardUnchanged && (now - lastRenderMs >= 5000);
  if (stateReady || renderRequested || scheduled) {
    lastRenderMs = now;
    renderDashboard(stateReady ? "synchronized state" : "scheduled refresh");
  }

  if (Serial.available()) {
    handleSerialCommand(Serial.read());
  }

  displayTick();
  delay(5);
}
