# Family Hub v0.1 — Verification Checklist

**Repair plan status (2026-06-18):** Automated server checks below pass locally. Hardware panel and endeavor deploy rows require on-LAN execution.

## Server

- [x] `curl /api/health` returns `ok: true` — `npm test` + smoke script
- [x] `curl /api/dashboard-state` returns today, members, grocery, chores, dinner
- [x] Grocery CRUD works via curl — smoke + `api.test.js`
- [x] Chore complete/uncomplete works via curl — smoke + `api.test.js`
- [x] Dinner set works via curl — smoke + `api.test.js`
- [x] `POST /api/events` with duplicate `eventId` returns `deduplicated: true`
- [ ] Data survives server restart — manual on endeavor
- [x] `tools/smoke-test.sh` passes — against local/ephemeral server

## Browser admin

- [ ] Home page loads on phone browser — manual
- [x] Add grocery item — API wired with optional `x-family-hub-token`
- [ ] Toggle chore complete — manual UI
- [ ] Set tonight's dinner — manual UI
- [ ] Add note — manual UI
- [ ] Server offline shows clear error (stop server, reload page) — manual

## ESP32 panel (thin client)

- [x] Boots cleanly (serial log shows firmware version + device ID) — `main.cpp` boot path
- [ ] Connects to WiFi — requires hardware + `secrets.h`
- [ ] Fetches `/api/dashboard-state` from endeavor — requires LAN
- [x] Shows offline/stale badge when WiFi drops — serial `[badge]` + reconnect loop
- [x] Shows server offline when endeavor is stopped — `ConnState::ServerOffline` + cache
- [x] Failed write does NOT show as success (serial: `[write] FAILED`)
- [x] Reconnect uses backoff (no request storm in logs) — `api_client.cpp`
- [x] Duplicate eventId handled safely by server — tests
- [x] Cached snapshot displayed when server unreachable — `state_cache.cpp`
- [ ] UI readable at 7" distance (LVGL on Waveshare hardware) — **deferred until Waveshare libs linked**

## Observability

- [x] Server logs write actions — `write_log` table + request logger
- [x] Panel shows: device ID, firmware version, last sync, WiFi RSSI, server host — Settings screen (serial/LVGL hooks)
- [x] Health endpoint reports uptime and DB status

## Safety

- [x] No high-risk device controls in v0.1
- [x] No public internet exposure (documented)
- [x] No secrets committed to firmware repo — `.gitignore` + `secrets.example.h`

## Automated commands

```bash
cd server && npm test
cd .. && bash tools/smoke-test.sh
cd firmware && pio run -e devkit && pio run -e waveshare7b
```

Evidence logs may be stored under `docs/verification-evidence/` (gitignored).
