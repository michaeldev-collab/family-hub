#include "api_client.h"
#ifndef FAMILY_HUB_APP_CHILD
#include "dashboard_state.h"
#endif
#ifndef FAMILY_HUB_APP_HOUSEHOLD
#include "child_focus_state.h"
#endif

#include <esp_heap_caps.h>
#include <esp_system.h>
#include <mbedtls/md.h>
#ifdef WAVESHARE_7B
#include <esp32s3/rom/tjpgd.h>
#include <esp_heap_caps.h>
#endif

#ifdef WAVESHARE_7B
#ifndef FAMILY_HUB_APP_HOUSEHOLD
void PanelImage::clear() {
  if (pixels) free(pixels);
  pixels = nullptr; mediaRef = ""; descriptor = {};
}

struct PanelJpegContext {
  const uint8_t* encoded;
  size_t encodedSize;
  size_t offset;
  uint16_t* output;
};

static UINT panelJpegInput(JDEC* decoder, BYTE* buffer, UINT requested) {
  PanelJpegContext* context = (PanelJpegContext*)decoder->device;
  const size_t available = context->encodedSize - context->offset;
  const size_t count = min((size_t)requested, available);
  if (buffer && count) memcpy(buffer, context->encoded + context->offset, count);
  context->offset += count;
  return (UINT)count;
}

static UINT panelJpegOutput(JDEC* decoder, void* bitmap, JRECT* rect) {
  PanelJpegContext* context = (PanelJpegContext*)decoder->device;
  const uint8_t* rgb = (const uint8_t*)bitmap;
  const unsigned width = rect->right - rect->left + 1;
  const unsigned height = rect->bottom - rect->top + 1;
  for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
    const size_t source = (y * width + x) * 3;
    const uint8_t r = rgb[source], g = rgb[source + 1], b = rgb[source + 2];
    context->output[(rect->top + y) * 128U + rect->left + x] =
      (uint16_t)(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
  }
  return 1;
}
#endif
#endif

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
  status_.serverHost = prefs_.getString(NVS_KEY_SERVER_HOST, DEFAULT_SERVER_HOST);
  status_.serverPort = prefs_.getUShort(NVS_KEY_SERVER_PORT, DEFAULT_SERVER_PORT);
  writeToken_ = prefs_.getString(NVS_KEY_WRITE_TOKEN, "");
#ifdef FAMILY_HUB_WRITE_TOKEN
  if (writeToken_.length() == 0) {
    writeToken_ = FAMILY_HUB_WRITE_TOKEN;
  }
#endif
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

bool ApiClient::connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    status_.wifiRssi = WiFi.RSSI();
    return true;
  }
  status_.conn = ConnState::WifiDisconnected;
  Serial.println("[wifi] connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);          // don't thrash NVS on every connect
  WiFi.setSleep(false);            // keep the dashboard link steady while RGB/LVGL is active
  WiFi.setAutoReconnect(true);     // let the stack recover drops in the background
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const unsigned long start = millis();
  // Bounded: a stalled associate must not freeze the UI task for long. The
  // display keeps rendering the last-known dashboard; reconnect retries later.
  while (WiFi.status() != WL_CONNECTED && millis() - start < 6000) {
    delay(100);
  }
  if (WiFi.status() == WL_CONNECTED) {
    status_.wifiRssi = WiFi.RSSI();
    Serial.printf("[wifi] connected ip=%s rssi=%d\n",
                  WiFi.localIP().toString().c_str(), status_.wifiRssi);
    return true;
  }
  status_.lastError = "wifi-failed";
  Serial.printf("[wifi] failed status=%d\n", (int)WiFi.status());
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

#ifndef FAMILY_HUB_APP_HOUSEHOLD
String ApiClient::displayToken() const {
  if (writeToken_.isEmpty()) return String();
  const String message = String("family-hub-panel:") + status_.deviceId;
  unsigned char digest[32] = {};
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!info || mbedtls_md_setup(&ctx, info, 1) != 0 ||
      mbedtls_md_hmac_starts(&ctx, (const unsigned char*)writeToken_.c_str(), writeToken_.length()) != 0 ||
      mbedtls_md_hmac_update(&ctx, (const unsigned char*)message.c_str(), message.length()) != 0 ||
      mbedtls_md_hmac_finish(&ctx, digest) != 0) {
    mbedtls_md_free(&ctx);
    return String();
  }
  mbedtls_md_free(&ctx);
  static const char hex[] = "0123456789abcdef";
  char encoded[65];
  for (int i = 0; i < 32; ++i) { encoded[i * 2] = hex[digest[i] >> 4]; encoded[i * 2 + 1] = hex[digest[i] & 15]; }
  encoded[64] = '\0';
  return String(encoded);
}

