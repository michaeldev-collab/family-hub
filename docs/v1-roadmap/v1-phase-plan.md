# V1 Phase Plan — Phases E through I

**Prerequisite:** v0.1 repair Phases A–D mostly complete (see `docs/verification-checklist.md`).

---

## Phase E — endeavor Production Deploy

**Objective:** Family Hub API running on endeavor under systemd with passing smoke test.

### Steps

| Step | Action | Command / path | Expected output | Stop if |
|------|--------|----------------|-----------------|---------|
| E.1 | Backup existing DB (if any) | `ssh endeavor 'test -f /var/lib/family-hub/family-hub.sqlite && cp /var/lib/family-hub/family-hub.sqlite ~/family-hub.sqlite.bak || echo no-db'` | Backup path or `no-db` | Cannot SSH |
| E.2 | Sync source to endeavor | See `06-cto-build-gate.md` rsync block | Files under `/opt/family-hub` | rsync fails twice |
| E.3 | Install deps | `ssh endeavor 'cd /opt/family-hub/server && sudo npm install --omit=dev'` | `node_modules` present | npm errors |
| E.4 | Configure `.env` | Edit `/opt/family-hub/server/.env` per `docs/endeavor-deploy.md` | `WRITE_TOKEN=` empty, `DB_PATH=/var/lib/family-hub/family-hub.sqlite` | — |
| E.5 | Init DB | `ssh endeavor 'cd /opt/family-hub/server && sudo npm run init-db && sudo npm run seed'` | Seed skips if members exist | init fails |
| E.6 | Install systemd | `ssh endeavor 'sudo useradd -r -s /usr/sbin/nologin familyhub 2>/dev/null; sudo chown -R familyhub:familyhub /opt/family-hub /var/lib/family-hub; sudo cp /opt/family-hub/deploy/family-hub.service /etc/systemd/system/; sudo systemctl daemon-reload; sudo systemctl enable --now family-hub'` | active (running) | service fails start |
| E.7 | Health check | `curl -s http://endeavor:3020/api/health` | `"ok":true` | not ok |
| E.8 | Smoke test | `BASE_URL=http://endeavor:3020 bash /home/stitch/Desktop/Operating/pi-iot/family-hub/tools/smoke-test.sh` | exit 0 | exit non-zero after 2 fix attempts |
| E.9 | Security checklist | `07-ciso-security-gate.md` deploy safety | Evidence file saved | RED finding |
| E.10 | CISO/validation gate | Run `security-review` + `validation-review` readonly on deploy diff | Pass notes in evidence | Critical auth gap |

### Phase E exit

- [ ] `systemctl is-active family-hub` = active
- [ ] Smoke exit 0
- [ ] Evidence: `E-deploy-*.txt`, `E-smoke-*.txt`, `E-systemctl-*.txt`

### Phase E stop

No SSH to endeavor → log blocker; mark **best-effort complete** for vacation closure. Prior LAN health/smoke PASS at `192.168.1.132:3020`. Manual deploy: [vacation-run-final.md](../verification-evidence/vacation-run-final.md), [endeavor-deploy.md](../endeavor-deploy.md).

---

## Phase F — Hardware Verification

**Status:** **DEFERRED-HARDWARE** (CEO 2026-06-18 — Waveshare ESP32-S3-Touch-LCD-7 not received)

**Entry criteria (resume when unblocked):**

- Waveshare ESP32-S3-Touch-LCD-7 received and powered via USB
- `firmware/include/secrets.h` created from `secrets.example.h` with `WIFI_SSID`, `WIFI_PASSWORD`, `DEFAULT_SERVER_HOST` (endeavor LAN IP)
- PlatformIO installed; Waveshare libs per [waveshare-esp32-s3-touch-lcd-7-reference.md](../waveshare-esp32-s3-touch-lcd-7-reference.md)

**Objective:** Prove firmware connects to real WiFi and server before LVGL investment.

### Steps

