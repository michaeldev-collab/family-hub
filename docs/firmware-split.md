# Family Hub Firmware Split — Audit & Migration Plan

**Date:** 2026-07-17  
**Status update (2026-07-26):** Live product is **household-only**. Child panel binary/env and rewards/behavior surfaces were archived out of tree. Treat child sections below as **ARCHIVED historical inventory**, not an active build target.

| Live / archived | Notes |
|---|---|
| Child panel archive | Outside this public repo — see [archive/CHILD_PANEL_MOVED.md](../archive/CHILD_PANEL_MOVED.md) |
| Rewards/behavior archive | Outside this public repo — see [archive/REWARDS_BEHAVIOR_MOVED.md](../archive/REWARDS_BEHAVIOR_MOVED.md) |
| Panel UX cleanup (current) | [panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md) |

**Scope (original):** Split dual-mode Waveshare 7B + M5 Launcher firmware into two independently buildable apps sharing low-level infra only.

| Target binary | PlatformIO env | Output | Status |
|---|---|---|---|
| Family Hub Household | `waveshare7b-household-launcher` (or current household env) | `firmware/dist/family-hub-household.bin` | **LIVE** — continue |
| Family Hub Child | `waveshare7b-child-launcher` | `firmware/dist/family-hub-child.bin` | **ARCHIVED** — see archive path above |
| Headless dev (serial UI) | `devkit` | `.pio/build/devkit/firmware.bin` | Dev only |

> **Retired (Step 7):** legacy dual-mode `waveshare-esp32-s3-touch-lcd-7b` → `family-hub-waveshare7b-launcher.bin`. Source archived at `firmware/archive/main.cpp`.

---

## 1. Household-only code inventory

| Area | Files / symbols | Evidence |
|---|---|---|
| Dashboard JSON filter + validation | `dashboardStateFilter()`, `dashboardStateValid()` | `firmware/src/dashboard_state.cpp` |
| Dashboard state document | `dashboardDoc` global | `firmware/src/main.cpp:25` |
| Dashboard sync lifecycle | `syncDashboard()`, `requestDashboardSync()`, `processDashboardSyncRequest()`, `renderDashboard()`, `RefreshPhase` | `firmware/src/main.cpp:257-428` |
| Household SPIFFS cache | `StateCache::save()` / `load()` on `CACHE_PATH` | `firmware/src/state_cache.cpp`, `firmware/include/config.h:17` |
| Chore completion write path | `completeChoreById()`, `ApiClient::completeChore()`, `httpPost()` + `addAuthHeaders()` | `firmware/src/main.cpp:434-453`, `firmware/src/api_client.cpp:250-310` |
| Household LVGL screens | `ScreenId`, `lvglEnsureScreen()`, `lvglUpdate()`, `renderHome/Grocery/Chores/Dinner/Notes/Settings()` | `firmware/include/ui_manager.h:12-19`, `firmware/src/ui_manager.cpp:137-224,480-1099` |
| Chore modal + row tap flow | `buildChoreModal()`, `populateChoreRows()`, `handleChoreRowTap()`, `consumeChoreCompleteRequest()` | `firmware/src/ui_manager.cpp:708-911` |
| Household poll loop | `lastPollMs`, `POLL_INTERVAL_MS` branch when `!childRuntime.active` | `firmware/src/main.cpp:573-576,709-716` |
| Serial test hooks | `'c'` complete-first-chore, `'n'` next screen | `firmware/src/main.cpp:468-504` |

**Household API endpoints (via `ApiClient`):**

| Method | Path | Auth |
|---|---|---|
| GET | `/api/dashboard-state` | `dashboardStateFilter()` — no display HMAC; LAN trust or write token unused on GET |
| POST | `/api/chores/{id}/complete` | `x-family-hub-token` (write token), `x-family-hub-source: panel` |

---

## 2. Child-only code inventory — ARCHIVED

> **ARCHIVED 2026-07-26.** Do not treat this section as a live build checklist. The previous subsystem was archived outside the public repository; restore documentation remains in the private project archive. Orphan symbols that may still linger in the live tree (e.g. `firmware/src/child_focus_state.cpp`, child surface in `firmware/include/ui_manager.h`) are Phase 1 hygiene debt per [panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md) — strip from household builds; do not re-expand.

