# Family Hub — Panel UX Cleanup Addendum

**Status:** Phase 0 product lock (docs only)  
**Date:** 2026-07-26  
**Owner:** CPO (product) → CTO (Execute Phases 1–3)  
**Reference product:** Bedroom Calendar Hub (private sibling project — patterns only; not in this repo)  
**Scope:** **Household-only.** Rewards/behavior and child panel stay archived (see [archive/REWARDS_BEHAVIOR_MOVED.md](../archive/REWARDS_BEHAVIOR_MOVED.md), [archive/CHILD_PANEL_MOVED.md](../archive/CHILD_PANEL_MOVED.md)).

This addendum does **not** rewrite [architecture-book.md](architecture-book.md). It locks the panel UX cleanup direction so firmware work has a product gate.

**Gesture choice:** **C** — shared `ShellGestures` API with touch (Waveshare primary) and joystick SW (Elecrow secondary) adapters.

**No production firmware in Phase 0.** Phase 1+ requires CEO ★ before Execute.

---

## 1. Steal list (from calendar)

Patterns to adopt. Cite calendar paths; do not merge repos.

| Pattern | Calendar evidence | FH intent |
|---------|-------------------|-----------|
| Thin client + server source of truth | `calender-display/docs/adr/002-panel-thin-client.md` | Keep; reinforce in shell |
| Fixed screen contracts + `schema_version` | `docs/adr/003-fixed-screen-contracts.md`, `docs/api/panel-contracts.md` | Per-screen VMs (Phase 3) |
| Typed payload validators | `firmware/src/contracts/*` | UI never binds raw JSON widgets |
| Module boundaries | `docs/firmware/module-boundaries.md` | Target tree in §5 |
| Real sleep (not backlight-only) | `firmware/src/board/display_driver.cpp` (`enterSleep`), `firmware/src/board/lvgl_shell.cpp` | Black fill ×2 + settle → backlight 0 → pause LVGL flushes → wake full redraw |
| Diagnostics overlay + fail-open | `firmware/src/main.cpp` boot / `pollDiagGesture` | Gesture toggle; invalid home → Diagnostics, not fake Home |
| Deferred UI outside LVGL callbacks | `firmware/include/ui_shell.h` (`uiShellPoll`) | Avoid stack overflow / re-entrancy |
| Web owns editing; panel mutations minimal | `docs/adr/006-web-owns-editing.md` | Panel: chore complete + grocery toggle |
| Per-screen layout specs | `docs/screens/*-800x480.md` | FH `docs/screens/` in Phase 3 |

---

## 2. Anti-list (do not copy / do not reopen)

- Calendar domain (events, RRULE, month grid, occurrence engine)
- Cloudflare tunnel / public ghost host as FH default (LAN-only stays)
- Calendar `ADMIN_TOKEN` bearer web model as FH default (keep LAN trust + optional `WRITE_TOKEN` / `PANEL_TOKEN`)
- Merging `calender-display` and `family-hub` repos (calendar ADR-007: standalone; patterns OK)
- Remounting rewards/child archives or live `/api/v1/display/*` child APIs
- Joystick-only gestures without touch mapping (Waveshare is primary)
- Generic server-driven block / layout renderer
- MQTT / Home Assistant / high-risk device control

---

## 3. Screen list (product)

### Bottom nav (6) — household-only

| Screen | Job | Data (target contract) | Panel mutations |
|--------|-----|------------------------|-----------------|
| Home | At-a-glance: dinner today, open chores count, grocery count, pinned notes, connection badge | `home` VM (subset of today’s `GET /api/dashboard-state`) | none |
| Grocery | Scroll list | `grocery` VM | none (add via web) |
| Chores | Open chores | `chores` VM | complete (existing) |
| Dinner | Today + short week | `dinner` VM | none (set via web) |
| Notes | Pinned / list | `notes` VM | none |
| Settings | Host, port, device ID, fw, RSSI, last sync, token fields | local NVS + status | save host / port / token |

Full on-panel Wi-Fi SSID/PSK provisioning is **deferred** unless already present; Settings owns host/token + status for Phase 0–2.

### Overlays (not nav tabs)

| Overlay | Role |
|---------|------|
| **Diagnostics** | Wi-Fi, API, last error, schema/version, heap; gesture entry only |
| **Chore confirm modal** | Keep existing household confirm flow |

### Out of panel scope

Member CRUD, grocery add, dinner edit, notes compose, rewards, child focus.

### UX acceptance

