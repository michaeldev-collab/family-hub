#include "shared/diagnostics/memory.h"

#include <esp_heap_caps.h>
#include <esp_system.h>

namespace {

size_t activeDecodedImages = 0;
size_t activeChildRoots = 0;

}  // namespace

const char* refreshPhaseName(RefreshPhase phase) {
  switch (phase) {
    case RefreshPhase::Idle: return "IDLE";
    case RefreshPhase::Syncing: return "SYNCING";
    case RefreshPhase::StateReady: return "STATE_READY";
    case RefreshPhase::Rendering: return "RENDERING";
  }
  return "?";
}

void setMemoryUiMetrics(size_t decodedImages, size_t childRoots) {
  activeDecodedImages = decodedImages;
  activeChildRoots = childRoots;
}

void logMemoryBoundary(const char* boundary, RefreshPhase phase) {
  constexpr uint32_t internal8BitCaps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  constexpr uint32_t psram8BitCaps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;

  Serial.printf("[mem] %s phase=%s free_int8=%u min_int8=%u largest_int8=%u "
                "free_psram=%u min_psram=%u largest_psram=%u stack_hwm=%u stack_unit=%u "
                "active_images=%u active_child_roots=%u\n",
                boundary, refreshPhaseName(phase),
                (unsigned)heap_caps_get_free_size(internal8BitCaps),
                (unsigned)heap_caps_get_minimum_free_size(internal8BitCaps),
                (unsigned)heap_caps_get_largest_free_block(internal8BitCaps),
                (unsigned)heap_caps_get_free_size(psram8BitCaps),
                (unsigned)heap_caps_get_minimum_free_size(psram8BitCaps),
                (unsigned)heap_caps_get_largest_free_block(psram8BitCaps),
                (unsigned)uxTaskGetStackHighWaterMark(nullptr),
                (unsigned)sizeof(StackType_t),
                (unsigned)activeDecodedImages,
                (unsigned)activeChildRoots);
}
