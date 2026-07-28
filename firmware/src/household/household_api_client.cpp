#include "api_client.h"
#include "dashboard_state.h"

#include <esp_heap_caps.h>
#include <esp_system.h>

bool ApiClient::httpGetDashboard(const String& path, JsonDocument& outState, int& httpCode,
                                 bool sendIfNoneMatch) {
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
  if (sendIfNoneMatch && knownEtag_.length() > 0) {
    http.addHeader("If-None-Match", knownEtag_);
    Serial.printf("[api] If-None-Match %s\n", knownEtag_.c_str());
  }
  const char* headerKeys[] = {"ETag"};
  http.collectHeaders(headerKeys, 1);
  http.setConnectTimeout(2000);
  http.setTimeout(5000);
  httpCode = http.GET();
  status_.lastHttpCode = httpCode;
  Serial.printf("[api] GET status=%d\n", httpCode);

  if (httpCode == HTTP_CODE_NOT_MODIFIED) {
    http.end();
    return true;
  }

  if (httpCode == HTTP_CODE_OK) {
    const String et = http.header("ETag");
    if (et.length() > 0) knownEtag_ = et;

    DeserializationError err = deserializeJson(
        outState, http.getStream(),
        DeserializationOption::Filter(dashboardStateFilter()));
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
    if (outState["state_version"].is<int>()) {
      lastStateVersion_ = outState["state_version"].as<int>();
    }
    // Prefer body state_version for ETag when header missing.
    if (knownEtag_.length() == 0 && outState["state_version"].is<int>()) {
      knownEtag_ = String('"') + String(outState["state_version"].as<int>()) + '"';
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
  status_.lastHttpCode = httpCode;
  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
    http.end();
    return true;
  }
  if (httpCode == HTTP_CODE_CONFLICT) {
    status_.lastError = "version-conflict";
    Serial.println("[api] POST 409 version-conflict");
    http.end();
    return false;
  }
  status_.lastError = String("http-") + httpCode;
  http.end();
  return false;
}

bool ApiClient::fetchDashboardState(JsonDocument& outState, bool force) {
  const unsigned long now = millis();
  lastFetchNotModified_ = false;
  if (!force && hasAttempted_ && now - lastAttemptMs_ < backoffMs_) {
    return false;
  }
  hasAttempted_ = true;
  lastAttemptMs_ = now;

  int code = 0;
  // Always send If-None-Match when known; force still uses conditional GET —
  // a bumped state_version yields 200 after mutations.
  if (!httpGetDashboard("/api/dashboard-state", outState, code, true)) {
    status_.conn = ConnState::ServerOffline;
    tickBackoff();
    return false;
  }

  if (code == HTTP_CODE_NOT_MODIFIED) {
    lastFetchNotModified_ = true;
    status_.lastSyncMs = millis();
    status_.conn = ConnState::Online;
    status_.lastError = "";
    consecutiveFailures_ = 0;
    backoffMs_ = BACKOFF_INITIAL_MS;
    Serial.println("[api] 304 not modified — keep last dashboard");
    return true;
  }

  status_.lastSyncMs = millis();
  status_.conn = ConnState::Online;
  status_.lastError = "";
  consecutiveFailures_ = 0;
  backoffMs_ = BACKOFF_INITIAL_MS;
  return true;
}

bool ApiClient::completeChore(const String& choreId, const String& idempotencyKey,
                              int expectedStateVersion) {
  if (choreId.length() == 0) {
    status_.lastError = "empty-chore-id";
    return false;
  }
  int code = 0;
  const String path = String("/api/chores/") + choreId + "/complete";

  String body = "{";
  bool first = true;
  if (idempotencyKey.length() > 0) {
    body += "\"idempotency_key\":\"";
    body += idempotencyKey;
    body += "\"";
    first = false;
  }
  if (expectedStateVersion >= 0) {
    if (!first) body += ",";
    body += "\"expected_state_version\":";
    body += String(expectedStateVersion);
  }
  body += "}";

  Serial.printf("[api] POST complete chore key=%s expected_sv=%d\n",
                idempotencyKey.c_str(), expectedStateVersion);
  return httpPost(path, body, code);
}

bool ApiClient::toggleGrocery(const String& groceryId) {
  if (groceryId.length() == 0) {
    status_.lastError = "empty-grocery-id";
    return false;
  }
  int code = 0;
  const String path = String("/api/grocery/") + groceryId + "/toggle";
  Serial.printf("[api] POST grocery toggle id=%s\n", groceryId.c_str());
  return httpPost(path, "{}", code);
}
