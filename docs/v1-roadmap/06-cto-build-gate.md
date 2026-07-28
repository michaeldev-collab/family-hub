# CTO Build Gate — Technical Phases v0.1 → V1.0

**Gate ID:** CTO-01  
**Status:** GREEN  
**Date:** 2026-06-18

---

## Version Ladder

| Version | Technical meaning | Exit signal |
|---------|-------------------|-------------|
| **v0.1** | Code complete server + firmware thin-client logic (serial UI) | Repair plan Phases A–B done |
| **v0.9** | endeavor deployed + panel connects (may be serial UI) | Phase E + F complete |
| **v1.0** | LVGL daily-usable panel + full verification | Phase I sign-off |

Bump `FIRMWARE_VERSION` to `1.0.0` in `firmware/platformio.ini` at Phase I only.

---

## Workstreams

### WS-SERVER (complete — maintain)

| Path | State |
|------|-------|
| `server/app.js`, routes, services | Stable |
| `server/tests/api.test.js` | Expanded |
| `tools/smoke-test.sh` | Expanded |
| `web/js/api.js` | WRITE_TOKEN wired |
| `deploy/family-hub.service` | EnvironmentFile aligned |

**Phase E action:** Deploy, don't refactor.

### WS-WEB (Phase H)

Manual browser verification; fix bugs only if checklist fails.

### WS-FIRMWARE (Phase F + G — primary remaining work)

| Component | File(s) | v0.1 state | V1 target |
|-----------|---------|------------|-----------|
| API client | `api_client.cpp` | Done — backoff, token, events | Verify on hardware |
| State cache | `state_cache.cpp` | Done | Verify on hardware |
| WiFi reconnect | `main.cpp` | Done | Verify on hardware |
| UI | `ui_manager.cpp` | Serial + `#ifdef WAVESHARE_7` stubs | **Full LVGL** |
| Config | `platformio.ini` | `waveshare7` env without LVGL libs | Add libs per waveshare-setup |

### WS-DEPLOY (Phase E)

Follow `docs/endeavor-deploy.md` exactly.

---

## Phase G — LVGL Implementation Spec

**Environment:** `waveshare7`

**Dependencies to add** (uncomment/configure in `platformio.ini`):

```ini
lib_deps =
    bblanchon/ArduinoJson@^7.2.1
    lvgl/lvgl@8.4.0
; Plus Waveshare ESP32_Display_Panel per docs/waveshare-setup.md
```

**Screens (minimal, embedded constraints):**

| Screen | Content | Actions |
|--------|---------|---------|
| Home | Pinned notes, dinner today, open chores count, grocery count, status badge | Nav buttons |
| Grocery | List scroll | Tap to add (prompt/simple keyboard or preset items) |
| Chores | Open chores | Tap to complete first / selected |
| Dinner | Today + 3-day week | Set tonight button |
| Notes | List + add | Add note |
| Settings | Host, port, device ID, fw, RSSI, last sync | Edit host/port → NVS |

**Mandatory UI rules:**

- Status badge top-right: WIFI OFFLINE / SERVER OFFLINE / STALE DATA / ONLINE
- Write toast or banner 8s: OK / FAILED (mirror serial behavior)
- No dense tables; font size readable at 1m

**Event types** (must match `server/services/events.js`):

- `grocery.add`
- `chore.complete`
- `dinner.set`
- `note.add`

---

## PlatformIO Commands

```bash
cd /home/stitch/Desktop/Operating/pi-iot/family-hub/firmware
pio run -e devkit          # CI / serial testing
pio run -e waveshare7      # Production panel build
pio run -e waveshare7 -t upload
pio device monitor -b 115200
```

---

## endeavor Deploy Commands

```bash
# From dev machine with SSH
rsync -av --exclude node_modules --exclude data \
  /home/stitch/Desktop/Operating/pi-iot/family-hub/ \
  endeavor:/tmp/family-hub-src/

ssh endeavor 'sudo mkdir -p /opt/family-hub /var/lib/family-hub && \
  sudo cp -r /tmp/family-hub-src/server /tmp/family-hub-src/web /tmp/family-hub-src/deploy /tmp/family-hub-src/tools /opt/family-hub/ && \
  cd /opt/family-hub/server && sudo npm install --omit=dev'

# Configure .env, init-db, seed, systemd — see endeavor-deploy.md
```

---

## Technical Stop Conditions (CTO)

From 3DL OS Build + Architecture Book:

1. ESP32 must not own canonical state — SPIFFS cache only
2. No secrets committed to git
3. No internet-exposed API
4. No HA/MQTT config changes under V1
5. LVGL blocked → stop Phase G; do not declare V1 without waiver

---

## Quality Gates

| When | Gate | Pass |
|------|------|------|
| After E | `validation-review` | endeavor smoke green |
| After G | `compatibility-scan-review` | dashboard-state + events contract |
| After G | `security-review` | no new auth exposure |
| Before I | `validation-review` | full v1 checklist |

---

## Exit Criteria (CTO Gate)

- [x] Technical phases mapped v0.1 → v1.0
- [x] LVGL spec actionable without Figma
- [x] Build/deploy commands exact

**Gate:** GREEN