void ApiClient::addDisplayAuthHeaders(HTTPClient& http) const {
  addFirmwareHeaders(http);
  const String token = displayToken();
  if (!token.isEmpty()) http.addHeader("x-family-hub-token", token);
  http.addHeader("x-family-hub-device-id", status_.deviceId);
}

String ApiClient::idempotencyKey() const {
  char value[33];
  snprintf(value, sizeof(value), "%08lx-%08lx-%08lx-%08lx",
           (unsigned long)esp_random(), (unsigned long)esp_random(),
           (unsigned long)esp_random(), (unsigned long)esp_random());
  return String(value);
}

bool ApiClient::httpDisplayGet(const String& path, const JsonDocument& filter,
                               JsonDocument& outState, int& httpCode) {
  if (WiFi.status() != WL_CONNECTED) { status_.lastError = "wifi-down"; return false; }
  HTTPClient http;
  WiFiClient client;
  if (!http.begin(client, baseUrl() + path)) { status_.lastError = "http-begin-failed"; return false; }
  addDisplayAuthHeaders(http);
  http.setConnectTimeout(2000);
  http.setTimeout(5000);
  httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) { status_.lastError = String("http-") + httpCode; http.end(); return false; }
  const DeserializationError err = deserializeJson(outState, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { status_.lastError = String("json-") + err.c_str(); return false; }
  outState.shrinkToFit();
  return true;
}

bool ApiClient::httpDisplayPost(const String& path, const String& body, int& httpCode) {
  if (WiFi.status() != WL_CONNECTED) { status_.lastError = "wifi-down"; return false; }
  HTTPClient http;
  WiFiClient client;
  if (!http.begin(client, baseUrl() + path)) { status_.lastError = "http-begin-failed"; return false; }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Idempotency-Key", idempotencyKey());
  addDisplayAuthHeaders(http);
  http.setConnectTimeout(2000);
  http.setTimeout(5000);
  httpCode = http.POST(body);
  const bool ok = httpCode >= 200 && httpCode < 300;
  if (!ok) status_.lastError = String("http-") + httpCode;
  http.end();
  return ok;
}
#endif

#ifndef FAMILY_HUB_APP_CHILD
bool ApiClient::httpGetDashboard(const String& path, JsonDocument& outState, int& httpCode) {
  if (WiFi.status() != WL_CONNECTED) { status_.lastError = "wifi-down"; return false; }

  HTTPClient http;
  WiFiClient client;
  const String url = baseUrl() + path;
  Serial.printf("[api] GET %s\n", url.c_str());
  if (!http.begin(client, url)) {
    status_.lastError = "http-begin-failed";
    Serial.println("[api] http begin failed");
    http.end();
    return false;
  }
  addFirmwareHeaders(http);
  http.setConnectTimeout(2000);
  http.setTimeout(5000);
  httpCode = http.GET();
  Serial.printf("[api] GET status=%d\n", httpCode);
  logApiMemory("after HTTP fetch headers");
  if (httpCode == HTTP_CODE_OK) {
    DeserializationError err = deserializeJson(
        outState, http.getStream(),
        DeserializationOption::Filter(dashboardStateFilter()));
    logApiMemory("after JSON parse");
    http.end();
    if (err) {
      status_.lastError = String("json-") + err.c_str();
      Serial.printf("[api] dashboard JSON parse failed: %s\n", err.c_str());
      return false;
    }
    if (!dashboardStateValid(outState)) {
      status_.lastError = "json-shape-invalid";
      Serial.println("[api] dashboard JSON shape invalid");
      return false;
    }
    outState.shrinkToFit();
    return true;
  }
  status_.lastError = String("http-") + httpCode;
  http.end();
  return false;
}

