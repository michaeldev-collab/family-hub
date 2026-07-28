#pragma once

// HouseholdApi extraction target — wraps dashboard fetch + chore completion.
// Delegates to ApiClient until transport split lands.

#include "../api_client.h"

class HouseholdApi {
 public:
  explicit HouseholdApi(ApiClient& client) : client_(client) {}

  bool fetchDashboardState(JsonDocument& outState, bool force = false) {
    return client_.fetchDashboardState(outState, force);
  }

  bool completeChore(const String& choreId, const String& idempotencyKey = String(),
                     int expectedStateVersion = -1) {
    return client_.completeChore(choreId, idempotencyKey, expectedStateVersion);
  }

  PanelStatus& status() { return client_.status(); }

 private:
  ApiClient& client_;
};
