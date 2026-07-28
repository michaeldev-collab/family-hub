# Plan: Family Hub v0.1 Repairs

**Status:** Executed — see [docs/v1-roadmap/](v1-roadmap/) for V1 vacation execution (Phases E–I).  
**Prerequisite:** [Architecture Book](architecture-book.md) and [Safety Boundaries](safety-boundaries.md) are the source-of-truth for scope.  
**Audit reference:** Architecture & Safety Audit (`eb4e34ea-cca8-4522-8cc7-b2da0a85d526`).

---

## Goal

Restore Family Hub v0.1 to a **production-safe, verifiable baseline** on endeavor (server + browser) and a **usable Waveshare thin-client panel** (ESP32 + LVGL), without scope creep into MQTT, Home Assistant, or high-risk device control.

**Success is measured when:**

1. The repo is git-managed with secrets and artifacts excluded.
2. `npm test` and `tools/smoke-test.sh` pass reliably (no PORT=0 collision).
3. Optional LAN write auth (`WRITE_TOKEN`) works end-to-end or is explicitly documented as disabled.
4. The ESP32 panel boots, reconnects after Wi-Fi loss, shows on-screen offline/stale state, and performs v0.1 writes only after server confirmation.
5. Full [verification-checklist.md](verification-checklist.md) is executed with evidence captured.

---

## Current State

| Area | Evidence | Risk |
|------|----------|------|
| Version control | No `.git` at repo root; no root `.gitignore` | Work loss; accidental commit of `node_modules/`, SQLite, secrets |
| Firmware secrets | `secrets.h` documented but never `#include`d; build falls back to `secrets.example.h` placeholders (`firmware/src/main.cpp` L10–12) | Panel cannot connect to real WiFi |
| Panel UI | LVGL hooks are stubs; serial-only output (`firmware/src/ui_manager.cpp` L78–81) | 7" display unusable for daily use |
| Server tests | `PORT=0` coerced to `3020` (`server/config/env.js` L38) | `EADDRINUSE` when dev server running |
| Write auth | `optionalWriteAuth` no-op when token empty; clients never send `x-family-hub-token` | Unauthenticated LAN writes; token enable breaks clients |
| `ADMIN_PIN` | Defined in `server/middleware/auth.js` L14–20; never mounted on routes | Misleading deploy docs |
| WiFi reconnect | `main.cpp` sets disconnected state but does not re-call `connectWiFi()` | Partial offline behavior |
| Dashboard contract | `connection.status` always `"online"` (`server/services/dashboardState.js` L52–55) | Misleading offline UX |
| Calendar `events` table | Schema only (`server/db/schema.sql` L53–62); no API/UI | Schema drift |
| Deploy | systemd unit lacks `EnvironmentFile=`; `.env` instructions in `endeavor-deploy.md` | Production secrets/config mismatch |

**Architecture intent is correct:** server SQLite owns family-ops data; ESP32 caches SPIFFS display state only; no MQTT/HA/high-risk controls in code.

---

## Next Target (v0.1 repair complete)

- **Server + browser:** Safe repo baseline, passing automated tests, aligned deploy docs, honest `dashboard-state` contract, expanded smoke coverage.
- **ESP32 panel:** Buildable secrets workflow, LVGL UI on Waveshare hardware, v0.1 event surface (grocery, chore complete, dinner set, note add), Wi-Fi reconnect, on-screen observability.
- **Deploy:** endeavor systemd + `.env` aligned; first-boot seed documented; verification checklist executed on hardware.

---

## Out of Scope (this plan)

- MQTT, Home Assistant, ESPHome integration
- Calendar/events API (unless Phase B decision is **drop table** — see decision gate)
- TLS / public internet exposure
- High-risk device control (locks, HVAC, cameras, etc.)
- Web UI scope reduction (week-dinner planner, member CRUD) — defer to v0.2 unless explicitly requested
- CORS, rate limiting, firewall automation (document recommendations only in Phase D)

---

## Workstreams