- At ~1 m, Home answers “what’s for dinner / what’s open” within a few seconds of glance
- Offline / stale is visible
- Sleep never leaves a static RGB image lit (no burn-in path)
- Diagnostics reachable without opening Settings

---

## 4. Gesture matrix (choice C)

One `ShellGestures` API; two input adapters. Nav remains **tap-only**. Diagnostics is **not** a bottom-nav tab.

| Action | Touch (Waveshare primary) | Stick SW (Elecrow secondary) |
|--------|---------------------------|------------------------------|
| Diagnostics | Long-press ≥2s on empty chrome / status bar (**not** on list rows) | Hold ≥2s |
| Sleep / wake | Double-tap same chrome zone | Double-click within ~450 ms |
| Deconflict | Double-tap cancels hold latch (mirror calendar) | Same |

Chrome-only gesture zone is mandatory so chore / list taps are not stolen.

Reference implementation (stick): `calender-display/firmware/src/main.cpp` (`pollSleepDoublePress`, `pollDiagGesture`).

---

## 5. Sleep / diagnostics UX

### Sleep pipeline

1. **Enter:** double-activate → board-specific black fill ×2 with frame settle → backlight 0 → `lvglSleeping=true` (no flushes) → ignore nav taps; wake gesture still live  
2. **Wake:** restore backlight → force full LVGL redraw → resume poll  
3. **Done =** no burn-in path; serial logs `[disp] sleep` / `[disp] wake`; photo evidence optional  

Calendar explicitly rejects “backlight off while RGB keeps scanning a static UI” (`calender-display/firmware/src/board/display_driver.cpp`).

### Diagnostics

**Fields:** Wi-Fi SSID/RSSI or down, API host:port, last HTTP code/error, firmware version, free heap, `schema_version` accepted/rejected, “Open Settings” affordance.

**Boot:** if no valid home payload after Wi-Fi attempts → show Diagnostics (not empty fake Home).

**Done =** hold / long-press toggles Diagnostics; double-activate never also opens Diagnostics.

---

## 6. Firmware module map (current → target)

### Current (messy)

```text
firmware/src/
  display.cpp                         # DUPLICATE — remove in Phase 1
  shared/display/display.cpp          # Waveshare bring-up (keep → board/)
  household/main.cpp                  # loop + sync + serial keys
  household/household_ui.cpp          # monolith screens + modal
  household/household_api_client.cpp
  household/household_state_cache.cpp
  shared/ui/ui_manager_core.cpp       # serial fallback residue
  dashboard_state.cpp
  child_focus_state.cpp               # orphan — fence/delete from household builds
  shared/api|diagnostics|state_cache/
```

Header debt: `firmware/include/ui_manager.h` still carries child LVGL / reward surface.

### Target (calendar-shaped, household content)

```text
firmware/
  board/          # Waveshare display/touch (+ Elecrow ifdef later)
  network/        # wifi, api_client, etag/poll
  contracts/      # home, grocery, chores, dinner, notes validators + VMs
  state/          # app_state, nav, sleep, stale
  ui/             # shell (nav, overlays, deferred poll) + per-screen files
  actions/        # complete_chore only
  diagnostics/    # diagnostics_ui
  input/          # ShellGestures + touch_source + joystick_source
```

**Map:**

| Current | Target |
|---------|--------|
| `household/household_ui.cpp` | `ui/shell` + `ui/home|grocery|chores|dinner|notes|settings` |
| `household/main.cpp` | thin boot + `ShellGestures` poll (calendar `main.cpp` pattern) |
| `shared/display/display.cpp` | `board/` |
| `dashboard_state.cpp` | evolve into `contracts/` + `state/` |

---

## 7. Phased plan and Done= criteria

### Phase 0 — Product lock (this document)

- Addendum committed (steal/anti-list, screens, gestures C, sleep/diag, module map)
- README scope + link; [firmware-split.md](firmware-split.md) child sections marked ARCHIVED
- **Done =** this file exists; README states household-only and points here; no firmware source edits in this phase

### Phase 1 — Hygiene (CTO Execute — CEO ★ required) — **COMPLETE 2026-07-26**

- Engineering plan: [panel-ux-cleanup-phase1-plan.md](panel-ux-cleanup-phase1-plan.md)
- Deleted duplicate `firmware/src/display.cpp` (archived); stripped child from household headers; archived `child_focus_state.*`
- Aligned `waveshare7b` with `-DFAMILY_HUB_APP_HOUSEHOLD=1`
- **Done =** `pio run -e waveshare7b`, `-e waveshare7b-household-launcher`, `-e devkit` SUCCESS; nm child-symbol check PASS

