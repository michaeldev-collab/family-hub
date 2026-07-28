# Family Hub — Panel UX Phase 1 Plan (Hygiene)

**Status:** Phase 1 Execute **COMPLETE** (2026-07-26) — CEO ★ authorized  
**Date:** 2026-07-26  
**Product lock:** [panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md)  
**Handoff:** [panel-ux-cleanup-cto-handoff-phase1.md](panel-ux-cleanup-cto-handoff-phase1.md)  
**Out of scope:** sleep, `ShellGestures`, diagnostics UI, contracts, deploy, archive remount

---

## 6. Execute record (2026-07-26)

| Check | Result |
|-------|--------|
| WP-1 macros + filters | Done — `waveshare7b` has `FAMILY_HUB_APP_HOUSEHOLD`; `devkit` uses `shared/display` stubs |
| WP-2 archive display twin | `firmware/archive/display.cpp.root-stale-20260726.cpp`; root deleted |
| WP-3 strip child | Headers/core cleaned; `child_focus_state.*` → archive `*.20260726` |
| WP-4 docs/scripts | Banners updated |
| `pio run -e devkit` | SUCCESS |
| `pio run -e waveshare7b` | SUCCESS |
| `pio run -e waveshare7b-household-launcher` | SUCCESS → `dist/family-hub-household.bin` |
| nm child-symbol check | PASS (waveshare7b + launcher) |
| Aikido | `issues: []` on modified headers/core (opengrep noted empty/non-git tree; 0 findings) |

---

## 1. Audit findings (Known — pre-Execute)

### PlatformIO envs (live)

| Env | Role | `FAMILY_HUB_APP_HOUSEHOLD` | Excludes root `display.cpp` | Excludes `child_focus_state.cpp` |
|-----|------|---------------------------|-----------------------------|----------------------------------|
| `devkit` | Serial household | **Yes** (`=1`) | No (file still on disk; no shared/display) | **No** — `+<*>` can still compile it |
| `waveshare7b` | Production panel | **No** — flag missing | Yes (`-<display.cpp>`) | Yes |
| `waveshare7b-household-launcher` | M5 launcher | **Yes** | Yes | Yes |
| `elecrow7` | Stub only | n/a | stub | stub |

Canonical Waveshare bring-up is already `firmware/src/shared/display/display.cpp` (filters exclude root twin). Root `firmware/src/display.cpp` is a **stale twin** (differs — shared is newer: `s_touch_active`, `displayTouchActive()`). Safe delete target for Phase 1 Execute: **root** `src/display.cpp` only. Keep `shared/display/display.cpp`.

Child app dir `firmware/src/child/` is **gone**. Orphans remaining in the live tree:

| Path | Issue |
|------|--------|
| `firmware/src/child_focus_state.cpp` | On disk; excluded on Waveshare; **not** excluded on `devkit` |
| `firmware/include/child_focus_state.h` | Still included by `ui_manager.h` + `api_client.h` |
| `firmware/include/ui_manager.h` | Full child LVGL / reward surface under `#if !defined(FAMILY_HUB_APP_HOUSEHOLD)` |
| `firmware/include/api_client.h` | Unconditional child/display API declarations + `#include "child_focus_state.h"` |
| `firmware/src/shared/ui/ui_manager_core.cpp` | Child pointer wipe block when `FAMILY_HUB_APP_HOUSEHOLD` unset |
| `firmware/docs/child-ui-rebuild.md` | Stale child launcher instructions |

**Critical:** `waveshare7b` does **not** define `FAMILY_HUB_APP_HOUSEHOLD=1`, so the production env still compiles the **child half of the `UiManager` class layout** (members/methods) even with no child `.cpp`. Launcher env correctly sets the flag. Phase 1 must align `waveshare7b` with household-only macros.

Child API method **bodies** live only in `firmware/archive/api_client.cpp` — live household link does not provide them (OK if unreferenced). Goal is to remove declarations so the household surface cannot regress into calling ghosts.

---

## 2. Work packages (ordered)

### WP-1 — Macro alignment

**Files:** [firmware/platformio.ini](../firmware/platformio.ini)

1. Add to `env:waveshare7b` `build_flags`:
   - `-DFAMILY_HUB_APP_HOUSEHOLD=1`
   - `-DFAMILY_HUB_FIRMWARE_KIND=\"household\"` (optional but matches launcher)
2. Keep launcher `extends` as-is (already sets household).
3. Confirm `devkit` keeps `-DFAMILY_HUB_APP_HOUSEHOLD=1`.
4. Add to `devkit` `build_src_filter`: `-<child_focus_state.cpp>` and `-<display.cpp>` (belt-and-suspenders even if display unused without Waveshare).

**Done =** `waveshare7b` and launcher both compile with household-only `UiManager` layout.

### WP-2 — Delete stale display twin

