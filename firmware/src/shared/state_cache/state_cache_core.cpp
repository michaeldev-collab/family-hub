#include "state_cache.h"
#include "config.h"
#if !defined(FAMILY_HUB_LAUNCHER_BUILD)
#include <SPIFFS.h>
#endif

bool StateCache::begin() {
#if defined(FAMILY_HUB_LAUNCHER_BUILD)
  // A raw Launcher app image does not install this project's SPIFFS
  // partition. Never mount or format a possibly shared data partition.
  hasCache_ = false;
  return false;
#else
  if (!SPIFFS.begin(true)) {
    return false;
  }
  hasCache_ = SPIFFS.exists(CACHE_PATH);
  return true;
#endif
}

bool StateCache::writePath(const char* path, const JsonDocument& state) {
#if defined(FAMILY_HUB_LAUNCHER_BUILD)
  (void)path;
  (void)state;
  return false;
#else
  File f = SPIFFS.open(path, FILE_WRITE);
  if (!f) return false;
  const size_t written = serializeJson(state, f);
  f.close();
  return written > 0;
#endif
}

bool StateCache::readPath(const char* path, JsonDocument& outState, const JsonDocument& filter) {
#if defined(FAMILY_HUB_LAUNCHER_BUILD)
  (void)path;
  (void)outState;
  (void)filter;
  return false;
#else
  if (!SPIFFS.exists(path)) return false;
  File f = SPIFFS.open(path, FILE_READ);
  if (!f) return false;
  const DeserializationError err = deserializeJson(outState, f, DeserializationOption::Filter(filter));
  f.close();
  if (err) {
    outState.clear();
    return false;
  }
  outState.shrinkToFit();
  return true;
#endif
}