| ID | Workstream | Scope | Owner agent | Expected output |
|----|------------|-------|-------------|-----------------|
| WS-1 | **Repo & secrets baseline** | Git init, root `.gitignore`, firmware `secrets.h` inclusion fix | `generalPurpose` (implementation) | Clean git repo; firmware builds with real `secrets.h` |
| WS-2 | **Auth & deploy safety** | `WRITE_TOKEN` client wiring, `ADMIN_PIN` wire-or-remove, systemd `.env` alignment | `generalPurpose` + `security-review` | Consistent auth story; deploy doc matches unit file |
| WS-3 | **Server contract & tests** | PORT fix, test expansion, smoke test, `connection` field, calendar decision, idempotency TTL | `generalPurpose` + `validation-review` | Green `npm test`; smoke matches checklist |
| WS-4 | **Firmware thin client** | LVGL UI, event handlers, Wi-Fi reconnect, observability, settings NVS UI | `generalPurpose` + `compatibility-scan-review` | Flashable `waveshare7` env; panel passes hardware checklist |
| WS-5 | **Deploy & verification** | endeavor deploy, seed policy, full checklist run, evidence capture | `generalPurpose` + `validation-review` | Signed-off verification log |

---

## Parallelization

### Parallel group 1 (read-only, after plan approval)

- **explore** agent: Confirm file inventory, grep for `secrets.h`, auth headers, LVGL stubs — return path/line evidence only.
- **security-review** agent (read-only pre-pass): Review `server/middleware/auth.js`, deploy docs, firmware secret handling — return risk list before WS-2 edits.

*Safe because read-only; informs Phase A auth decisions.*

### Parallel group 2 (after Phase A git + PORT fix land)

- **WS-3** server tests + smoke expansion (server/, tools/)
- **WS-2** auth client wiring in `web/js/api.js` (no firmware overlap)

*Safe: disjoint file trees.*

### Sequential dependencies

| Must finish first | Before | Reason |
|-------------------|--------|--------|
| Phase A: git + `secrets.h` + PORT fix | Phase B tests / Phase C firmware builds | Tests need ephemeral port; firmware needs compilable secrets |
| Phase A: `WRITE_TOKEN` policy decision | WS-2 client wiring + Phase D deploy | Clients must match server config |
| Phase B: `dashboard-state` contract finalized | Phase C panel UI consuming `pinnedNotes` / `connection` | Avoid rework on LVGL screens |
| Phase B: calendar table decision | Schema migration (if any) | Single migration window |
| Phase C: LVGL + events complete | Phase D hardware verification | Checklist requires on-screen UI |

### Serialize (do not parallelize)

- **LVGL implementation** — single agent owns `firmware/src/ui_manager.cpp` and related hooks.
- **Schema changes** (calendar drop or API add) — one migration, one PR slice.

---

## Agent Assignments

### explore (read-only, Phase 0)

- **Task:** Re-validate audit findings against current tree; list tracked/untracked files after git init dry-run.
- **Inspect:** repo root, `server/`, `firmware/`, `deploy/`, `docs/`.
- **Return:** Markdown table of findings with file paths; flag any new drift from audit.

### generalPurpose — Phase A (WS-1, WS-2 partial)

- **Task:** Initialize git; add root `.gitignore`; fix `secrets.h` include strategy; fix `PORT=0` parsing in `server/config/env.js`.
- **Constraints:** Do not commit `node_modules/`, `*.sqlite*`, `.env`, `secrets.h`, `.pio/`.
- **Return:** `git status` clean except intentional files; `npm test` passes with no server on 3020.

### security-review (gate after Phase A auth edits)

- **Task:** Review write-auth wiring, secrets handling, systemd env, LAN bind address.
- **Inspect:** `server/middleware/auth.js`, `server/config/env.js`, `web/js/api.js`, `firmware/src/api_client.cpp`, `deploy/family-hub.service`, `docs/endeavor-deploy.md`.
- **Return:** Pass/fail per item: unauthenticated writes, token header presence, no secrets in git, no hardcoded production creds in firmware repo.

### generalPurpose — Phase B (WS-3)