| Step | Action | Command | Expected output | Stop if |
|------|--------|---------|-----------------|---------|
| F.1 | Create secrets | `cp firmware/include/secrets.example.h firmware/include/secrets.h` — set `WIFI_SSID`, `WIFI_PASSWORD`, `DEFAULT_SERVER_HOST` (endeavor LAN IP) | File gitignored | — |
| F.2 | Build devkit | `cd firmware && pio run -e devkit` | SUCCESS | build fail |
| F.3 | Flash devkit (optional) | `pio run -e devkit -t upload && pio device monitor` | Serial: fw version + device ID | no USB |
| F.4 | WiFi connect | Monitor serial 60s | WiFi connected, RSSI logged | never connects → fix secrets |
| F.5 | Server fetch | Serial: successful dashboard parse or `[badge]` ONLINE | JSON fields present | server unreachable → fix host |
| F.6 | Write test | Serial key `a` (add grocery) | `[write] OK` | `[write] FAILED` → fix token/host |
| F.7 | WiFi drop test | Disable AP 30s, re-enable | `[badge] WIFI OFFLINE` → reconnect | no reconnect loop |
| F.8 | Build waveshare7 | `pio run -e waveshare7` | SUCCESS (may be serial-only until G) | — |
| F.9 | Flash panel | `pio run -e waveshare7 -t upload` | Boot serial on panel USB | no hardware → WAIVED path |

### Phase F exit

- [ ] Devkit or panel connects WiFi + fetches dashboard from endeavor (or local server)
- [ ] Write event confirmed by server
- [ ] Evidence: `F-boot-*.txt`, `F-wifi-*.txt`

### Phase F stop

No Waveshare hardware → **DEFERRED-HARDWARE (W-HW01)**; V1.0 ships without panel. Do not fake PASS. Resume from F.1 when panel arrives.

---

## Phase G — LVGL Panel UI

**Status:** **DEFERRED-HARDWARE** — blocked on Phase F + physical panel

**Dual-panel order (D-38):** **G-Waveshare** (required for V1-full) → **G-Elecrow** (optional V1.1 bonus). See [panel-targets.md](../panel-targets.md).

### G-Waveshare (primary)

**Entry criteria (resume when unblocked):**

- Phase F exit criteria met (WiFi + dashboard fetch on panel or devkit)
- Waveshare ESP32-S3-Touch-LCD-7 on USB; `secrets.h` WiFi configured
- `ESP32_Display_Panel`, `ESP32_IO_Expander`, `lvgl@8.4.0` installed per [waveshare-esp32-s3-touch-lcd-7-reference.md](../waveshare-esp32-s3-touch-lcd-7-reference.md)

**Objective:** Replace serial-only UI with touch LVGL on Waveshare 7".

| Step | Action | Details | Expected output | Stop if |
|------|--------|---------|-----------------|---------|
| G.1 | Add LVGL + panel libs | Edit `firmware/platformio.ini` `[env:waveshare7]`, follow [waveshare-setup.md](../waveshare-setup.md) | `pio run -e waveshare7` links | lib resolution fails after doc steps |
| G.2 | Display init | Waveshare board init in `ui_manager.cpp` `#ifdef WAVESHARE_7` | Backlight on | hardware smoke fail |
| G.3 | Status badge | Implement `renderStatusBar` LVGL branch | Badge visible on screen | — |
| G.4 | Home screen | `renderHome` — pinned, dinner, counts | Readable text | — |
| G.5 | Nav + screens | Grocery, chores, dinner, notes, settings | Touch nav works | — |
| G.6 | Touch writes | Wire buttons → `submitEvent` types | On-screen OK/FAIL | server rejects |
| G.7 | Settings NVS | Host/port edit persists reboot | New host used | — |
| G.8 | Flash panel | `pio run -e waveshare7 -t upload` | LVGL renders | — |
| G.9 | Compatibility gate | `npx agent-compatibility scan` (if available) | No contract breaks | — |
| G.10 | Photo evidence | Panel photo at ~1m | `G-panel-home-*.jpg` | — |

**G-Waveshare exit:** UI readable on Waveshare display; offline badge on screen; ≥1 write with OK/FAIL feedback.

### G-Elecrow (optional V1.1 bonus)

**Entry criteria:** G-Waveshare exit met **or** CEO waives Waveshare; Elecrow CrowPanel 7" on USB.

