# Family Hub — Panel UX Phase 2 Plan (Shell UX)

**Status:** Phase 2 Execute **COMPLETE** (2026-07-26) — CEO ★ authorized · builds PASS · touch H1–H6 PASS on panel · H7 pending (API-down boot)  
**Date:** 2026-07-26  
**Product lock:** [panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md) §§4–5, §7 Phase 2  
**Depends on:** Phase 1 hygiene **COMPLETE**  
**Out of scope:** contracts / `docs/screens/` (Phase 3), sleep-as-deploy, archive remount, WRITE_TOKEN, rewards/child

---

## 8. Execute record (2026-07-26)

| Check | Result |
|-------|--------|
| WP-A sleep API | `displayEnterSleep/Wake`, black FB fill ×2, flush gated, `[disp] sleep/wake` |
| WP-B ShellGestures | `shared/input/shell_gestures.cpp` — touch chrome; stick stub |
| WP-C diagnostics | top-layer overlay `shared/diagnostics/diagnostics_ui.cpp` |
| WP-D main | sleep gate in loop; boot → diag if invalid dashboard; keys `z`/`d` |
| `pio run -e devkit` | SUCCESS |
| `pio run -e waveshare7b` | SUCCESS |
| `pio run -e waveshare7b-household-launcher` | SUCCESS → `dist/family-hub-household.bin` |
| Hardware H1–H6 | **PASS** — see `docs/verification-evidence/phase2-touch-scorecard-20260726.md` |
| Hardware H7 | **Pending** — needs brief API stop + cold boot (confirm before kill) |
| Aikido | scanned new shell modules |

---

## 1. Goal

Make the household Waveshare panel feel like Bedroom Calendar Hub’s shell:

1. **Real sleep** — black fill → settle → backlight off → pause LVGL flushes (no burn-in path)
2. **ShellGestures (choice C)** — shared API; touch adapter (primary) + joystick SW adapter (Elecrow secondary / stub on Waveshare)
3. **Diagnostics overlay** — gesture toggle; boot fail-open when no valid dashboard

**Not in Phase 2:** splitting `household_ui.cpp` into per-screen files, typed view-model contracts, full `firmware/{board,input,...}` directory rename. Keep changes inside current tree with a thin new module set; map toward target layout without a big-bang move.

---

## 2. Audit (Known)

| Area | Current FH | Calendar reference |
|------|------------|-------------------|
| Backlight | `displaySetBacklight()` in `firmware/src/shared/display/display.cpp` (CH32V003 PWM) | `CrowPanelDisplay::enterSleep/wake` black fill + BL |
| LVGL pump | `displayTick()` always runs `lv_timer_handler` when ready | `lvglShellLoop` no-ops when `g_sleeping` |
| Sleep state | **None** | `g_display_asleep` + enter/wake in `calender-display/firmware/src/main.cpp` |
| Gestures | Tap-only LVGL (`LV_EVENT_CLICKED`) | Stick double-click sleep / hold 2s diag |
| Chrome | Header bar **70 px** (`household_ui.cpp` `lv_obj_set_size(header, PANEL_H_RES, 70)`) | Nav + stick |
| Diagnostics UI | Serial + boot stage logs only | `diagnostics/diagnostics_ui.cpp` overlay |
| Boot | Always `renderDashboard("initial…")` even if empty/offline | Invalid dashboard → Diagnostics / Setup |
| Stick | Not in live FH Waveshare build | `joystick_driver.cpp` on Elecrow |

**Assumed:** Phase 2 ships fully on `waveshare7b` (+ launcher). Joystick adapter compiles on Waveshare as **no-op** (`joystickAvailable()==false`); real stick wiring lands when `elecrow7` leave stub state (still Phase 2 code paths, hardware checklist deferred for stick).

**Unknown until hardware check:** exact RGB black-fill API on Waveshare `esp_lcd` path (may need draw black via LVGL full-screen obj or panel draw bitmap — see WP-2 risk).

---

## 3. Architecture (Phase 2 slice)