- **Task:** Expand `server/tests/api.test.js`; extend `tools/smoke-test.sh`; fix `dashboard-state.connection`; resolve calendar table; add idempotency retention.
- **Constraints:** No breaking changes to existing event types; document API shape changes in `docs/architecture-book.md` if needed.
- **Return:** Test file list, new test count, smoke script diff summary.

### compatibility-scan-review (gate after Phase B)

- **Task:** Run agent-compatibility CLI; verify firmware/server contract documented in plan matches code.
- **Inspect:** `GET /api/dashboard-state` shape, `POST /api/events` types, client headers.
- **Return:** Compatibility score + contract gap list (if any).

### generalPurpose — Phase C (WS-4)

- **Task:** Implement LVGL under `#ifdef WAVESHARE_7`; touch handlers for v0.1 events; Wi-Fi reconnect in `main.cpp`; on-screen status badge; settings screen for NVS host/port; `WRITE_TOKEN` header in `api_client.cpp`.
- **Constraints:** Panel remains thin client — no canonical state in SPIFFS beyond cache; respect `docs/waveshare-setup.md` and embedded UI constraints from 3D Design Labs Build.
- **Return:** PlatformIO build log for `waveshare7`; serial boot log sample; list of implemented event types.

### validation-review (gate after Phase B and after Phase D)

- **Task:** Confirm verification commands are runnable without guessing; tests are not flaky.
- **Inspect:** `server/tests/`, `tools/smoke-test.sh`, `docs/verification-checklist.md`.
- **Return:** Pass/fail per checklist section with commands run and output snippets.

### code-reviewer (optional, end of each phase)

- **Task:** Review diff against this plan; flag scope creep.
- **Return:** Approve or list blockers.

---

## Implementation Steps

### Phase 0 — Plan approval (human gate)

1. **Owner:** Michael  
2. **Action:** Review this plan against [architecture-book.md](architecture-book.md) and [safety-boundaries.md](safety-boundaries.md).  
3. **Decision required:** `WRITE_TOKEN` policy — (a) wire through all clients and require on endeavor, or (b) keep empty for v0.1 LAN-only and remove misleading deploy text.  
4. **Decision required:** Calendar `events` table — (a) drop from schema v0.1, or (b) defer API to v0.2 and document as unused.  
5. **Exit:** Explicit approval: *"Approve this plan for execution, request changes, or mark sections out of scope."*

---

### Phase A — Repo hygiene & safety baseline (Stabilize/Fix)

**Objective:** Safe version control, compilable firmware secrets, reliable test port, auth story unambiguous.

| Step | Owner | Action | Expected result |
|------|-------|--------|-----------------|
| A.1 | generalPurpose | `git init` at repo root; add root `.gitignore` aggregating `server/.gitignore`, `firmware/.gitignore`, OS/IDE junk | `git status` excludes `node_modules/`, `server/data/`, `.env`, `secrets.h`, `.pio/` |
| A.2 | generalPurpose | Initial commit of source only (no artifacts) | Clean baseline commit |
| A.3 | generalPurpose | Fix firmware secrets: `#include "secrets.h"` when present, else `secrets.example.h`; update `docs/waveshare-setup.md` if build flags change | `pio run -e devkit` succeeds with user `secrets.h` |
| A.4 | generalPurpose | Fix `server/config/env.js` PORT parsing: treat `0` as valid ephemeral port (`PORT=0` for tests) | `npm test` binds random port, no `EADDRINUSE` on 3020 |
| A.5 | generalPurpose + Michael | Implement chosen `WRITE_TOKEN` policy: wire `x-family-hub-token` in `web/js/api.js` and `firmware/src/api_client.cpp` **or** strip optional-token language from deploy docs and add README note | Clients and docs agree |
| A.6 | generalPurpose | `ADMIN_PIN`: mount `validatePinIfRequired` on agreed routes (e.g. member delete) **or** remove from `.env.example` and `endeavor-deploy.md` | No dead config |
| A.7 | generalPurpose | Align `deploy/family-hub.service` with `docs/endeavor-deploy.md`: add `EnvironmentFile=/opt/family-hub/server/.env` or document inline-only vars | Production env matches docs |
| A.8 | security-review | Gate — auth/secrets/deploy review | Written pass or blocker list |

