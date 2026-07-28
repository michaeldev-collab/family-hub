#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "config.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

enum class ConnState {
  WifiDisconnected,
  ServerOffline,
  Online,
  Stale,
};

struct PanelStatus {
  ConnState conn = ConnState::WifiDisconnected;
  String lastError;
  unsigned long lastSyncMs = 0;
  int wifiRssi = 0;
  int lastHttpCode = 0;
  String serverHost;
  uint16_t serverPort = 3020;
  String wifiSsid;
  String firmwareVersion = FIRMWARE_VERSION;
  String deviceId;
};

class ApiClient {
 public:
  void begin(const String& deviceId);
  void setServer(const String& host, uint16_t port, bool persist = true);
  void setWriteToken(const String& token, bool persist = true);
  void setWiFiCredentials(const String& ssid, const String& pass, bool persist = true);
  String wifiSsid();
  bool connectWiFi();
  // Returns true on 200 with parsed body. On 304 (not modified), returns true
  // with lastFetchNotModified()==true and leaves outState untouched.
  bool fetchDashboardState(JsonDocument& outState, bool force = false);
  bool lastFetchNotModified() const { return lastFetchNotModified_; }
  int lastStateVersion() const { return lastStateVersion_; }
  bool completeChore(const String& choreId, const String& idempotencyKey = String(),
                     int expectedStateVersion = -1);
  bool toggleGrocery(const String& groceryId);
  void tickBackoff();
  unsigned long currentBackoffMs() const { return backoffMs_; }
  PanelStatus& status() { return status_; }

 private:
  String baseUrl() const;
  void addAuthHeaders(HTTPClient& http) const;
  void addFirmwareHeaders(HTTPClient& http) const;
  bool httpGetDashboard(const String& path, JsonDocument& outState, int& httpCode,
                        bool sendIfNoneMatch);
  bool httpPost(const String& path, const String& body, int& httpCode);

  PanelStatus status_;
  Preferences prefs_;
  String writeToken_;
  String knownEtag_;
  bool lastFetchNotModified_ = false;
  int lastStateVersion_ = -1;
  unsigned long backoffMs_ = BACKOFF_INITIAL_MS;
  unsigned long lastAttemptMs_ = 0;
  bool hasAttempted_ = false;
  int consecutiveFailures_ = 0;
};
