# Waveshare Panel Setup (ESP32-S3-Touch-LCD-7B)

> **Dual-panel hub:** [panel-targets.md](panel-targets.md) compares Waveshare (primary) vs Elecrow (secondary). Panel-specific references: [waveshare-esp32-s3-touch-lcd-7b-reference.md](waveshare-esp32-s3-touch-lcd-7b-reference.md) (primary board), [waveshare-esp32-s3-touch-lcd-7-reference.md](waveshare-esp32-s3-touch-lcd-7-reference.md) (old 800×480 board — historical), [elecrow-esp32-display-7-reference.md](elecrow-esp32-display-7-reference.md).

Target panel: **Waveshare ESP32-S3-Touch-LCD-7B** (1024×600 touch, Type B).

**Official docs:** https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7B
**Bring-up reference (read this first):** [waveshare-esp32-s3-touch-lcd-7b-reference.md](waveshare-esp32-s3-touch-lcd-7b-reference.md)

## Firmware environments

| Environment | Purpose |
|-------------|---------|
| `devkit` | Generic ESP32-S3 devkit — serial UI fallback for connectivity testing |
| `waveshare7b` | **Primary** production panel — full LVGL + RGB panel + GT911 stack (home/dev; may use `secrets.h`) |
| `waveshare7b-public` | Public-prep image — **no** baked Wi-Fi PSK / write token; NVS + Settings only |
| `elecrow7` | **Secondary** Elecrow 7" — stub until LovyanGFX stack (see [elecrow reference](elecrow-esp32-display-7-reference.md)) |

> The old `waveshare7` env (800×480 -7 board) was replaced by `waveshare7b`.
> Historical docs/evidence mentioning `pio run -e waveshare7` refer to builds
> from before the board swap.

### Public-prep builds (`waveshare7b-public`)

`FAMILY_HUB_PUBLIC_BUILD=1` refuses compile-time secrets from `secrets.h` for
Wi-Fi and write token. First boot needs NVS `wifi_ssid` / `wifi_pass` (SoftAP
UI is out of scope) and server host/token via **Diagnostics → Open Settings**.

Internet / multi-tenant exposure still needs **C5** (hashed panel credentials +
server auth plane) — gated by CEO ★ / CISO; not enabled in this firmware pass.

## Configure

1. Copy `include/secrets.example.h` → `include/secrets.h` (gitignored). Build uses `__has_include("secrets.h")` automatically.
2. Set `WIFI_SSID`, `WIFI_PASSWORD`, `DEFAULT_SERVER_HOST` (endeavor LAN IP) for home `waveshare7b` builds
3. Optional write token: NVS `write_tok`, Diagnostics → Settings, or `-DFAMILY_HUB_WRITE_TOKEN=\"...\"` in `platformio.ini` (home builds only)
4. Build: `pio run -e waveshare7b` (or `waveshare7b-public` for NVS-only secrets)
5. Flash: `pio run -e waveshare7b -t upload` (recovery: hold BOOT, tap RESET, release BOOT, retry)
6. Monitor: `pio device monitor -b 115200`

## Panel role: view-only except chore completion (+ Settings)

The ESP32 panel is a **view-only household dashboard**. It reads and displays
dashboard state (chores, grocery, dinner, notes, members) and performs exactly
**one** server mutation: marking an existing chore complete, via the dedicated
endpoint `POST /api/chores/{id}/complete`. All creation, editing, deletion,
assignment, and household content configuration happen in the **Family Hub web UI**.

**Settings** (host / port / write token) is opened only from **Diagnostics →
Open Settings** — not from the bottom nav. That screen is device connectivity
config, not family content editing.

### Completing a chore from the panel (touch flow)

On the **Chores** tab, open chores render as tappable rows:

```
Chores tab
  → tap a chore row          (opens a read-only detail modal)
  → tap "Mark Complete"      (modal switches to confirm)
  → tap "Confirm"            (button disables: "Completing…")
  → POST /api/chores/{id}/complete   (runs in loop(), never in the callback)
  → on success: "Chore completed", modal closes, chore list re-syncs
  → on failure: "Unable to complete chore" (or "Server offline"), retry/Cancel
```

Guarantees built into the flow:
- HTTP runs in `loop()`, dispatched by an intent flag — never inside the LVGL
  button callback.
- The confirm button disables during the request; duplicate taps and duplicate
  HTTP requests are blocked (in-flight mode ignores further taps).
- The chore leaves the open list **only after** the server confirms and a
  re-sync — no optimistic update.
- Offline: completion is refused with a visible message; nothing is queued.
- Completion is a pure network op, so it runs with the RGB panel live (same as
  the dashboard fetch); SPI-flash cache writes remain deferred while active.

### Scoped panel credential

Chore completion is authorized by `panelCompleteAuth`, which accepts either the
full `WRITE_TOKEN` or a narrow `PANEL_TOKEN`. Provision the panel with
`PANEL_TOKEN` (not `WRITE_TOKEN`): it can then complete chores but is rejected
(401) on every other write route — create/edit/delete chores, grocery, notes,
dinner, members, events. Set it in the server env (`server/.env`):
`PANEL_TOKEN=<value>`, and on the panel via NVS `write_tok` or the
`FAMILY_HUB_WRITE_TOKEN` build flag. If neither token is set, the server is in
LAN-trust mode (open) as before.

## Panel serial commands (devkit testing)

| Key | Action |
|-----|--------|
| `n` | Next screen |
| `r` | Refresh from server |
| `c` | Complete first open chore (`POST /api/chores/{id}/complete`) |
| `h` | Reset server host/port to `secrets.h` defaults |
| `w` | Save test write token to NVS (`write_tok`) |

> The former `a` (grocery add), `d` (dinner set), and `t` (note add) write
> commands and their firmware helpers were removed — the panel no longer has
> any create/edit path. `c` is the only remaining write.

## Display stack (waveshare7b)

The `waveshare7b` env is self-contained — no Waveshare Arduino board package
or ESP32_Display_Panel library needed:

- pioarduino platform (Arduino core 3.x / IDF 5.x) — required by the esp_lcd
  RGB API the official BSP uses
- `lvgl@^8.4.0` with `include/lv_conf.h`
- Custom `src/display.cpp` mirroring the official BSP drivers
  (`rgb_lcd_port`, `io_extension`, `gt911`) — values documented in
  [waveshare-esp32-s3-touch-lcd-7b-reference.md](waveshare-esp32-s3-touch-lcd-7b-reference.md)
- `FIRST_BOOT_DIAG=1` build flag: boot-time I2C scan, color test screens,
  touch echo — set to 0 after hardware bring-up passes

UI render hooks live in `ui_manager.cpp` under `#ifdef WAVESHARE_7B`. Serial
fallback remains available for CI and devkit builds without display hardware.

## Server URL

Stored in NVS (`srv_host`, `srv_port`). Default from `secrets.h`. Change via Settings screen when LVGL UI is wired, or serial `h` to reset defaults.

## Write token

When endeavor has `WRITE_TOKEN` set, panel must send `x-family-hub-token`. Configure via NVS `write_tok` or `FAMILY_HUB_WRITE_TOKEN` build flag. Empty token = LAN trust mode.