| Area | Files / symbols | Evidence |
|---|---|---|
| Child focus runtime model | `ChildFocusPage`, `ChildFocusRuntime` | `firmware/include/child_focus_state.h` |
| Display-home + child-mode JSON filters | `displayHomeFilter()`, `childModeFilter()`, validators | `firmware/src/child_focus_state.cpp` |
| Child JSON documents | `displayHomeDoc`, `childModeDoc` | `firmware/src/main.cpp:26-27` |
| Child runtime global | `childRuntime` | `firmware/src/main.cpp:28` |
| Child panel images (PSRAM) | `childFocusImage`, `childHeaderImage`, `childProfileImages[8]`, `childTaskImages[6]`, `childRewardImages[6]`, `childRoutineImages[4]`, `childStripDailyImage`, `childStripTermImage` | `firmware/src/main.cpp:30-37` |
| Image prepare/clear | `prepareChildPanelImages()`, `prepareChildFocusImage()`, `clearChildPanelImages()` | `firmware/src/main.cpp:56-230` |
| Child mode lifecycle | `enterChildMode()`, `leaveChildMode()`, `syncDisplayHome()`, `syncChildMode()`, `persistChildRuntime()` | `firmware/src/main.cpp:121-255` |
| Child SPIFFS cache | `StateCache::saveChild()` / `loadChild()` on `CHILD_CACHE_PATH` | `firmware/src/state_cache.cpp:39-56` |
| Child NVS persistence | `ApiClient::loadChildRuntime()` / `saveChildRuntime()` | `firmware/src/api_client.cpp:388-411` |
| RTC fast-resume (child) | `rtcChildModeActive`, `rtcSelectedChildId` | `firmware/src/main.cpp:39-40,516` |
| Child LVGL UI (~900 lines) | `rebuildChildScreen()`, `buildChildRenderFingerprint()`, child helpers, tab bar, PIN pad | `firmware/src/ui_manager.cpp:72-96,442-478,1116-1162,1164-2150` |
| Child event handlers | `handleChildModeToggle/ProfileTap/TaskTap/RewardTap/...` | `firmware/src/ui_manager.cpp:2133-2148` |
| Child poll + idle return | `lastChildPollMs`, `refreshSeconds`, `idleReturnSeconds` | `firmware/src/main.cpp:682-694` |
| Child loop dispatch | entire `childRuntime.active` block in `loop()` | `firmware/src/main.cpp:583-694` |

**Child API endpoints (via `ApiClient` display auth):**

| Method | Path | Purpose |
|---|---|---|
| GET | `/api/v1/display/rewards-behavior/home` | Child selection / mode flags |
| GET | `/api/v1/display/child-mode/{childId}` | Full child focus payload |
| POST | `/api/v1/display/actions/complete-task` | Task completion |
| POST | `/api/v1/display/actions/select-reward` | Reward selection |
| POST | `/api/v1/display/actions/request-reward` | Reward request |
| POST | `/api/v1/display/actions/verify-parent-pin` | Exit / change-child PIN |
| GET | `{mediaRef}` (relative URL from JSON) | 128×128 JPEG panel images |

---

## 3. Genuinely shared infrastructure

| Component | Current location | Shared layer target |
|---|---|---|
| Waveshare RGB + GT911 + LVGL bring-up | `firmware/src/display.cpp`, `firmware/include/display.h`, `firmware/include/panel_config.h` | `src/shared/display/` |
| LVGL lock/tick/memory gate | `displayLock()`, `displayTick()`, `displayRenderMemoryAvailable()` | `src/shared/display/` |
| Wi-Fi connect | `ApiClient::connectWiFi()` | `src/shared/network/` |
| HTTP transport shell | `HTTPClient` begin/timeout/end patterns | `src/shared/api_transport/` |
| Display HMAC auth | `displayToken()`, `addDisplayAuthHeaders()`, `idempotencyKey()` | `src/shared/api_transport/` |
| Write-token auth headers | `addAuthHeaders()` | `src/shared/api_transport/` (household POST only) |
| JPEG decode → `PanelImage` | `PanelImage`, `fetchPanelImage()` | `src/shared/panel_image/` |
| Device ID + NVS server/token | `makeDeviceId()`, `ApiClient::begin()`, `setServer()`, `setWriteToken()` | `src/shared/` + per-app NVS subset |
| Memory diagnostics | `logMemoryBoundary()`, `logApiMemory()` | `src/shared/diagnostics/` |
| Launcher bin copy + size gate | `firmware/launcher_copy.py` | `src/shared/launcher/` |
| Build constants | `firmware/include/config.h`, `lv_conf.h`, `secrets.h` | `include/shared/` |
| `PanelStatus`, `ConnState` | `firmware/include/api_client.h:22-38` | `include/shared/` |

**Shared MUST NOT contain:** child task/reward logic, household dashboard cards, screen-specific JSON filters (`dashboardStateFilter` vs `childModeFilter`), discipline/focus page routing.

---

## 4. Global JsonDocument / PanelImage allocations

### JsonDocument globals (`main.cpp`)

| Global | Owner app | Populated by | Max concern |
|---|---|---|---|
| `dashboardDoc` | Household | `syncDashboard()` ← `fetchDashboardState` | Compact filtered dashboard (~few KB) |
| `displayHomeDoc` | Child (selection) | `syncDisplayHome()` | Small: children list + mode flags |
| `childModeDoc` | Child | `syncChildMode()` | Largest JSON; tasks/rewards/routines arrays |

