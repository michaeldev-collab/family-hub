#include "state_cache.h"
#include "config.h"
#include "dashboard_state.h"
#if !defined(FAMILY_HUB_LAUNCHER_BUILD)
#include <SPIFFS.h>
#endif

bool StateCache::save(const JsonDocument& state) {
#if defined(FAMILY_HUB_LAUNCHER_BUILD)
  (void)state;
  return false;
#else
  File f = SPIFFS.open(CACHE_PATH, FILE_WRITE);
  if (!f) return false;
  const size_t written = serializeJson(state, f);
  f.close();
  if (written == 0) return false;
  hasCache_ = true;
  return true;
#endif
}

bool StateCache::load(JsonDocument& outState) {
#if defined(FAMILY_HUB_LAUNCHER_BUILD)
  (void)outState;
  return false;
#else
  if (!SPIFFS.exists(CACHE_PATH)) return false;
  File f = SPIFFS.open(CACHE_PATH, FILE_READ);
  if (!f) return false;
  DeserializationError err = deserializeJson(
      outState, f, DeserializationOption::Filter(dashboardStateFilter()));
  f.close();
  if (err || !dashboardStateValid(outState)) {
    outState.clear();
    return false;
  }
  outState.shrinkToFit();
  return true;
#endif
}
