#pragma once

#include <Arduino.h>

enum class RefreshPhase {
  Idle,
  Syncing,
  StateReady,
  Rendering,
};

const char* refreshPhaseName(RefreshPhase phase);

// Update the UI-owned counts reported by subsequent memory boundary logs.
// This stores only scalar values and is safe to call before every boundary.
void setMemoryUiMetrics(size_t activeDecodedImages, size_t activeChildRoots);

void logMemoryBoundary(const char* boundary, RefreshPhase phase = RefreshPhase::Idle);