**Phase A stop conditions (halt and re-plan):**

- User chooses to store long-lived secrets in firmware without secure provisioning approach.
- Plan would expose API beyond LAN without explicit approval and security plan.
- Git init would commit existing SQLite with real family data — **stop**, export/sanitize first.

---

### Phase B — Server contract & tests (Fix/Build/Verify)

**Objective:** Honest API contract, automated coverage matching checklist server section.

| Step | Owner | Action | Expected result |
|------|-------|--------|-----------------|
| B.1 | generalPurpose | Expand `server/tests/api.test.js`: chores complete/uncomplete, dinner set, notes CRUD, members, auth rejection when `WRITE_TOKEN` set, idempotent events for second type | ≥8 tests, all green |
| B.2 | generalPurpose | Extend `tools/smoke-test.sh`: chore toggle, dinner set, optional auth header when token configured | Smoke matches checklist server rows |
| B.3 | generalPurpose | Fix `dashboard-state.connection`: remove field or derive honest status (e.g. `generatedAt` + server health only; document that clients infer offline locally) | No false `"online"` semantics |
| B.4 | generalPurpose | Calendar table per Phase 0 decision: migration to drop, or comment in schema + architecture book "deferred v0.2" | No silent schema drift |
| B.5 | generalPurpose | Idempotency key retention: TTL cleanup job or prune on insert (e.g. >30 days) | Table bounded |
| B.6 | compatibility-scan-review | Gate — firmware/server contract | Score + gaps documented |
| B.7 | validation-review | Gate — tests credible, not flaky | Pass with command log |

**Phase B stop conditions:**

- API breaking changes would brick panel without Phase C coordinated release — **serialize** B.3 with C contract review.
- Test suite requires production DB path — use isolated test DB only.

---

### Phase C — Firmware thin client completion (Build/Integrate)

**Objective:** Waveshare 7" panel as architecture-compliant thin client.

| Step | Owner | Action | Expected result |
|------|-------|--------|-----------------|
| C.1 | generalPurpose | Link Waveshare + LVGL per `docs/waveshare-setup.md` and `platformio.ini` `waveshare7` env | `pio run -e waveshare7` succeeds |
| C.2 | generalPurpose | Implement LVGL screens: home (pinned notes, dinner today, chores summary), grocery, chores, dinner, notes, settings | Readable at 7"; large tap targets |
| C.3 | generalPurpose | On-screen offline/stale badge driven by `ConnState` / `staleData` | Visible without serial |
| C.4 | generalPurpose | Touch write paths via `POST /api/events`: `grocery.add`, `chore.complete`, `dinner.set`, `note.add` (match `server/services/events.js` types) | Failed writes show failure on screen |
| C.5 | generalPurpose | Wi-Fi reconnect loop in `main.cpp` when `WL_DISCONNECTED`; exponential backoff on HTTP; gate `postEvent` on backoff | Checklist reconnect + no write storm |
| C.6 | generalPurpose | Observability on settings/status screen: device ID, firmware version, last sync, RSSI, server host:port | Matches checklist observability |
| C.7 | generalPurpose | Settings UI to edit NVS `srv_host` / `srv_port` | Host change without reflash |
| C.8 | generalPurpose | Render `pinnedNotes` on home; optional week dinner summary from `weekDinner` | Aligns with server payload |
| C.9 | code-reviewer | Review firmware diff vs thin-client rules | No persistent canonical state in SPIFFS |

**Phase C stop conditions (from 3D Design Labs Build — mandatory):**

- ESP32 asked to own persistent state or automation rules — **stop**, re-read architecture book.
- Offline behavior undefined for new screens — define before continuing.
- LVGL integration blocked by missing Waveshare libs — stop and document hardware blocker; do not fake pass on serial-only.
- Plan would control high-risk physical devices — **out of scope**; stop.