All three coexist in dual-mode firmware today. **Split target:** household binary holds only `dashboardDoc`; child binary holds `displayHomeDoc` + `childModeDoc`.

### Static filter documents (singleton pools)

| Filter | File:line | Owner |
|---|---|---|
| `dashboardStateFilter()` static doc | `dashboard_state.cpp:4` | Household |
| `displayHomeFilter()` static doc | `child_focus_state.cpp:4` | Child |
| `childModeFilter()` static doc | `child_focus_state.cpp:31` | Child |

### PanelImage allocations (`main.cpp:29-37`, WAVESHARE_7B only)

| Array / variable | Count | Bytes each (decoded) | Max total |
|---|---|---|---|
| `childFocusImage` | 1 | 128×128×2 = 32,768 | 32 KB |
| `childHeaderImage` | 1 | 32,768 | 32 KB |
| `childProfileImages` | 8 | 32,768 | 256 KB |
| `childTaskImages` | 6 | 32,768 | 192 KB |
| `childRewardImages` | 6 | 32,768 | 192 KB |
| `childRoutineImages` | 4 | 32,768 | 128 KB |
| `childStripDailyImage` | 1 | 32,768 | 32 KB |
| `childStripTermImage` | 1 | 32,768 | 32 KB |
| **Worst-case decoded** | **28 images** | | **~896 KB PSRAM** |

`fetchPanelImage()` also allocates transient JPEG buffer (≤96 KB) + 4 KB work buffer in internal heap during decode (`api_client.cpp:366-373`).

### UiManager image pointer mirrors (not owning)

`childFocusImage_`, `childHeaderImage_`, `childProfileImages_[8]`, etc. — pointers into `main.cpp` allocations (`ui_manager.h:175-182`).

---

## 5. LVGL screen ownership / deletion patterns

### Household dashboard (persistent tree)

1. **First build:** `lvglEnsureScreen()` — `lv_obj_clean(lv_scr_act())` then creates header, nav, 4 cards, chore list, modal once (`ui_manager.cpp:480-484`). Guarded by `if (headerBar_) return`.
2. **Updates:** `lvglUpdate()` mutates labels in place; does **not** destroy the tree (`ui_manager.cpp:967-1099`).
3. **Chore rows:** `populateChoreRows()` reuses `choreList_` container; `hideChoreRows()` hides when not on Chores screen.

### Child mode (destructive rebuild) — ARCHIVED

> **ARCHIVED 2026-07-26.** Historical dual-mode notes only. Live household UI: `firmware/src/household/household_ui.cpp`. Child restore: see [archive/CHILD_PANEL_MOVED.md](../archive/CHILD_PANEL_MOVED.md).

1. **Enter child from household:** household tree remains allocated until `clearChildFocusScreen()` or `rebuildChildScreen()` runs.
2. **`rebuildChildScreen()`:** `lv_obj_clean(lv_scr_act())` + `resetLvglPointers()` — full screen teardown every navigation-worthy change (`ui_manager.cpp:1542-1544`).
3. **Fingerprint gate:** `renderChildMode()` skips rebuild if fingerprint unchanged and not dirty (`ui_manager.cpp:85-90`).
4. **Exit child:** `leaveChildMode()` → `clearChildFocusScreen()` → `ui.setScreen(Home)` → `renderDashboard()` rebuilds household via `lvglEnsureScreen()` (header already null after clean).
5. **Content layout:** Child content area `y=100..512` (`kChildContentTop`..`kChildNavBarY`), nav bar pinned at `y=512` (`ui_manager.h:153-157`).

### Display boot screen

`displayBegin()` creates smoke-test screen; `lvglEnsureScreen()` or `rebuildChildScreen()` replaces it via `lv_obj_clean` (`display.cpp:535-559`, `ui_manager.cpp:484`).

### Thread safety

All LVGL mutations under `displayLock()` / `displayUnlock()`; `displayTick()` also locks in loop (`display.cpp:661-667`).

---

## 6. API methods per application

### ApiTransport (shared extract target)

| Symbol | Current file |
|---|---|
| `connectWiFi()` | `api_client.cpp:89` |
| `baseUrl()` | `api_client.cpp:118` |
| `addAuthHeaders()` | `api_client.cpp:127` |
| `addDisplayAuthHeaders()` | `api_client.cpp:155` |
| `displayToken()` | `api_client.cpp:133` |
| `idempotencyKey()` | `api_client.cpp:161` |
| `httpDisplayGet()` | `api_client.cpp:169` |
| `httpDisplayPost()` | `api_client.cpp:187` |
| `httpGetDashboard()` | `api_client.cpp:204` |
| `httpPost()` | `api_client.cpp:250` |
| `tickBackoff()` | `api_client.cpp:122` |
| `begin()` / NVS for server+token | `api_client.cpp:60` |

### HouseholdApi (extract from ApiClient)

| Method | Endpoint |
|---|---|
| `fetchDashboardState()` | GET `/api/dashboard-state` |
| `completeChore()` | POST `/api/chores/{id}/complete` |

