#!/usr/bin/env bash
# Compile and run host-side child UI preset unit checks (no PlatformIO / Arduino).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="$ROOT/test/host/child_ui_preset_host_test.cpp"
OUT="${TMPDIR:-/tmp}/child_ui_preset_host_test"
g++ -std=c++17 -Wall -Wextra -I"$ROOT/include" "$SRC" -o "$OUT"
"$OUT"
