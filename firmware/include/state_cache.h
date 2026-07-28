#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class StateCache {
 public:
  bool begin();
  bool save(const JsonDocument& state);
  bool load(JsonDocument& outState);
  bool saveChild(const JsonDocument& state);
  bool loadChild(JsonDocument& outState, const String& expectedChildId);
  bool hasCache() const { return hasCache_; }

 private:
  bool hasCache_ = false;
  bool writePath(const char* path, const JsonDocument& state);
  bool readPath(const char* path, JsonDocument& outState, const JsonDocument& filter);
};