### ChildApi (extract from ApiClient) — ARCHIVED

> **ARCHIVED 2026-07-26.** Not a live Family Hub API surface.

| Method | Endpoint |
|---|---|
| `fetchDisplayHome()` | GET `/api/v1/display/rewards-behavior/home` |
| `fetchChildMode()` | GET `/api/v1/display/child-mode/{id}` |
| `requestTaskCompletion()` | POST `.../complete-task` |
| `selectReward()` | POST `.../select-reward` |
| `requestReward()` | POST `.../request-reward` |
| `verifyParentPin()` | POST `.../verify-parent-pin` |
| `fetchPanelImage()` | GET media ref |
| `loadChildRuntime()` / `saveChildRuntime()` | NVS only |

### Future HTTP header

Neither app sends `x-family-hub-firmware` today. **Target:** `x-family-hub-firmware: household|child` + `FIRMWARE_VERSION`. Server currently accepts requests without this header (`server/middleware/auth.js`); add compatibility note only — no server change required for initial split builds.

---

## 7. Persistent RTC / NVS state per app

### NVS namespace `familyhub` (`config.h:20-27`)

| Key | Household | Child | Notes |
|---|---|---|---|
| `srv_host` | ✓ | ✓ | Shared server address |
| `srv_port` | ✓ | ✓ | Shared |
| `write_tok` | ✓ (chore POST) | — | Household write token |
| `child_mode` | — | ✓ | `childRuntime.active` |
| `child_id` | — | ✓ | Selected child |
| `child_page` | — | ✓ | `ChildFocusPage` name |
| `child_sync` | — | ✓ | Last successful sync ms |

### RTC slow memory (`RTC_DATA_ATTR`, `main.cpp:39-40`)

| Variable | Child only | Purpose |
|---|---|---|
| `rtcChildModeActive` | ✓ | Survive deep sleep / reset without NVS read |
| `rtcSelectedChildId[101]` | ✓ | Fast child resume |

Household app has no RTC state today.

### SPIFFS (`StateCache`)

| Path | App | Launcher build |
|---|---|---|
| `/dashboard_cache.json` | Household | Disabled (`FAMILY_HUB_LAUNCHER_BUILD`) |
| `/child_cache.json` | Child | Disabled |

`persistentCacheIoAllowed()` returns `false` for launcher builds and `!displayReady()` for waveshare7b (`main.cpp:295-303`).

---

## 8. File ownership map

| File | household | child | shared | Notes |
|---|---|---|---|---|
| `src/main.cpp` | — | — | — | **Archived** — `household/main.cpp` + `child/main.cpp` |
| `src/api_client.cpp` | — | — | — | **Retired from builds** — split into `shared/api/`, `household/`, `child/` |
| `src/ui_manager.cpp` | — | — | — | **Retired from builds** — split into `shared/ui/`, `household/`, `child/` |
| `src/shared/api/api_client_core.cpp` | | | ✓ | WiFi, NVS, backoff, firmware headers |
| `src/household/household_api_client.cpp` | ✓ | | | Dashboard fetch, chore complete |
| `src/child/child_api_client.cpp` | | ✓ | | Display auth, child endpoints, PanelImage JPEG |
| `src/shared/ui/ui_manager_core.cpp` | | | ✓ | begin, consume*, badge, status bar, resetLvglPointers |
| `src/household/household_ui.cpp` | ✓ | | | Household serial + LVGL tree |
| `src/child/child_ui.cpp` | | ✓ | | Child LVGL + handlers (~1100 lines) |
| `src/display.cpp` | | | ✓ | |
| `src/state_cache.cpp` | — | — | — | **Retired** — split into `shared/state_cache/`, `household/`, `child/` |
| `src/shared/state_cache/state_cache_core.cpp` | | | ✓ | `begin()`, `writePath()`, `readPath()` |
| `src/household/household_state_cache.cpp` | ✓ | | | `save()`, `load()` |
| `src/child/child_state_cache.cpp` | | ✓ | | `saveChild()`, `loadChild()` |
| `archive/api_client.cpp` | — | — | — | Retired monolith (reference only) |
| `archive/ui_manager.cpp` | — | — | — | Retired monolith; source for `split_ui_manager.py` |
| `archive/main.cpp` | — | — | — | Retired dual-mode monolith (reference only) |
| `src/dashboard_state.cpp` | ✓ | | | |
| `src/child_focus_state.cpp` | | ✓ | | |
| `include/api_client.h` | partial | partial | partial | |
| `include/ui_manager.h` | partial | partial | | |
| `include/display.h` | | | ✓ | |
| `include/panel_config.h` | | | ✓ | |
| `include/config.h` | | | ✓ | App-specific keys can move later |
| `include/dashboard_state.h` | ✓ | | | |
| `include/child_focus_state.h` | | ✓ | | |
| `include/state_cache.h` | | | ✓ | All methods declared; no `#ifndef` guards |
| `include/lv_conf.h` | | | ✓ | |
| `include/secrets.h` | | | ✓ | Not committed |
| `launcher_copy.py` | | | ✓ | Parameterized per env |
| `platformio.ini` | | | ✓ | Env definitions |