**Embedded UI constraints (mandatory):** minimal screens, high contrast, low animation, glanceable, clear write success/failure, no dense tables.

---

### Phase D — Deploy & verification (Verify/Handoff)

**Objective:** endeavor production path validated; evidence for all checklist sections.

| Step | Owner | Action | Expected result |
|------|-------|--------|-----------------|
| D.1 | generalPurpose | Update `docs/endeavor-deploy.md`: first-boot seed vs empty DB, `WRITE_TOKEN` final policy, systemd reload steps | Operator can deploy without guessing |
| D.2 | generalPurpose | Deploy to endeavor per docs; `systemctl enable --now family-hub` | `curl http://endeavor:3020/api/health` → `ok: true` |
| D.3 | generalPurpose | Run `BASE_URL=http://endeavor:3020 bash tools/smoke-test.sh` | Exit 0 |
| D.4 | generalPurpose | Execute [verification-checklist.md](verification-checklist.md) — server, browser, ESP32, observability, safety | Completed checklist with dates |
| D.5 | generalPurpose | Capture evidence: test output, smoke log, panel photos or serial log excerpts, `systemctl status` | Stored under `docs/verification-evidence/` (gitignored if sensitive) |
| D.6 | validation-review | Final gate — all checklist items addressed or explicitly waived | Sign-off table |
| D.7 | Michael | Handoff review — approve v0.1 for daily use or list follow-ups | v0.1 repair closed or Phase E scoped |

**Phase D stop conditions:**

- Modifying production HA/ESPHome/MQTT config — requires separate plan.
- Port 3020 exposed beyond LAN — stop until TLS/firewall plan approved.

---

## Quality Gates

| Gate | When | Checks | Passing criteria |
|------|------|--------|------------------|
| **security-review** | End of Phase A (auth/secrets/deploy) | `WRITE_TOKEN` wiring, no secrets in git, LAN bind policy, firmware secret provisioning | No critical auth gaps; documented policy matches code |
| **validation-review** | End of Phase B and Phase D | `npm test`, smoke script, checklist mapping | All automated checks green; no flaky PORT binding |
| **compatibility-scan-review** | End of Phase B | Agent-compatibility CLI + firmware/server contract | No undocumented contract breaks; score baseline recorded |
| **code-reviewer** | Optional end of each phase | Diff vs plan scope | No unapproved scope creep |
| **Human approval** | Phase 0 and Phase D handoff | Architecture book + safety boundaries + checklist | Michael explicit approve |

---

## Verification

### Automated (run after Phase A/B)

```bash
cd /home/stitch/Desktop/Operating/pi-iot/family-hub/server
npm test
```

```bash
cd /home/stitch/Desktop/Operating/pi-iot/family-hub
bash tools/smoke-test.sh
# With token enabled:
# WRITE_TOKEN=xxx FAMILY_HUB_TOKEN=xxx bash tools/smoke-test.sh
```

```bash
cd /home/stitch/Desktop/Operating/pi-iot/family-hub/firmware
pio run -e devkit
pio run -e waveshare7
```

### Contract probes

```bash
curl -s http://127.0.0.1:3020/api/health | jq .
curl -s http://127.0.0.1:3020/api/dashboard-state | jq '.today, .members, .connection, .pinnedNotes'
```

```bash
# Auth rejection when WRITE_TOKEN set
curl -s -o /dev/null -w "%{http_code}" -X POST http://127.0.0.1:3020/api/grocery \
  -H 'Content-Type: application/json' -d '{"text":"unauth test"}'
# Expect 401 when token required
```

### Firmware evidence (Phase C/D)

- Serial log: firmware version, device ID on boot (`main.cpp` boot path).
- Serial log: `[write] FAILED` on unreachable server; `[write] OK` on success.
- Wi-Fi drop test: disconnect AP → stale badge → reconnect → resync without request storm.
- Photo or screenshot: LVGL home screen readable at ~7" distance.

### Deploy evidence (Phase D)