bool ApiClient::httpPost(const String& path, const String& body, int& httpCode) {
  if (WiFi.status() != WL_CONNECTED) { status_.lastError = "wifi-down"; return false; }

  HTTPClient http;
  WiFiClient client;
  const String url = baseUrl() + path;
  if (!http.begin(client, url)) {
    status_.lastError = "http-begin-failed";
    http.end();
    return false;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-family-hub-source", "panel");
  addAuthHeaders(http);
  http.setConnectTimeout(2000);
  http.setTimeout(5000);
  httpCode = http.POST(body);
  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
    http.end();
    return true;
  }
  status_.lastError = String("http-") + httpCode;
  http.end();
  return false;
}

bool ApiClient::fetchDashboardState(JsonDocument& outState, bool force) {
  const unsigned long now = millis();
  if (!force && hasAttempted_ && now - lastAttemptMs_ < backoffMs_) {
    return false;
  }
  hasAttempted_ = true;
  lastAttemptMs_ = now;

  int code = 0;
  if (!httpGetDashboard("/api/dashboard-state", outState, code)) {
    status_.conn = ConnState::ServerOffline;
    tickBackoff();
    return false;
  }

  status_.lastSyncMs = millis();
  status_.conn = ConnState::Online;
  status_.lastError = "";
  consecutiveFailures_ = 0;
  backoffMs_ = BACKOFF_INITIAL_MS;
  return true;
}

bool ApiClient::completeChore(const String& choreId) {
  if (choreId.length() == 0) {
    status_.lastError = "empty-chore-id";
    return false;
  }
  // User-initiated, discrete action — deliberately independent of the periodic
  // dashboard-fetch backoff so a tap is never silently swallowed by the poll
  // timer. Does not touch backoffMs_/lastAttemptMs_.
  int code = 0;
  const String path = String("/api/chores/") + choreId + "/complete";
  return httpPost(path, "{}", code);
}
#endif

#ifndef FAMILY_HUB_APP_HOUSEHOLD
bool ApiClient::fetchDisplayHome(JsonDocument& outState) {
  int code = 0;
  if (!httpDisplayGet("/api/v1/display/rewards-behavior/home", displayHomeFilter(), outState, code)) return false;
  if (!displayHomeValid(outState)) { status_.lastError = "display-home-invalid"; outState.clear(); return false; }
  return true;
}

bool ApiClient::fetchChildMode(const String& childId, JsonDocument& outState) {
  int code = 0;
  if (!httpDisplayGet(String("/api/v1/display/child-mode/") + childId, childModeFilter(), outState, code)) return false;
  if (!childModeValid(outState, childId)) { status_.lastError = "child-mode-invalid"; outState.clear(); return false; }
  return true;
}

bool ApiClient::requestTaskCompletion(const String& childId, const String& assignmentId,
                                      const String& occurrenceDate) {
  JsonDocument body;
  body["childId"] = childId; body["assignmentId"] = assignmentId;
  if (!occurrenceDate.isEmpty()) body["occurrenceDate"] = occurrenceDate;
  String encoded; serializeJson(body, encoded); int code = 0;
  return httpDisplayPost("/api/v1/display/actions/complete-task", encoded, code);
}

bool ApiClient::selectReward(const String& childId, const String& rewardId,
                             const String& rewardType) {
  JsonDocument body;
  body["childId"] = childId; body["rewardId"] = rewardId;
  body["goalType"] = rewardType;
  String encoded; serializeJson(body, encoded); int code = 0;
  return httpDisplayPost("/api/v1/display/actions/select-reward", encoded, code);
}

bool ApiClient::requestReward(const String& childId, const String& goalId) {
  JsonDocument body; body["childId"] = childId; body["goalId"] = goalId;
  String encoded; serializeJson(body, encoded); int code = 0;
  return httpDisplayPost("/api/v1/display/actions/request-reward", encoded, code);
}