---

## 9. Shared dependency map

```
                    ┌─────────────┐
                    │  secrets.h  │
                    └──────┬──────┘
                           │
         ┌─────────────────┼─────────────────┐
         ▼                 ▼                 ▼
   ┌──────────┐     ┌────────────┐    ┌───────────┐
   │ config.h │────▶│ ApiTransport│◀───│ display.h │
   └──────────┘     └─────┬──────┘    └─────┬─────┘
                           │                  │
              ┌────────────┼────────────┐     │ lvgl, panel_config
              ▼            ▼            ▼     ▼
        HouseholdApi   ChildApi    PanelImage  ui_* (per app)
              │            │            │
              ▼            ▼            ▼
      dashboard_state  child_focus   child_media
              │            │            │
              ▼            ▼            ▼
         household_ui   child_ui    (image decode)
              │            │
              └─────┬──────┘
                    ▼
              display.cpp (LVGL tick/lock)
```

**External libs (platformio):** ArduinoJson ^7.2.1, lvgl ^8.4.0 (waveshare7b), WiFi/HTTPClient/Preferences (Arduino).

---

## 10. High-risk globals list (with evidence)

| Global | Risk | File:line |
|---|---|---|
| `dashboardDoc` | Household state leaked into child binary if not stripped | `main.cpp:25` |
| `childModeDoc` | Large child JSON + pointers into LVGL after rebuild | `main.cpp:27` |
| `childRuntime` | Mode flag gates entire `loop()` behavior | `main.cpp:28` |
| `childProfileImages[8]` etc. | ~896 KB PSRAM footprint | `main.cpp:32-37` |
| `refreshPhase` | Household sync/render reentrancy guard | `main.cpp:264` |
| `ui` (UiManager) | Mixes household + child LVGL pointers | `main.cpp:23` |
| `api` (ApiClient) | Single class with all endpoints | `main.cpp:21` |
| `rtcChildModeActive` | Boots into child on household app if shared NVS | `main.cpp:39` |
| `headerBar_` / child pointers in UiManager | Stale pointers after `lv_obj_clean` without `resetLvglPointers` | `ui_manager.cpp:1101-1114` |
| `childRenderFingerprint_` | Skips needed rebuild if stale | `ui_manager.cpp:85-90` |
| Static filter JsonDocuments | Permanent heap pools (~3 docs) | `dashboard_state.cpp:4`, `child_focus_state.cpp:4,31` |
| `displayHomeDoc` | Required for child poll intervals when in child mode | `main.cpp:683-685` |

---

## 11. platformio.ini — current envs and launcher copy

| Env | Purpose | Partition table | Post-build |
|---|---|---|---|
| `devkit` | Headless serial UI (household loop, no panel) | `default.csv` | — |
| `waveshare7b` | Full 16 MB Waveshare bring-up | `partitions_16MB.csv` | — |
| `waveshare7b-household-launcher` | M5 Launcher household app | `partitions_launcher_16MB.csv` | `launcher_copy.py` → `dist/family-hub-household.bin` |
| `waveshare7b-child-launcher` | M5 Launcher child app | `partitions_launcher_16MB.csv` | `launcher_copy.py` → `dist/family-hub-child.bin` |
| `elecrow7` | Stub secondary panel | 4 MB | — |

**Retired:** `waveshare-esp32-s3-touch-lcd-7b` (dual-mode launcher) — removed Step 7.

**Launcher copy process** (`launcher_copy.py`):

1. Post-action on `firmware.bin` after link.
2. Copies to `firmware/dist/<output>.bin`.
3. Enforces 0x180000 (1.5 MB) size limit for M5 Launcher test slot.
4. Per-app images ~1.25–1.34 MB — well under 0x180000 (1.5 MB) Launcher slot.

**Launcher envs:** `waveshare7b-household-launcher`, `waveshare7b-child-launcher` — see `platformio.ini`.

---

## 12. Known blockers and remaining dual-mode code

| Blocker | Severity | Mitigation |
|---|---|---|
| Single `main.cpp` owns both app loops | High | Extract `household/main.cpp` and `child/main.cpp` with `build_src_filter` |
| `UiManager` is monolithic (~2150 lines) | High | Split `household_ui` / `child_ui`; shared status badge only |
| `ApiClient` combines transport + both APIs | Medium | Extract `ApiTransport`, `HouseholdApi`, `ChildApi` |
| Household header has "Child" button (`childModeButton_`) | Medium | Remove from household binary |
| `enterChildMode()` / `leaveChildMode()` bridge apps | Medium | Delete from household; child app has no exit-to-dashboard |
| NVS child keys written by dual-mode | Low | Child app owns child_* keys; household ignores |
| No `x-family-hub-firmware` header yet | Low | Add in ApiTransport; server optional |
| Launcher SPIFFS disabled | Info | Both apps network-only on launcher install (by design) |
| `build_src_filter` not yet isolating TUs | High | Required before symbol verification passes |
| New env stubs not yet compiling standalone | Expected | This session: skeleton + wrappers only |

