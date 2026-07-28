# CPO Product Gate — V1 Definition

**Gate ID:** CPO-01  
**Status:** GREEN  
**Date:** 2026-06-18

---

## V1 User Outcomes

Family Hub V1 lets the household **manage daily family ops from the LAN** with:

1. **endeavor server** always available on the home network (groceries, chores, dinner, notes, members).
2. **Phone/browser admin** for CRUD when near the server.
3. **Waveshare kitchen panel** (V1-full only) as a glanceable thin client: read dashboard state, perform low-risk writes, clear offline/stale feedback.
4. **Trustworthy writes** — success only when server confirms; cached read when server down.

**Primary user:** Michael's household (internal family product).

---

## In Scope (V1)

| Capability | Server | Browser | Panel |
|------------|--------|---------|-------|
| Grocery list CRUD | ✓ | ✓ | Add via events |
| Chores complete/uncomplete | ✓ | ✓ | Complete via events |
| Dinner (today + week read) | ✓ | ✓ | Set tonight via events |
| Notes (incl. pinned on home) | ✓ | ✓ | Add via events |
| Members CRUD | ✓ API | ✓ basic UI | Read-only display |
| Dashboard aggregate | ✓ | ✓ | ✓ primary view |
| Offline/stale UX | N/A | Error on fetch fail | Badge + cache |
| Optional WRITE_TOKEN | ✓ | ✓ | ✓ NVS/build flag |
| endeavor systemd deploy | ✓ | — | — |
| LVGL touch UI on 7" panel | — | — | ✓ **V1-full only** (deferred — W-HW01) |

---

## Out of Scope (V1 — defer to v1.1+)

| Item | Rationale |
|------|-----------|
| Calendar / `events` API & UI | CEO deferred; schema only |
| MQTT / Home Assistant / ESPHome | Separate domain per Architecture Book |
| TLS / public internet | LAN-only v1 |
| High-risk device control | Safety boundaries |
| Week-dinner planner UI polish | v0.2+ |
| Member management UX polish | API sufficient for v1 |
| CORS / rate limiting automation | Document only |
| Native mobile apps | Browser sufficient |
| Multi-home / multi-tenant | Single household |

---

## v0.1 vs V1 Boundary

| Label | Meaning |
|-------|---------|
| **v0.1 (code baseline)** | Feature-complete server + serial/stub panel firmware |
| **v0.1 repair** | Safety, auth, tests, contract honesty — **mostly done** |
| **V1.0 (release, no panel)** | v0.1 repair **complete** + endeavor production + browser admin + verification signed off (panel waived) |
| **V1-full / V1.1 panel** | V1.0 + **usable Waveshare LVGL panel** (Phase F–G when hardware arrives) |

V1 is **not** a feature expansion release. It is **operational readiness** of the v0.1 product on real infrastructure. **Michael confirmed panel not received (2026-06-18)** — V1.0 ships without ESP32; panel is V1-full.

See [vacation-mode-complete.md](vacation-mode-complete.md).

---

## Acceptance Criteria — "V1.0 Done" (without panel)

All must pass or be **WAIVED** with documented mitigation in `v1-risks-and-waivers.md`:

**Panel rows (P1–P16):** WAIVED under **W-HW01** until Waveshare ESP32-S3-Touch-LCD-7 received. V1.0 does **not** require panel PASS.

## Acceptance Criteria — "V1-full Done" (with panel)

When hardware arrives, Phase F–G must pass panel section (or explicit waiver with CEO review):

### Server (endeavor)

- [ ] `curl http://endeavor:3020/api/health` → `ok: true`
- [ ] `BASE_URL=http://endeavor:3020 bash tools/smoke-test.sh` → exit 0
- [ ] SQLite persists across `systemctl restart family-hub`
- [ ] `npm test` passes on dev machine (regression guard)

### Browser

- [ ] Home loads on phone browser on LAN
- [ ] Add grocery, toggle chore, set dinner, add note — each succeeds
- [ ] Server stopped → page shows clear error (not silent success)

### Panel (Waveshare) — **V1-full only; WAIVED for V1.0 (W-HW01)**

- [ ] ~~Boots, connects WiFi, fetches `/api/dashboard-state` from endeavor~~ → deferred
- [ ] ~~LVGL UI readable at ~7" viewing distance (not serial-only)~~ → deferred
- [ ] ~~Offline/stale badge visible on display when WiFi or server lost~~ → deferred
- [ ] ~~Write actions show on-screen success/failure matching server~~ → deferred
- [ ] ~~Settings: device ID, fw version, last sync, RSSI, server host visible on screen~~ → deferred
- [ ] ~~WiFi reconnect after AP drop without request storm~~ → deferred

Docs: https://docs.waveshare.com/ESP32-S3-Touch-LCD-7 — prep: [waveshare-esp32-s3-touch-lcd-7-reference.md](../waveshare-esp32-s3-touch-lcd-7-reference.md)

### Safety & ops

- [ ] No high-risk controls in codebase
- [ ] No secrets in git
- [ ] Evidence bundle in `docs/verification-evidence/`

---

## Product Metrics (Internal)

V1 success is **binary checklist**, not analytics. Optional observation:

- Panel used for one real grocery add + chore complete during Phase I demo test.

---

## Exit Criteria (CPO Gate)

- [x] V1 scope bounded and realistic
- [x] v1.1 backlog explicit
- [x] Acceptance criteria testable without CEO judgment calls

**Gate:** GREEN
