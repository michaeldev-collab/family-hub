# V1 Verification Checklist

**Supersedes:** `docs/verification-checklist.md` for V1 sign-off (v0.1 checklist remains historical reference).

**Evidence directory:** `docs/verification-evidence/`  
**Mark each row:** PASS | FAIL | WAIVED (waiver ID from `v1-risks-and-waivers.md`)

---

## Automated Baseline (run before Phase I)

| # | Check | Command | Pass criteria | Status |
|---|-------|---------|---------------|--------|
| A1 | Server tests | `cd server && npm install && npm test` | All tests pass | PASS (20260619, 9/9) |
| A2 | Smoke (local) | `bash tools/smoke-test.sh` | Exit 0 | PASS (20260619) |
| A3 | Firmware devkit build | `cd firmware && pio run -e devkit` | SUCCESS | PASS (20260619) |
| A4 | Firmware waveshare build | `pio run -e waveshare7` | SUCCESS | PASS (20260619) |

---

## Server — endeavor Production

| # | Check | Procedure | Pass criteria | Status |
|---|-------|-----------|---------------|--------|
| S1 | Health | `curl http://192.168.1.132:3020/api/health` | `ok: true` | PASS (20260619, dev npm start on dev-pc) |
| S2 | Dashboard state | LAN smoke / curl | today, members, grocery, chores, dinner | PASS (20260619 LAN smoke) |
| S3 | Grocery CRUD | smoke / curl | Create, read, delete work | PASS (20260619 LAN smoke) |
| S4 | Chore toggle | smoke | Complete + uncomplete | PASS (20260619 LAN smoke) |
| S5 | Dinner set | smoke | PUT today works | PASS (20260619 LAN smoke) |
| S6 | Idempotent events | smoke / test | Duplicate `eventId` deduped | PASS (20260619 LAN smoke) |
| S7 | endeavor smoke | `BASE_URL=http://192.168.1.132:3020 bash tools/smoke-test.sh` | Exit 0 | PASS (E-smoke-lan-20260619) |
| S8 | Data survives restart | `systemctl restart family-hub` → re-query members/grocery | Data unchanged | [ ] — systemd inactive (20260619) |
| S9 | systemd active | `systemctl is-active family-hub` | `active` | [ ] — inactive (20260619) |
| S10 | WRITE_TOKEN policy | `.env` has empty token; writes work without header | LAN trust mode | PASS (LAN smoke writes) |

---

## Browser Admin (LAN phone or desktop)

| # | Check | Procedure | Pass criteria | Status |
|---|-------|-----------|---------------|--------|
| B1 | Home loads | smoke `web index` @ 192.168.1.132:3020 | UI renders | PASS (LAN smoke) |
| B2 | Add grocery | UI add flow | Item appears | [ ] |
| B3 | Toggle chore | UI toggle | State changes | [ ] |
| B4 | Set dinner | UI set tonight | Dinner updates | [ ] |
| B5 | Add note | UI add note | Note appears | [ ] |
| B6 | Server offline UX | Stop service, reload | Clear error (not fake success) | [ ] |
| B7 | WRITE_TOKEN UI | Settings token field documented | Blank OK for V1 | [ ] |

---

## ESP32 Panel — Thin Client (Waveshare Target)

| # | Check | Procedure | Pass criteria | Status |
|---|-------|-----------|---------------|--------|
| P1 | Boots cleanly | Serial or on-screen | FW version + device ID shown | **WAIVED — W-HW01** ([docs](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7)) |
| P2 | WiFi connects | Boot on panel | Connected, RSSI logged | **WAIVED — W-HW01** |
| P3 | Fetches dashboard | Panel polls endeavor | Data matches server | **WAIVED — W-HW01** |
| P4 | Offline badge WiFi | Drop AP 30s | **On-screen** WIFI OFFLINE | **WAIVED — W-HW01** |
| P5 | Server offline | Stop endeavor | **On-screen** SERVER OFFLINE + cache | **WAIVED — W-HW01** |
| P6 | Failed write | POST with server down | **On-screen** FAILED, not success | **WAIVED — W-HW01** |
| P7 | Reconnect backoff | Monitor logs during outage | No request storm | **WAIVED — W-HW01** |
| P8 | Duplicate eventId | Repeat same event | Server dedupes | **WAIVED — W-HW01** |
| P9 | Cached snapshot | Server down | Last good data visible | **WAIVED — W-HW01** |
| P10 | LVGL readable | View panel at ~1m | Home screen legible | **WAIVED — W-HW01** |
| P11 | Touch grocery add | Panel UI | Server confirms, UI OK | **WAIVED — W-HW01** |
| P12 | Touch chore complete | Panel UI | Server confirms | **WAIVED — W-HW01** |
| P13 | Touch dinner set | Panel UI | Server confirms | **WAIVED — W-HW01** |
| P14 | Touch note add | Panel UI | Server confirms | **WAIVED — W-HW01** |
| P15 | Settings screen | Panel UI | Host, port, device ID, fw, RSSI, last sync | **WAIVED — W-HW01** |
| P16 | NVS host change | Change host in settings, reboot | Uses new host | **WAIVED — W-HW01** |

---

## Observability

| # | Check | Pass criteria | Status |
|---|-------|---------------|--------|
| O1 | Server write logs | `write_log` / request logger entries | PASS (local smoke + tests) |
| O2 | Panel status fields | All settings fields populated | **WAIVED — W-HW01** |
| O3 | Health uptime | `/api/health` reports uptime + DB | PASS (local) |

---

## Safety

| # | Check | Pass criteria | Status |
|---|-------|---------------|--------|
| F1 | No high-risk controls | Code review grep | PASS |
| F2 | No public exposure | No WAN port forward documented | PASS |
| F3 | No secrets in git | `git log -p` / check-ignore for secrets.h | PASS |
| F4 | Architecture compliance | Panel does not own canonical state | PASS |

---

## V1 Sign-Off Summary

| Section | Pass | Fail | Waived |
|---------|------|------|--------|
| Automated | 4/4 | | 0 |
| Server | 8/10 | | |
| Browser | 1/7 | | |
| Panel | 0/16 pass | | **16/16 WAIVED (W-HW01)** |
| Observability | 2/3 | | 1 |
| Safety | 4/4 | | |
| **Total** | 19/44 pass | | **17 waived** |

**V1.0 SHIPPED criteria (no panel):** Server S1–S7 + browser B1–B6 PASS or waived; panel P1–P16 waived under **W-HW01**.  
**V1-full SHIPPED criteria:** Panel P10–P16 PASS when hardware arrives (Phase G).

**Signed by:** Agent validation reviewer  
**Date:** 2026-06-19 (re-run)  
**Evidence file:** `docs/verification-evidence/vacation-run-final.md` (§ 2026-06-19)

---

## Regression Commands

```bash
cd /home/stitch/Desktop/Operating/pi-iot/family-hub/server && npm test
cd /home/stitch/Desktop/Operating/pi-iot/family-hub && bash tools/smoke-test.sh
cd /home/stitch/Desktop/Operating/pi-iot/family-hub/firmware && pio run -e devkit && pio run -e waveshare7
curl -s http://endeavor:3020/api/health
BASE_URL=http://endeavor:3020 bash tools/smoke-test.sh
```