```text
loop()
  ├─ shellGesturesPoll()     // touch + stick → SleepToggle | DiagToggle
  ├─ if (!displayIsSleeping())
  │     process sync / chore / uiShell deferred work
  └─ displayTick()           // no-op LVGL when sleeping

ShellGestures
  ├─ TouchChromeSource       // long-press / double-tap on header Y < 70
  └─ JoystickSwSource        // hold / double-click (no-op if no stick)
```

Constants (lock in plan; tune only with evidence):

| Constant | Value | Source |
|----------|-------|--------|
| `kChromeHeightPx` | 70 | FH header |
| `kHoldDiagMs` | 2000 | Calendar |
| `kDoubleActivateMs` | 450 | Calendar |
| `kSleepSettleMs` | ~40 × 2 | Calendar |

---

## 4. Work packages (Execute order)

### WP-A — Display sleep API (`shared/display`)

**Files:** `firmware/include/display.h`, `firmware/src/shared/display/display.cpp`

Add:

```cpp
void displayEnterSleep();   // black ×2 + settle + backlight 0; log [disp] sleep
void displayWake();         // backlight restore; log [disp] wake
bool displayIsSleeping();
void displayForceFullRedraw();  // invalidate act/top/sys + lv_refr_now
```

Behavior:

1. `displayEnterSleep`: set sleeping flag **before** blanking; suppress flush path in `displayTick` / flush callback (mirror `lvglShellSetSleeping`)
2. Black fill: prefer panel-safe full black (investigate: solid LVGL screen fill under lock, or RGB draw). **Must not** only call `displaySetBacklight(0)` while RGB keeps scanning UI
3. `displayWake`: clear sleeping; restore backlight to prior/default percent; `displayForceFullRedraw()`
4. Non-`WAVESHARE_7B`: no-op stubs (devkit)

**Done =** unit of behavior callable from serial test key (e.g. `z` toggle sleep) on Waveshare build; serial shows `[disp] sleep` / `[disp] wake`; after sleep, no LVGL flush activity.

### WP-B — ShellGestures module (`shared/input/` or `household/`)

**New files (proposed):**

| Path | Role |
|------|------|
| `firmware/include/shell_gestures.h` | `shellGesturesBegin()`, `shellGesturesPoll()`, event enum |
| `firmware/src/shared/input/shell_gestures.cpp` | Deconflict + dispatch |
| `firmware/src/shared/input/touch_chrome_source.cpp` | Touch long-press / double-tap in chrome |
| `firmware/src/shared/input/joystick_sw_source.cpp` | Stick SW; no-op if unavailable |

**API sketch:**

```cpp
enum class ShellGesture : uint8_t { None, SleepToggle, DiagnosticsToggle };

void shellGesturesBegin();
ShellGesture shellGesturesPoll();  // at most one action per call; sleep wins over diag
```

**Touch chrome rules:**

- Only consider presses with `y < kChromeHeightPx` (status/header), **not** chore rows / nav / modal
- Rising-edge tracking for double-tap window
- Hold ≥2s → DiagnosticsToggle; cancel hold latch if double-tap fires
- While sleeping: only SleepToggle (wake); ignore DiagnosticsToggle
- While modal open: ignore chrome gestures (or only allow sleep — prefer **ignore both** to avoid accidents)

**Stick rules:** Port calendar `pollSleepDoublePress` / hold logic; if `!joystickPresent()`, source returns idle.

**PlatformIO:** add `+<shared/input/>` implicitly via `+<shared/>` on Waveshare; ensure `devkit` filter includes stubs that compile.

**Done =** `pio run -e waveshare7b` links module; serial debug lines on gesture fire (`[gest] sleep` / `[gest] diag`).

### WP-C — Diagnostics overlay

**New files (proposed):**

| Path | Role |
|------|------|
| `firmware/include/diagnostics_ui.h` | init / toggle / visible / render |
| `firmware/src/shared/diagnostics/diagnostics_ui.cpp` | LVGL overlay (not nav tab) |

**Fields (addendum):** Wi-Fi SSID/RSSI or down, API host:port, last HTTP code/error, firmware version, free heap, dashboard validity (`dashboardStateValid` — stand-in until Phase 3 `schema_version`), button **Open Settings** → `ui.setScreen(Settings)` + hide overlay.

**Toggle:** `uiToggleDiagnostics()` pattern — show/hide overlay on `lv_layer_top()` or dedicated screen object; does not destroy household tree.