### Phase 2 — Shell UX (sleep + gestures + diagnostics) — **COMPLETE 2026-07-26**

- Engineering plan: [panel-ux-cleanup-phase2-plan.md](panel-ux-cleanup-phase2-plan.md)
- Sleep API + ShellGestures (touch chrome; stick no-op) + diagnostics overlay wired in `household/main.cpp`
- **Done =** builds PASS; serial `z`/`d` PASS; touch H1–H6 PASS (`docs/verification-evidence/phase2-touch-scorecard-20260726.md`); H7 deferred (API-down boot needs confirm)

### Phase 3 — Contracts — **COMPLETE 2026-07-26**

- Engineering plan: [panel-ux-cleanup-phase3-plan.md](panel-ux-cleanup-phase3-plan.md)
- Server view-model endpoints or versioned dashboard fields; firmware validators; UI binds VMs only
- Screen specs under `docs/screens/` (1024×600)
- **Done =** `schema_version: 1` + nested VMs; firmware rejects unsupported schema; Home/Grocery/Chores/Dinner/Notes bind VMs; chore complete path intact; builds + serial C1/C4 PASS; Aikido clean

### Phase 4 — Polish + prod path — **COMPLETE 2026-07-26** (systemd = operator)

- Engineering plan: [panel-ux-cleanup-phase4-plan.md](panel-ux-cleanup-phase4-plan.md)
- Acceptance: [verification-evidence/phase4-panel-ux-acceptance-20260726.md](verification-evidence/phase4-panel-ux-acceptance-20260726.md)
- Browser admin coherent with VMs; smoke asserts `schema_version`
- systemd: `tools/phase4-local-systemd-deploy.sh` (S8/S9) — run on host; agent sandbox blocks `/opt` chown
- **Done =** panel UX acceptance + smoke contract assert + deploy script ready

### Follow-on (planning) — Calendar API/web borrow

- [calendar-arch-borrow-plan.md](calendar-arch-borrow-plan.md) — ADRs, JSON Schema tests, `state_version`/ETag, viewModels; CEO ★ before Execute

### Explicit non-goals until new CEO ★

Remount rewards/child; public exposure; force-enable `WRITE_TOKEN`.

---

## 8. Related paths

| Path | Role |
|------|------|
| [architecture-book.md](architecture-book.md) | Canonical SoT / safety |
| [adr/README.md](adr/README.md) | FH ADRs (calendar-pattern borrow C0) |
| [calendar-arch-borrow-plan.md](calendar-arch-borrow-plan.md) | Calendar API/web borrow plan (C0 done; C1+ ★) |
| [firmware-split.md](firmware-split.md) | Historical split audit; child **ARCHIVED** |
| [panel-targets.md](panel-targets.md) | Waveshare primary, Elecrow secondary |
| `../archive/CHILD_PANEL_MOVED.md` | Child archive pointer |
| `../archive/REWARDS_BEHAVIOR_MOVED.md` | Rewards archive pointer |
| `../../calender-display/docs/architecture-book.md` | Reference Architecture Book |
| [panel-ux-cleanup-cto-handoff-phase1.md](panel-ux-cleanup-cto-handoff-phase1.md) | Phase 1 CTO hygiene handoff (CEO ★ before Execute) |
| [panel-ux-cleanup-phase1-plan.md](panel-ux-cleanup-phase1-plan.md) | Phase 1 engineering plan (audit, WPs, Done=) |
| [panel-ux-cleanup-phase2-plan.md](panel-ux-cleanup-phase2-plan.md) | Phase 2 shell UX plan (sleep, gestures, diagnostics) |
| [panel-ux-cleanup-phase3-plan.md](panel-ux-cleanup-phase3-plan.md) | Phase 3 contracts plan (VMs + schema_version) |
| [panel-ux-cleanup-cto-handoff-phase3.md](panel-ux-cleanup-cto-handoff-phase3.md) | Phase 3 CTO handoff (CEO ★ before Execute) |
| [panel-ux-cleanup-phase4-plan.md](panel-ux-cleanup-phase4-plan.md) | Phase 4 polish + prod path |
| [calendar-arch-borrow-plan.md](calendar-arch-borrow-plan.md) | Calendar API/web borrow plan (CEO ★ before Execute) |
| [verification-evidence/phase4-panel-ux-acceptance-20260726.md](verification-evidence/phase4-panel-ux-acceptance-20260726.md) | Phase 4 acceptance rollup |