### Remaining dual-mode code (do not delete until both targets compile)

- Entire `firmware/src/main.cpp` child branches (`loop()` lines 583-694, globals 25-40)
- `ui_manager` child methods and `childModeButton_` on household header
- `ApiClient` child endpoint methods
- `StateCache::saveChild/loadChild`

---

## Target directory structure (migration destination)

```
firmware/
  src/
    shared/
      display/          ← display.cpp (move)
      network/          ← WiFi helpers
      api_transport/    ← HTTP + auth shell
      panel_image/      ← PanelImage + JPEG decode
      launcher/         ← copy script helpers
      diagnostics/      ← logMemoryBoundary
    household/
      main.cpp
      household_api.cpp
      household_state.cpp   ← dashboard_state
      household_ui.cpp      ← household half of ui_manager
    child/
      main.cpp
      child_api.cpp
      child_state.cpp       ← child_focus_state
      child_ui.cpp            ← child LVGL
      child_media.cpp         ← image prepare helpers from main
  include/
    shared/
    household/
    child/
```

### Compatibility wrappers (session 2026-07-17)

Legacy includes remain valid. New paths re-export via thin headers:

- `include/shared/compat/legacy.h` — includes original `display.h`, `config.h`, etc.
- `include/household/compat/legacy.h` — includes `dashboard_state.h`
- `include/child/compat/legacy.h` — includes `child_focus_state.h`

---

## Verification checklist (when builds exist)

```bash
cd firmware
pio run -e waveshare7b-household-launcher
pio run -e waveshare7b-child-launcher
pio run -e devkit   # optional headless regression
```

**Symbol inspection (nm / map file):**

- Household must NOT reference: `ChildFocusRuntime`, `childModeDoc`, `rebuildChildScreen`, `fetchChildMode`, `childProfileImages`
- Child must NOT reference: `dashboardDoc`, `syncDashboard`, `completeChore`, `lvglEnsureScreen` (household tree), `ScreenId`

---

## Session log

| Date | Step | Status |
|---|---|---|
| 2026-07-17 | Step 1 — Audit | **Complete** (this document) |
| 2026-07-17 | Step 2 — Shared skeleton + env stubs | **Complete** — folders, compat headers, API wrappers, `platformio.ini` stubs; all three launcher envs build (still dual-mode object code) |
| 2026-07-17 | Step 3 — Household extract + compile | **Complete** — see build map below |
| 2026-07-17 | Step 4 — Child extract + compile | **Complete** — see Step 4 results below |
| 2026-07-17 | Step 5 — Physical api/ui split | **Complete** — see Step 5 results below |
| 2026-07-17 | Step 6 — Archive monoliths + state_cache split | **Complete** — see Step 6 results below |
| 2026-07-17 | Step 7 — Retire dual-mode main + legacy env | **Complete** — see Step 7 results below |

### Step 3 results (2026-07-17)

**Household entry:** `firmware/src/household/main.cpp` (replaces `src/main.cpp` for household env).

**Shared extract (household env only):**

| Component | Path |
|---|---|
| Display stack | `src/shared/display/display.cpp` (legacy `src/display.cpp` excluded via `build_src_filter`) |
| Memory diagnostics | `src/shared/diagnostics/memory.cpp`, `include/shared/diagnostics/memory.h` |

**Compile-time guards (`FAMILY_HUB_APP_HOUSEHOLD`):**

- `api_client.cpp` — child endpoint methods excluded
- `ui_manager.cpp` / `ui_manager.h` — child LVGL UI, handlers, header button excluded
- `state_cache.cpp` — `saveChild` / `loadChild` excluded

**Excluded from household link:** `src/main.cpp`, `src/display.cpp`, `src/child_focus_state.cpp`

**Legacy/child env fix:** `build_src_filter` excludes `shared/`, `household/`, `child/` so dual-mode still links `src/display.cpp` (no duplicate symbol).

#### Build map — `waveshare7b-household-launcher`

| Artifact | Size |
|---|---|
| `dist/family-hub-household.bin` | **1,253,040 B** (79.7% of 0x180000 launcher slot) |
| Flash (.text + .rodata) | **1,149,394 B** (reported 1,252,668 B incl. headers) |
| RAM (.data + .bss) | **97,068 B** (29.6% of 327,680 B) |

**Top first-party objects (text):**

| Object | .text |
|---|---|
| `ui_manager.cpp.o` | 21,030 |
| `api_client.cpp.o` | 14,195 |
| `household/main.cpp.o` | 9,093 |
| `shared/display/display.cpp.o` | 6,956 |
| `dashboard_state.cpp.o` | 4,231 |

