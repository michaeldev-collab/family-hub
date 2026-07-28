# Archived monolithic sources

Retired from launcher builds during firmware-split Steps 6–7 (2026-07-17).

| File | Replaced by |
|---|---|
| `api_client.cpp` | `src/shared/api/api_client_core.cpp`, `src/household/household_api_client.cpp`, `src/child/child_api_client.cpp` |
| `ui_manager.cpp` | `src/shared/ui/ui_manager_core.cpp`, `src/household/household_ui.cpp`, `src/child/child_ui.cpp` |
| `main.cpp` | `src/household/main.cpp` + `src/child/main.cpp` (separate M5 Launcher apps) |

`scripts/split_ui_manager.py` reads `ui_manager.cpp` from here when regenerating household/child UI extracts. **Child extract path is archived** (2026-07-26) — live product is household-only; do not regenerate into a live `src/child/`.

**Phase 1 hygiene archives (2026-07-26):**

| File | Note |
|---|---|
| `display.cpp.root-stale-20260726.cpp` | Stale root twin of `src/shared/display/display.cpp` |
| `child_focus_state.cpp.20260726` / `child_focus_state.h.20260726` | Orphan child focus types removed from live include/src |

**Production launcher binaries** (post Step 7 / Phase 1):

- `dist/family-hub-household.bin` — `pio run -e waveshare7b-household-launcher`
- ~~`dist/family-hub-child.bin`~~ — **retired from live `platformio.ini`**; restore from child-panel archive if needed

The legacy dual-mode `family-hub-waveshare7b-launcher.bin` env (`waveshare-esp32-s3-touch-lcd-7b`) was removed after panel flash validation of both split apps.