1. Move or delete `firmware/src/display.cpp` (prefer move to `firmware/archive/display.cpp.root-stale-20260726.cpp` if any doubt; otherwise delete).
2. Do **not** touch `firmware/src/shared/display/display.cpp` or `include/display.h` except if a comment still points at root `display.cpp`.
3. Keep existing `-<display.cpp>` filters (harmless after delete) or remove as cleanup.

**Done =** only one display TU exists under `src/`; Waveshare env still links `shared/display`.

### WP-3 — Strip child from household headers / core

**Primary files:**

| File | Action |
|------|--------|
| `firmware/include/ui_manager.h` | Remove `#include "child_focus_state.h"`; delete all `#if !defined(FAMILY_HUB_APP_HOUSEHOLD)` child API / members / friends; keep household `ScreenId` + LVGL household tree |
| `firmware/include/api_client.h` | Remove `#include "child_focus_state.h"`; remove child/display action methods (`fetchDisplayHome`, `fetchChildMode`, reward/PIN/runtime, etc.); keep `fetchDashboardState`, `completeChore`, Wi-Fi/server/token; decide PanelImage: keep for now if Waveshare household might use later, or strip if unused by household UI (audit refs — if only child used images, strip under Phase 1) |
| `firmware/src/shared/ui/ui_manager_core.cpp` | Remove `#if !defined(FAMILY_HUB_APP_HOUSEHOLD)` child pointer wipe block |
| `firmware/src/child_focus_state.cpp` | Delete or move to archive sibling note — not compiled |
| `firmware/include/child_focus_state.h` | Delete or move to archive; no live includes remain |
| `firmware/include/household/compat/legacy.h` | Re-check includes still resolve |

**PanelImage audit rule:** If no household `.cpp` references `PanelImage` / `fetchPanelImage`, strip from `api_client.h` in the same PR to avoid dead child media path. If Waveshare display path still needs types for future chrome, leave types but drop fetch APIs.

**Done =** `rg -i 'child_focus|ChildFocus|fetchChildMode|rebuildChild'` under `firmware/include` + `firmware/src` (excluding `archive/` and `docs/`) returns empty or only historical comments.

### WP-4 — Doc / script hygiene (same phase, no firmware behavior)

1. Banner [firmware/docs/child-ui-rebuild.md](../firmware/docs/child-ui-rebuild.md): **ARCHIVED** → point to `family-hub-child-panel-archive-20260726`.
2. Note in [firmware/archive/README.md](../firmware/archive/README.md): child launcher env retired from live `platformio.ini`.
3. Leave `scripts/split_ui_manager.py` as archive tooling; add one-line comment “child extract path archived”.

### WP-5 — Verify (Execute acceptance)

```bash
cd firmware
pio run -e waveshare7b
pio run -e waveshare7b-household-launcher
pio run -e devkit
```

Symbol check (adjust nm tool name to toolchain):

```bash
ELF=.pio/build/waveshare7b/firmware.elf
nm -C "$ELF" | rg -i 'ChildFocus|rebuildChild|fetchChild|renderChild|childMode' && echo FAIL || echo PASS
```

**Done =** all three envs build; symbol check PASS on `waveshare7b` (+ launcher optional); no `/opt` deploy; Aikido scan on modified first-party sources.

---

## 3. Risk register

| Risk | Mitigation |
|------|------------|
| Deleting root `display.cpp` while some env still needs it | Filters already exclude on Waveshare; archive copy first if unsure |
| Stripping `PanelImage` breaks Waveshare compile | Grep before remove; keep types if referenced |
| `ui_manager.h` split too aggressive → household_ui.cpp fails | Build after each WP; keep household public API (`render`, `setScreen`, chore modal) unchanged |
| Empty `.git` on family-hub | Doc/firmware edits still OK locally; commit story is separate ops item |
| Accidental Phase 2 scope | Stop at hygiene; no sleep/gestures |

---

## 4. Explicit non-goals (Phase 1)

- Sleep pipeline / backlight black-fill
- `ShellGestures` / touch long-press / stick
- Diagnostics overlay
- View-model contracts / `docs/screens/`
- Remount rewards or child archives
- systemd / `/opt/family-hub`
- Enabling `WRITE_TOKEN`

---

## 5. CEO ★ gate

Phase 1 **planning** is complete with this document.  
Phase 1 **Execute** (file deletes + header surgery + `pio run`) requires explicit CEO authorization.

### Paste-ready Execute prompt

```text
/3dl-matrix-cto Execute Family Hub Panel UX Phase 1 hygiene per docs/panel-ux-cleanup-phase1-plan.md.

Authority: CEO ★ Phase 1 authorized.
Follow WP-1 → WP-5 in order. Cite Done= from that plan and docs/panel-ux-cleanup-addendum.md §7 Phase 1.

Constraints:
- Do NOT start Phase 2 (sleep/gestures/diagnostics)
- Do NOT remount archives or deploy systemd
- Prefer archive-then-delete for root display.cpp and child_focus_state.*
- Aikido scan modified first-party sources before complete
```
