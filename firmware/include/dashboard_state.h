#pragma once

#include <ArduinoJson.h>

// A persistent filter describing the only dashboard fields rendered or acted
// on by the panel. Keeping the schema separate from the candidate/live data
// lets ArduinoJson discard server-only metadata while streaming the response.
const JsonDocument& dashboardStateFilter();

// Reject malformed or unsupported schema before it can replace the last valid
// dashboard state. Empty VM item arrays and a null dinner.today are valid.
bool dashboardStateValid(const JsonDocument& state);

// Returns schema_version or -1 if missing/invalid type.
int dashboardSchemaVersion(const JsonDocument& state);