```bash
ssh endeavor 'systemctl status family-hub'
curl -s http://endeavor:3020/api/health
BASE_URL=http://endeavor:3020 bash tools/smoke-test.sh
```

### Compatibility

```bash
# From repo root, if agent-compatibility CLI available:
npx agent-compatibility scan /home/stitch/Desktop/Operating/pi-iot/family-hub
```

---

## Approval Gate

**Do not execute Phases A–D until Michael responds with one of:**

1. **Approve** — proceed Phase A sequentially; parallelize only as marked above.  
2. **Request changes** — revise this document first.  
3. **Mark sections out of scope** — e.g. defer LVGL to v0.1.1 (explicitly waives panel checklist items).

Required decisions before Phase A execution:

- [ ] `WRITE_TOKEN`: wire + require **or** disable + document  
- [ ] `ADMIN_PIN`: wire to routes **or** remove  
- [ ] Calendar `events` table: drop **or** defer v0.2  
- [ ] Confirm Waveshare hardware available for Phase C/D  

---

## Stop Conditions (global)

Stop and re-architect (do not continue implementation) if:

1. ESP32 is asked to own canonical family-ops data or automation rules.
2. Source of truth becomes unclear between server, panel cache, and browser.
3. Offline/reconnect behavior is undefined for new UI flows.
4. No verification plan for network loss, reboot, and failed writes.
5. Production secrets would be hardcoded in firmware without approved approach.
6. API would be internet-exposed without TLS and security plan.
7. Production HA/ESPHome/MQTT config would be modified under this plan.
8. High-risk physical device control is introduced.
9. Phase quality gate fails with critical findings unresolved.

---

## Rollback Notes

| Phase | Rollback |
|-------|----------|
| A | `git reset` to pre-init backup; restore prior filesystem copy if needed |
| B | Revert server commits; restore SQLite from backup if migration ran |
| C | Reflash previous firmware binary; SPIFFS cache is disposable |
| D | `systemctl stop family-hub`; restore prior systemd unit and `.env`; restore DB backup |

Keep endeavor DB backup before Phase D deploy: `cp /var/lib/family-hub/family-hub.sqlite ~/family-hub.sqlite.bak`

---

## Human Review Checkpoints

1. **Phase 0:** Plan approval + policy decisions (this document).  
2. **After Phase A:** security-review report + `git log -1` sanity.  
3. **After Phase B:** test/smoke output + compatibility gate.  
4. **After Phase C:** hardware demo or log evidence before endeavor dependency.  
5. **Phase D:** Full checklist sign-off before declaring v0.1 repaired.

---

## Files Likely Touched

| Phase | Paths |
|-------|-------|
| A | `.gitignore`, `firmware/src/main.cpp`, `firmware/include/config.h`, `server/config/env.js`, `web/js/api.js`, `firmware/src/api_client.cpp`, `deploy/family-hub.service`, `docs/endeavor-deploy.md`, `server/.env.example` |
| B | `server/tests/api.test.js`, `tools/smoke-test.sh`, `server/services/dashboardState.js`, `server/db/schema.sql`, `server/services/idempotency.js`, `docs/architecture-book.md` |
| C | `firmware/platformio.ini`, `firmware/src/ui_manager.cpp`, `firmware/include/ui_manager.h`, `firmware/src/main.cpp`, `firmware/src/api_client.cpp`, `docs/waveshare-setup.md` |
| D | `docs/endeavor-deploy.md`, `docs/verification-checklist.md`, `docs/verification-evidence/` (new, optional) |

---

## Agent Build Plan Checklist (self-review)

- [x] Each delegated agent has clear task and expected output.  
- [x] Parallel tasks do not depend on uncommitted edits in the same files.  
- [x] Quality gates match auth, secrets, contract, and test risk.  
- [x] Plan works if optional `code-reviewer` finds nothing.  
- [x] Human can follow sequence without reading agent transcripts.  
- [x] Aligned with 3D Design Labs Build: Architecture Book first, approval before execution, IoT stop conditions included.

---

*Generated for agent execution via `execute-planner-plan` skill — only after approval.*