#### Size delta vs legacy dual-mode (`waveshare-esp32-s3-touch-lcd-7b`)

| Metric | Household | Legacy dual | Δ |
|---|---|---|---|
| Flash | 1,252,668 B | 1,362,764 B | **−110,096 B (~108 KB)** |
| RAM | 97,068 B | 99,508 B | **−2,440 B** |
| `.bin` | 1,253,040 B | 1,363,248 B | **−110,208 B** |

#### Symbol verification (household ELF)

```bash
xtensa-esp-elf-nm .pio/build/waveshare7b-household-launcher/firmware.elf \
  | rg -i 'child|ChildFocus|rebuildChild|fetchChild|syncChild|enterChild|childProfile|childMode'
# → no matches
```

**Remaining for Step 4:** child `main.cpp`, child `build_src_filter`, strip child guards from household tree once child target compiles independently.

### Step 4 results (2026-07-17)

**Child entry:** `firmware/src/child/main.cpp` — child-only loop (selection + focus UI, no dashboard sync/render).

**Compile-time guards (`FAMILY_HUB_APP_CHILD`):**

- `api_client` — `fetchDashboardState`, `completeChore`, `httpGetDashboard`, `httpPost` excluded
- `state_cache` — household `save`/`load` excluded
- `ui_manager` — household LVGL tree (`lvglEnsureScreen`, chore modal, `render*`) excluded

**Excluded from child link:** `src/main.cpp`, `src/display.cpp`, `src/dashboard_state.cpp`, `household/`

**Household filter fix:** `-<child/>` added so household env no longer compiles `child/main.cpp`.

#### Build map — three launcher targets

| Target | Flash | RAM | `.bin` | Launcher slot |
|---|---|---|---|---|
| **Household** | 1,252,668 B | 97,068 B | 1,253,040 B | 79.7% |
| **Child** | 1,336,504 B | 98,924 B | 1,336,976 B | 85.0% |
| **Legacy dual** | 1,362,768 B | 99,508 B | 1,363,248 B | 86.7% |

**Delta vs legacy dual:**

| | Household | Child |
|---|---|---|
| Flash | −110,100 B (~108 KB) | −26,264 B (~26 KB) |
| RAM | −2,440 B | −584 B |

**Top first-party objects (child `.text`):** `ui_manager` ~19 KB · `child/main` ~11 KB · `api_client` ~12 KB · `shared/display` ~7 KB · `child_focus_state` ~2 KB

#### Symbol verification (child ELF)

```bash
xtensa-esp-elf-nm .pio/build/waveshare7b-child-launcher/firmware.elf \
  | rg -i 'syncDashboard|renderDashboard|fetchDashboard|completeChore|dashboardDoc|lvglEnsureScreen|lvglUpdate|renderHome|dashboardState'
# → no matches
```

**Remaining for Step 5:** physical file split of `ui_manager.cpp` / `api_client.cpp`; retire legacy dual-mode `main.cpp` after panel flash validation of both launcher bins.

### Step 5 results (2026-07-17)

**Physical split** — monolithic `api_client.cpp` / `ui_manager.cpp` excluded from all launcher envs via `build_src_filter`. Headers (`api_client.h`, `ui_manager.h`) no longer use `#ifndef FAMILY_HUB_APP_*` guards; isolation is by compile unit selection.

| Component | Path |
|---|---|
| API transport core | `src/shared/api/api_client_core.cpp` |
| Household API | `src/household/household_api_client.cpp` |
| Child API | `src/child/child_api_client.cpp` |
| UI core | `src/shared/ui/ui_manager_core.cpp` |
| Household UI | `src/household/household_ui.cpp` (generated from monolith via `scripts/split_ui_manager.py`) |
| Child UI | `src/child/child_ui.cpp` |

**`resetLvglPointers()`** — sole remaining `#if !defined(FAMILY_HUB_APP_*)` guard in split tree (`ui_manager_core.cpp`); dual-mode legacy needs both pointer-reset blocks.

**Legacy dual env** — still links `src/main.cpp` + `src/display.cpp`; includes only split api/ui objects (`household_*` + `child_*` + `shared/*`), not `household/main.cpp` or `child/main.cpp`.

#### Build map — three launcher targets (post-split)

| Target | Flash | RAM | `.bin` | Δ Flash vs Step 4 | Δ `.bin` vs Step 4 |
|---|---|---|---|---|---|
| **Household** | 1,253,848 B | 98,260 B | 1,254,208 B | +1,180 B | +1,168 B |
| **Child** | 1,337,624 B | 99,340 B | 1,338,096 B | +1,120 B | +1,120 B |
| **Legacy dual** | 1,364,752 B | 99,508 B | 1,365,232 B | +1,984 B | +1,984 B |