- Env: `elecrow7`; flag `ELECROW_7=1`
- LovyanGFX + GT911 + PCA9557 per [elecrow-esp32-display-7-reference.md](../elecrow-esp32-display-7-reference.md)
- `#elif defined(ELECROW_7)` hooks in `ui_manager.cpp` — same ScreenId contract as Waveshare

**Objective:** Same LVGL UX on Elecrow hardware. **Do not block or complicate Waveshare path.**

| Step | Action | Stop if |
|------|--------|---------|
| G-E.1 | Install Elecrow board JSON + LovyanGFX stack | — |
| G-E.2 | `pio run -e elecrow7` links | touch/V3.0 issues → doc troubleshooting, not waveshare7 changes |
| G-E.3 | LVGL screens mirror G-Waveshare | — |
| G-E.4 | Photo evidence | `G-elecrow-panel-home-*.jpg` |

### Phase G exit (V1-full)

- [ ] **G-Waveshare** complete (required)
- [ ] UI readable at 7" distance **on physical Waveshare display**
- [ ] Offline badge on display (not serial-only)
- [ ] At least one write shown on screen with OK/FAIL feedback
- [ ] G-Elecrow: optional; not required for V1-full

### Phase G stop

Waveshare libs unavailable or no Waveshare hardware → **DEFERRED-HARDWARE**; document W-HW01. Elecrow-only owners may attempt G-Elecrow but V1-full still expects Waveshare unless CEO waives. Do not fake PASS.

---

## Phase H — V1 Gap Fill

**Objective:** Close remaining checklist items not covered by automation.

### Steps

| Step | Action | Expected | Stop if |
|------|--------|----------|---------|
| H.1 | DB restart test | `ssh endeavor 'sudo systemctl restart family-hub'` → data intact | Members/grocery still present | data loss → restore backup |
| H.2 | Browser on phone | Open `http://endeavor:3020` on LAN phone | Page loads | — |
| H.3 | Browser CRUD | Add grocery, toggle chore, set dinner, add note | Each succeeds | UI bug → fix minimal |
| H.4 | Browser offline | Stop service, reload page | Clear error message | silent fail → fix |
| H.5 | Panel end-to-end | Full panel flows against endeavor | Matches checklist | — |
| H.6 | Update checklist | Mark rows in `v1-verification-checklist.md` | All PASS or WAIVED | — |

### Phase H exit

All server/browser/panel manual rows addressed.

---

## Phase I — V1 Sign-Off

**Objective:** Formal release closure.

### Steps

| Step | Action | Expected |
|------|--------|----------|
| I.1 | Run full automated suite | `cd server && npm test`; `bash tools/smoke-test.sh`; `pio run -e devkit && pio run -e waveshare7` | All green |
| I.2 | Complete `v1-verification-checklist.md` | 100% PASS or WAIVED with waiver IDs |
| I.3 | Bump firmware version | `FIRMWARE_VERSION=\"1.0.0\"` in both envs if G passed |
| I.4 | Fill board log | `09-board-decision-log.md` Phase I template |
| I.5 | Evidence index | Update `docs/verification-evidence/README.md` |
| I.6 | Optional git tag | `git tag v1.0.0` + commit message per CEO brief |

### Phase I exit

Board log status: **SHIPPED** or **BLOCKED** with explicit CEO follow-ups.

---

## Global Stop Conditions

From `docs/phase-repair-plan.md` — still apply. Additionally:

- Declaring V1.0 with panel WAIVED is **approved** under W-HW01 (CEO 2026-06-18).
- V1-full still requires Phase G PASS or CEO review on return.

---

## Quick Command Reference

```bash
# Dev baseline
cd /home/stitch/Desktop/Operating/pi-iot/family-hub/server && npm install && npm test
cd /home/stitch/Desktop/Operating/pi-iot/family-hub && bash tools/smoke-test.sh
cd /home/stitch/Desktop/Operating/pi-iot/family-hub/firmware && pio run -e devkit && pio run -e waveshare7

# Production
curl -s http://endeavor:3020/api/health | jq .
BASE_URL=http://endeavor:3020 bash tools/smoke-test.sh
```
