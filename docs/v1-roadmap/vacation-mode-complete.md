# Vacation Mode — Complete (Hardware-Deferred V1)

**Date:** 2026-06-18  
**Status:** **V1-without-panel SHIPPABLE** — panel work deferred to V1.1 / Phase F–G

**Last autonomous run:** 2026-06-19T01:58 UTC — baseline **PASS** (npm test 9/9, smoke local+LAN, firmware devkit/waveshare7/elecrow7); Phase E deploy **BLOCKED** (SSH W-Y04, systemd inactive); panel **WAIVED** W-HW01. Evidence: `docs/verification-evidence/vacation-run-final.md` § 2026-06-19.

---

## What was completed without panel

| Area | Done |
|------|------|
| v0.1 repair (Phases A–D) | Server tests, auth, contract, safety — green |
| Automated baseline | `npm test` 9/9 pass; firmware envs defined (`devkit`, `waveshare7`, `elecrow7`) |
| V1 roadmap pack | Board gates, phase plan, decisions register, waivers |
| Waveshare prep docs | [waveshare-esp32-s3-touch-lcd-7-reference.md](../waveshare-esp32-s3-touch-lcd-7-reference.md) |
| Elecrow prep docs | [elecrow-esp32-display-7-reference.md](../elecrow-esp32-display-7-reference.md), [panel-targets.md](../panel-targets.md) |
| Deploy documentation | [endeavor-deploy.md](../endeavor-deploy.md) + manual steps in verification evidence |
| LAN verification (prior) | Health + smoke PASS at `192.168.1.132:3020` (see `E-smoke-lan-20260618`) |
| Panel checklist | All P1–P16 marked **WAIVED — W-HW01** (hardware not received) |

**V1-without-panel definition:** endeavor runs Family Hub API + browser admin on LAN; smoke + tests pass; no ESP32/LVGL required for this milestone.

---

## What remains when panel hardware arrives

**Dual-panel hub:** [panel-targets.md](../panel-targets.md) — Waveshare **primary**, Elecrow **secondary (V1.1 bonus)**.

### Waveshare (primary — V1-full)

- **Product:** [Waveshare ESP32-S3-Touch-LCD-7](https://www.waveshare.com/product/arduino/boards/esp32-s3/esp32-s3-touch-lcd-7.htm)
- **Docs:** https://docs.waveshare.com/ESP32-S3-Touch-LCD-7
- **Env:** `waveshare7`

### Elecrow (secondary — optional)

- **Product:** [CrowPanel 7" HMI DIS08070H-1](https://www.elecrow.com/esp32-display-7-inch-hmi-display-rgb-tft-lcd-touch-screen-support-lvgl.html)
- **Docs:** [elecrow-esp32-display-7-reference.md](../elecrow-esp32-display-7-reference.md)
- **Env:** `elecrow7` — run **after** G-Waveshare unless you only own Elecrow

### Phase F checklist

- [ ] Create `firmware/include/secrets.h` (WiFi + `DEFAULT_SERVER_HOST`)
- [ ] `pio run -e devkit` — optional devkit WiFi/API proof
- [ ] `pio run -e waveshare7` — build with Waveshare + LVGL libs
- [ ] Flash panel; serial/monitor: WiFi connected, dashboard fetch OK
- [ ] Write test (`a`/`c`/`d`/`t` keys or touch) confirmed on server

### Phase G checklist (G-Waveshare — required for V1-full)

- [ ] Add `ESP32_Display_Panel`, `ESP32_IO_Expander`, `lvgl@8.4.0` per [Waveshare reference](../waveshare-esp32-s3-touch-lcd-7-reference.md)
- [ ] LVGL UI in `ui_manager.cpp` `#ifdef WAVESHARE_7` — home, badge, touch writes, settings
- [ ] On-screen offline badge (WiFi + server down)
- [ ] Photo evidence + checklist P10–P16 → PASS
- [ ] Bump `FIRMWARE_VERSION` to `1.0.0`; board log **SHIPPED** for V1-full

### Phase G checklist (G-Elecrow — optional V1.1)

- [ ] LovyanGFX + GT911 + Elecrow board JSON per [Elecrow reference](../elecrow-esp32-display-7-reference.md)
- [ ] `#elif defined(ELECROW_7)` in `ui_manager.cpp`
- [ ] `pio run -e elecrow7 -t upload`; touch verified (see troubleshooting if V3.0)
- [ ] Photo evidence `G-elecrow-panel-home-*.jpg`

---

## When back

If you are on **dev-pc** (this machine at `192.168.1.132`), close Phase E using **[Deploy on dev-pc](../endeavor-deploy.md#deploy-on-dev-pc-same-machine-as-1921681132)** in `endeavor-deploy.md` — run install and `systemctl enable --now family-hub` locally after stopping dev `npm start`. Do not treat SSH `publickey` failure as a deploy blocker on that host; verify **S9** (`systemctl is-active family-hub`) and **S8** (dashboard unchanged after `systemctl restart family-hub`). Panel work still follows Phase F/G below when hardware arrives.


## One-command resume

When the panel arrives, open Phase F in the phase plan and execute from step F.1:

```bash
# From repo root — then follow steps in v1-phase-plan.md Phase F → G
less docs/v1-roadmap/v1-phase-plan.md   # jump to "## Phase F — Hardware Verification"
```

Or tell an agent:

> **When panel arrives, run Phase F from `docs/v1-roadmap/v1-phase-plan.md`**, then **G-Waveshare** (required), optionally **G-Elecrow**, using [panel-targets.md](../panel-targets.md) and the panel reference docs.

---

## Related docs

| Doc | Purpose |
|-----|---------|
| [v1-phase-plan.md](v1-phase-plan.md) | F/G steps |
| [02-cpo-product-gate.md](02-cpo-product-gate.md) | V1.0 vs V1.1 acceptance |
| [v1-verification-checklist.md](v1-verification-checklist.md) | Panel rows waived |
| [vacation-run-final.md](../verification-evidence/vacation-run-final.md) | Test + deploy evidence |