**Boot (household/main.cpp):**

```text
after displayBegin + first sync attempt:
  if displayReady && !dashboardStateValid(dashboardDoc) && !bootCacheLoaded:
      show Diagnostics (not empty Home as “success”)
  else:
      renderDashboard as today
```

**Done =** hold/long-press toggles overlay; Close / hold again hides; Open Settings works; boot path verified on forced Wi-Fi fail (serial + panel).

### WP-D — Wire into `household/main.cpp`

1. `shellGesturesBegin()` after `displayBegin`
2. Each `loop()`: poll gestures → handle SleepToggle / DiagnosticsToggle **before** heavy sync when asleep
3. When `displayIsSleeping()`: skip dashboard poll/render/chore HTTP (or keep Wi-Fi reconnect only); still call `displayTick()` (no-op flush)
4. Optional serial: `z` = sleep toggle, `d` = diagnostics toggle (dev aid)
5. On wake: `displayWake` + if diag visible re-render diag else `renderDashboard("wake")`

**Done =** loop respects sleep; no chore complete while asleep.

### WP-E — Verify (hardware + build)

```bash
cd firmware
pio run -e waveshare7b
pio run -e waveshare7b-household-launcher
pio run -e devkit
```

**Hardware checklist (Waveshare):**

| # | Check | Pass |
|---|-------|------|
| H1 | Double-tap header → sleep; screen black; `[disp] sleep` | **PASS** |
| H2 | Double-tap (or any chrome double-activate) → wake; UI redraws; `[disp] wake` | **PASS** |
| H3 | After sleep, no burn-in / no static UI ghost with BL off | **PASS*** (serial + visual) |
| H4 | Long-press header ≥2s → Diagnostics; again → hide | **PASS** |
| H5 | Double-tap does **not** also open Diagnostics | **PASS** |
| H6 | Chore row tap / nav tap unaffected (no accidental sleep/diag) | **PASS*** |
| H7 | Boot with server down → Diagnostics (or clear offline), not fake healthy Home | pending (API-stop confirm) |

**Stick (Elecrow):** mark **DEFERRED** until `elecrow7` non-stub; code paths must compile.

**Aikido:** scan new/modified first-party sources before calling Execute complete.

---

## 5. Explicit non-goals (Phase 2)

- Phase 3 contracts / per-screen VMs / `docs/screens/`
- Full firmware directory restructure to calendar `board/network/contracts/...`
- On-panel Wi-Fi SSID/PSK provisioning (Settings host/token only)
- Remount child/rewards
- systemd / `/opt` deploy
- Changing LAN auth policy

---

## 6. Risks

| Risk | Mitigation |
|------|------------|
| Waveshare cannot `fillScreen(BLACK)` like LovyanGFX | Prototype black via LVGL full-screen obj under `displayLock`, then BL off; document chosen path in Execute notes |
| Touch long-press steals LVGL clicks on header sync button | Prefer empty chrome / status pill region; exclude sync button hitbox; or require long-press on left brand mark only |
| Sleep while modal open loses modal state | Wake restores tree (not destroyed); test modal + sleep |
| `displayTick` still flushing during sleep | Gate flush callback + early-return in `displayTick` |
| Scope creep into contracts | Stop after shell; schema field = validity bool only |

**Gates:** CEO ★ before Execute. No Deploy/Secrets/Auth changes in this phase.

---

## 7. CEO ★ gate

Phase 2 **planning** = this document.  
Phase 2 **Execute** = WP-A→E after explicit authorization.

### Paste-ready Execute prompt

```text
/3dl-matrix-cto Execute Family Hub Panel UX Phase 2 shell UX per docs/panel-ux-cleanup-phase2-plan.md.

Authority: CEO ★ Phase 2 authorized.
Follow WP-A → WP-E. Cite Done= from that plan and docs/panel-ux-cleanup-addendum.md §7 Phase 2.

Constraints:
- Do NOT start Phase 3 contracts
- Do NOT remount archives or deploy systemd
- Waveshare primary; joystick no-op OK until elecrow7
- Aikido scan modified first-party sources before complete
- Prefer reversible display sleep API; no burn-in path
```