bool ApiClient::verifyParentPin(const String& pin, const String& purpose) {
  JsonDocument body; body["pin"] = pin; body["purpose"] = purpose;
  String encoded; serializeJson(body, encoded); int code = 0;
  return httpDisplayPost("/api/v1/display/actions/verify-parent-pin", encoded, code);
}

#ifdef WAVESHARE_7B
bool ApiClient::fetchPanelImage(const String& mediaRef, PanelImage& image) {
  if (mediaRef.isEmpty()) return false;
  if (image.pixels && image.mediaRef == mediaRef) return true;
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http; WiFiClient client;
  if (!http.begin(client, baseUrl() + mediaRef)) return false;
  addDisplayAuthHeaders(http); http.setConnectTimeout(2000); http.setTimeout(5000);
  const int code = http.GET(); const int length = http.getSize();
  if (code != HTTP_CODE_OK || length < 4 || length > 96 * 1024) { http.end(); return false; }
  uint8_t* encoded = (uint8_t*)heap_caps_malloc(length, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  if (!encoded) { http.end();return false; }
  const size_t received = http.getStream().readBytes(encoded, length); http.end();
  if (received != (size_t)length || encoded[0] != 0xff || encoded[1] != 0xd8) { free(encoded);return false; }
  const size_t outputNeeded = 128U * 128U * 2U;
  uint8_t* decoded = (uint8_t*)heap_caps_malloc(outputNeeded, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  uint8_t* work = (uint8_t*)heap_caps_malloc(4096, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  if (!decoded || !work) { free(encoded);if(decoded)free(decoded);if(work)free(work);return false; }
  PanelJpegContext context = { encoded, (size_t)length, 0, (uint16_t*)decoded };
  JDEC decoder = {};
  const bool prepared = jd_prepare(&decoder, panelJpegInput, work, 4096, &context) == JDR_OK;
  const bool dimensions = prepared && decoder.width == 128 && decoder.height == 128;
  const bool ok = dimensions && jd_decomp(&decoder, panelJpegOutput, 0) == JDR_OK;
  free(work); free(encoded);
  if (!ok) { free(decoded);return false; }
  image.clear(); image.pixels = decoded; image.mediaRef = mediaRef;
  image.descriptor.header.always_zero = 0; image.descriptor.header.w = 128; image.descriptor.header.h = 128;
  image.descriptor.header.cf = LV_IMG_CF_TRUE_COLOR; image.descriptor.data_size = outputNeeded; image.descriptor.data = decoded;
  return true;
}
#endif

void ApiClient::loadChildRuntime(ChildFocusRuntime& runtime) {
  runtime.active = prefs_.getBool(NVS_KEY_CHILD_MODE, false);
  runtime.selectedChildId = prefs_.getString(NVS_KEY_CHILD_ID, "");
  const String page = prefs_.getString(NVS_KEY_CHILD_PAGE, "selection");
  runtime.page = page == "task" ? ChildFocusPage::Task
    : page == "waiting" ? ChildFocusPage::Waiting
    : page == "celebrate" ? ChildFocusPage::Celebrate
    : page == "treats" ? ChildFocusPage::Treats
    : page == "reward" ? ChildFocusPage::Reward
    : page == "goals" ? ChildFocusPage::Goals
    : page == "correction" ? ChildFocusPage::Correction
    : page == "first_then" ? ChildFocusPage::FirstThen
    : page == "pin" ? ChildFocusPage::Pin
    : page == "dashboard" ? ChildFocusPage::Dashboard
    : ChildFocusPage::Selection;
  runtime.lastSuccessfulSyncMs = prefs_.getULong(NVS_KEY_CHILD_SYNC, 0);
}

void ApiClient::saveChildRuntime(const ChildFocusRuntime& runtime) {
  prefs_.putBool(NVS_KEY_CHILD_MODE, runtime.active);
  prefs_.putString(NVS_KEY_CHILD_ID, runtime.selectedChildId);
  prefs_.putString(NVS_KEY_CHILD_PAGE, childFocusPageName(runtime.page));
  prefs_.putULong(NVS_KEY_CHILD_SYNC, runtime.lastSuccessfulSyncMs);
}
#endif
