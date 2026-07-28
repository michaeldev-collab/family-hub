#include "api_client.h"

#include <esp_heap_caps.h>
#include <esp_system.h>

static void logApiMemory(const char* boundary) {
  Serial.printf("[mem] %s free8=%u min8=%u largest8=%u psram=%u stack_hwm=%u stack_unit=%u\n",
                boundary,
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                (unsigned)ESP.getFreePsram(),
                (unsigned)uxTaskGetStackHighWaterMark(nullptr),
                (unsigned)sizeof(StackType_t));
}

void ApiClient::begin(const String& deviceId) {
  status_.deviceId = deviceId;
  prefs_.begin(NVS_NAMESPACE, false);
#if defined(FAMILY_HUB_PUBLIC_BUILD)
  // Public images never fall back to compile-time host/token/Wi-Fi secrets.
  status_.serverHost = prefs_.getString(NVS_KEY_SERVER_HOST, "");
  status_.serverPort = prefs_.getUShort(NVS_KEY_SERVER_PORT, DEFAULT_SERVER_PORT);
  writeToken_ = prefs_.getString(NVS_KEY_WRITE_TOKEN, "");
  Serial.println("[api] PUBLIC_BUILD: NVS-only server host + write token (no secrets.h bake-in)");
#else
  status_.serverHost = prefs_.getString(NVS_KEY_SERVER_HOST, DEFAULT_SERVER_HOST);
  status_.serverPort = prefs_.getUShort(NVS_KEY_SERVER_PORT, DEFAULT_SERVER_PORT);
  writeToken_ = prefs_.getString(NVS_KEY_WRITE_TOKEN, "");
#ifdef FAMILY_HUB_WRITE_TOKEN
  if (writeToken_.length() == 0) {
    writeToken_ = FAMILY_HUB_WRITE_TOKEN;
  }
#elif defined(WRITE_TOKEN)
  // Backward compatibility for existing local secrets.h files created from the
  // original template. New configurations should use FAMILY_HUB_WRITE_TOKEN.
  if (writeToken_.length() == 0) {
    writeToken_ = WRITE_TOKEN;
  }
#endif
#endif
  status_.wifiSsid = wifiSsid();
}

void ApiClient::setServer(const String& host, uint16_t port, bool persist) {
  status_.serverHost = host;
  status_.serverPort = port;
  if (persist) {
    prefs_.putString(NVS_KEY_SERVER_HOST, host);
    prefs_.putUShort(NVS_KEY_SERVER_PORT, port);
  }
}

void ApiClient::setWriteToken(const String& token, bool persist) {
  writeToken_ = token;
  if (persist) {
    prefs_.putString(NVS_KEY_WRITE_TOKEN, token);
  }
}

void ApiClient::setWiFiCredentials(const String& ssid, const String& pass, bool persist) {
  if (persist && ssid.length() > 0) {
    prefs_.putString(NVS_KEY_WIFI_SSID, ssid);
    // Empty pass is allowed (open networks); still persist so blank means "clear".
    prefs_.putString(NVS_KEY_WIFI_PASS, pass);
    status_.wifiSsid = ssid;
  }
}

String ApiClient::wifiSsid() {
  String ssid = prefs_.getString(NVS_KEY_WIFI_SSID, "");
  if (ssid.length() > 0) return ssid;
#if !defined(FAMILY_HUB_PUBLIC_BUILD)
  return String(WIFI_SSID);
#else
  return String();
#endif
}

bool ApiClient::connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    status_.wifiRssi = WiFi.RSSI();
    return true;
  }
  status_.conn = ConnState::WifiDisconnected;
  Serial.println("[wifi] connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  String ssid = prefs_.getString(NVS_KEY_WIFI_SSID, "");
  String pass = prefs_.getString(NVS_KEY_WIFI_PASS, "");
#if defined(FAMILY_HUB_PUBLIC_BUILD)
  if (ssid.length() == 0) {
    status_.lastError = "wifi-nvs-missing";
    Serial.println("[wifi] PUBLIC_BUILD: no NVS wifi_ssid (SoftAP provisioning not in this build)");
    return false;
  }
#else
  // Prefer NVS (Settings UI); fall back to compiled secrets.h.
  if (ssid.length() == 0) {
    ssid = WIFI_SSID;
    pass = WIFI_PASSWORD;
  }
#endif
  WiFi.begin(ssid.c_str(), pass.c_str());
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 6000) {
    delay(100);
  }
  if (WiFi.status() == WL_CONNECTED) {
    status_.wifiRssi = WiFi.RSSI();
    status_.wifiSsid = WiFi.SSID().length() ? WiFi.SSID() : ssid;
    Serial.printf("[wifi] connected ssid=%s ip=%s rssi=%d\n",
                  status_.wifiSsid.c_str(), WiFi.localIP().toString().c_str(), status_.wifiRssi);
    return true;
  }
  status_.lastError = "wifi-failed";
  Serial.printf("[wifi] failed ssid=%s status=%d\n", ssid.c_str(), (int)WiFi.status());
  return false;
}

String ApiClient::baseUrl() const {
  return String("http://") + status_.serverHost + ":" + String(status_.serverPort);
}

void ApiClient::tickBackoff() {
  consecutiveFailures_++;
  backoffMs_ = min(backoffMs_ * 2, (unsigned long)BACKOFF_MAX_MS);
}

void ApiClient::addAuthHeaders(HTTPClient& http) const {
  addFirmwareHeaders(http);
  if (writeToken_.length() > 0) {
    http.addHeader("x-family-hub-token", writeToken_);
  }
}

void ApiClient::addFirmwareHeaders(HTTPClient& http) const {
#if defined(FAMILY_HUB_FIRMWARE_KIND)
  http.addHeader("x-family-hub-firmware", FAMILY_HUB_FIRMWARE_KIND);
#endif
  http.addHeader("x-family-hub-version", FIRMWARE_VERSION);
}
