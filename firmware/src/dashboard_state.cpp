#include "dashboard_state.h"

namespace {
constexpr int kSupportedSchemaVersion = 1;
}

const JsonDocument& dashboardStateFilter() {
  static JsonDocument filter;
  static bool initialized = false;
  if (!initialized) {
    filter["schema_version"] = true;
    filter["state_version"] = true;
    filter["generated_at"] = true;
    filter["generatedAt"] = true;
    filter["server_version"] = true;
    filter["serverVersion"] = true;
    filter["today"] = true;

    filter["home"]["dinner_today"] = true;
    filter["home"]["dinner_cook"] = true;
    filter["home"]["open_chores_count"] = true;
    filter["home"]["grocery_count"] = true;
    filter["home"]["pinned"][0]["text"] = true;
    filter["home"]["badge"] = true;

    filter["grocery"]["items"][0]["id"] = true;
    filter["grocery"]["items"][0]["text"] = true;
    filter["grocery"]["items"][0]["checked"] = true;
    filter["grocery"]["other_title"] = true;
    filter["grocery"]["constant"][0]["id"] = true;
    filter["grocery"]["constant"][0]["text"] = true;
    filter["grocery"]["constant"][0]["needed"] = true;
    filter["grocery"]["main"][0]["id"] = true;
    filter["grocery"]["main"][0]["text"] = true;
    filter["grocery"]["main"][0]["checked"] = true;
    filter["grocery"]["other"][0]["id"] = true;
    filter["grocery"]["other"][0]["text"] = true;
    filter["grocery"]["other"][0]["checked"] = true;

    filter["chores"]["items"][0]["id"] = true;
    filter["chores"]["items"][0]["title"] = true;
    filter["chores"]["items"][0]["assignee_name"] = true;

    filter["dinner"]["today"]["date"] = true;
    filter["dinner"]["today"]["meal"] = true;
    filter["dinner"]["today"]["main"] = true;
    filter["dinner"]["today"]["side"] = true;
    filter["dinner"]["today"]["side2"] = true;
    filter["dinner"]["today"]["cook_name"] = true;
    filter["dinner"]["today"]["notes"] = true;
    filter["dinner"]["week"][0]["date"] = true;
    filter["dinner"]["week"][0]["main"] = true;
    filter["dinner"]["week"][0]["meal"] = true;

    filter["notes"]["pinned"][0]["text"] = true;
    filter["notes"]["recent"][0]["text"] = true;

    filter["connection"]["sourceOfTruth"] = true;
    filter["public_app_url"] = true;

    filter.shrinkToFit();
    initialized = true;
  }
  return filter;
}

bool dashboardStateValid(const JsonDocument& state) {
  if (!state.is<JsonObjectConst>()) return false;
  JsonObjectConst root = state.as<JsonObjectConst>();

  if (!root["schema_version"].is<int>()) return false;
  if (root["schema_version"].as<int>() != kSupportedSchemaVersion) return false;

  if (!root["home"].is<JsonObjectConst>()) return false;
  if (!root["grocery"].is<JsonObjectConst>()) return false;
  if (!root["chores"].is<JsonObjectConst>()) return false;
  if (!root["dinner"].is<JsonObjectConst>()) return false;
  if (!root["notes"].is<JsonObjectConst>()) return false;

  if (!root["grocery"]["items"].is<JsonArrayConst>()) return false;
  if (!root["chores"]["items"].is<JsonArrayConst>()) return false;
  if (!root["dinner"]["week"].is<JsonArrayConst>()) return false;
  if (!root["notes"]["pinned"].is<JsonArrayConst>()) return false;
  if (!root["notes"]["recent"].is<JsonArrayConst>()) return false;

  // dinner.today may be null; if present must be an object.
  if (!root["dinner"]["today"].isNull() && !root["dinner"]["today"].is<JsonObjectConst>()) {
    return false;
  }

  return true;
}

int dashboardSchemaVersion(const JsonDocument& state) {
  if (!state.is<JsonObjectConst>()) return -1;
  JsonObjectConst root = state.as<JsonObjectConst>();
  if (!root["schema_version"].is<int>()) return -1;
  return root["schema_version"].as<int>();
}