Step 4 baselines: household flash 1,252,668 · child 1,336,504 · legacy 1,362,768. Split adds ~1.1–2.0 KB per target (translation-unit overhead); within ~2 KB of Step 4, slightly above the ±1 KB goal.

**Top first-party objects (household `.text` after split):** `household_ui` ~21 KB · `household_api_client` ~14 KB · `household/main` ~9 KB · `shared/display` ~7 KB

**Top first-party objects (child `.text` after split):** `child_ui` ~19 KB · `child/main` ~11 KB · `child_api_client` ~12 KB · `shared/display` ~7 KB

#### Symbol verification (post-split)

```bash
# Household — no child symbols
xtensa-esp32s3-elf-nm .pio/build/waveshare7b-household-launcher/firmware.elf \
  | rg -i 'child|ChildFocus|rebuildChild|fetchChild|renderChild'
# → no matches

# Child — no household dashboard symbols
xtensa-esp32s3-elf-nm .pio/build/waveshare7b-child-launcher/firmware.elf \
  | rg -i 'syncDashboard|fetchDashboard|completeChore|lvglEnsureScreen|renderHome'
# → no matches
```

**Remaining for Step 6:** retire monolithic `src/api_client.cpp` / `src/ui_manager.cpp` (or move to `archive/`) after panel flash validation; optional `state_cache` physical split.

### Step 6 results (2026-07-17)

**Archived monoliths** — moved to `firmware/archive/` (outside `src/`, not compiled):

- `archive/api_client.cpp`
- `archive/ui_manager.cpp`
- `archive/README.md` — maps retired files to split replacements

`scripts/split_ui_manager.py` now reads `archive/ui_manager.cpp` when regenerating household/child UI.

**`state_cache` physical split:**

| Component | Path |
|---|---|
| Core | `src/shared/state_cache/state_cache_core.cpp` |
| Household | `src/household/household_state_cache.cpp` |
| Child | `src/child/child_state_cache.cpp` |

`include/state_cache.h` — guards removed; all methods always declared.

**`platformio.ini`** — all three launcher envs exclude `state_cache.cpp`; legacy includes split cache objects explicitly (same pattern as api/ui).

#### Build map — regression vs Step 5 (identical)

| Target | Flash | RAM | `.bin` | Δ vs Step 5 |
|---|---|---|---|---|
| **Household** | 1,253,848 B | 98,260 B | 1,254,208 B | **0** |
| **Child** | 1,337,624 B | 99,340 B | 1,338,096 B | **0** |
| **Legacy dual** | 1,364,752 B | 99,508 B | 1,365,232 B | **0** |

#### Symbol verification (post-Step 6)

```bash
# Household — no child UI/API symbols
xtensa-esp32s3-elf-nm -C .pio/build/waveshare7b-household-launcher/firmware.elf \
  | rg -i 'saveChild|loadChild|childMode|rebuildChild|renderChild|fetchChild'
# → no matches

# Child — no household dashboard symbols
xtensa-esp32s3-elf-nm -C .pio/build/waveshare7b-child-launcher/firmware.elf \
  | rg -i 'syncDashboard|fetchDashboard|completeChore|lvglEnsureScreen|renderHome|StateCache::save\b|StateCache::load\b'
# → no matches
```

Note: `StateCache::save/load/saveChild/loadChild` are dead-stripped in launcher builds because `persistentCacheIoAllowed()` is compile-time `false` under `FAMILY_HUB_LAUNCHER_BUILD`. Only `StateCache::begin()` remains in the ELF.

**Remaining:** retire legacy dual-mode `src/main.cpp` after both launcher bins pass panel flash validation.

### Step 7 results (2026-07-17)

**Panel flash validation:** household + child launcher bins confirmed on hardware (prerequisite for this step).

**Archived:** `firmware/archive/main.cpp` — dual-mode entry that combined household dashboard + child focus in one `loop()`.

**Removed PlatformIO env:** `waveshare-esp32-s3-touch-lcd-7b` (no longer produces `family-hub-waveshare7b-launcher.bin`).

**`devkit` env** — now uses `household/main.cpp` + split api/ui/state_cache (serial dashboard, no `WAVESHARE_7B`).

#### Production launcher workflow

```bash
cd firmware
pio run -e waveshare7b-household-launcher   # → dist/family-hub-household.bin
pio run -e waveshare7b-child-launcher       # → dist/family-hub-child.bin
```

Install each `.bin` via M5 Launcher SD card as separate apps. Household and child are independent images — no dual-mode switching at runtime.

#### Build map — post-Step 7 (unchanged from Step 6)

| Target | Flash | RAM | `.bin` |
|---|---|---|---|
| **Household** | 1,253,848 B | 98,260 B | 1,254,208 B |
| **Child** | 1,337,624 B | 99,340 B | 1,338,096 B |
| **Devkit** (serial) | 814,317 B | 44,940 B | — |

**Firmware split complete.** Optional follow-ups: update historical doc line refs from `main.cpp` → split paths.
