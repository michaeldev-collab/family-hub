# CEO Handoff Brief — Family Hub V1 (Vacation Mode)

**CEO:** Michael  
**Effective:** 2026-06-18  
**Authority:** Autonomous agent execution approved through V1 sign-off  
**Repo:** `/home/stitch/Desktop/Operating/pi-iot/family-hub`

---

## What Michael Already Decided

These policies are **final for V1**. Agents must not re-open them or ask Michael during vacation.

| Decision | Choice | Evidence |
|----------|--------|----------|
| **WRITE_TOKEN** | Optional — empty = LAN trust mode; when set, all write clients send `x-family-hub-token` | `server/middleware/auth.js`, `web/js/api.js`, `firmware/src/api_client.cpp`, `docs/endeavor-deploy.md` |
| **ADMIN_PIN** | Removed — no PIN middleware, no deploy docs referencing it | `server/middleware/auth.js` exports only `optionalWriteAuth` |
| **Calendar / events API** | Deferred to v1.1 — `events` table exists in schema only; no API/UI in V1 | `docs/architecture-book.md` |
| **Architecture** | Server SQLite on endeavor = source of truth; ESP32 = thin client + SPIFFS cache only | `docs/architecture-book.md`, `docs/safety-boundaries.md` |
| **Scope ceiling** | No MQTT, Home Assistant, ESPHome, TLS/public exposure, or high-risk device control in V1 | `docs/safety-boundaries.md`, `docs/phase-repair-plan.md` |
| **Seed policy** | First boot on empty DB: run `npm run seed` (idempotent demo data); empty production start = `init-db` only | `docs/endeavor-deploy.md` |
| **Deploy target** | endeavor (Arch Linux), port 3020, LAN-only, systemd `family-hub.service` | `docs/endeavor-deploy.md`, `deploy/family-hub.service` |
| **Panel hardware** | Waveshare ESP32-S3-Touch-LCD-7 (or 7B); LVGL required for **V1-full** only | `docs/waveshare-setup.md` |
| **Panel not received (2026-06-18)** | **Confirmed by Michael** — defer Phase F–G; ship V1-without-panel (server + browser) | [vacation-mode-complete.md](vacation-mode-complete.md) |

---

## v0.1 Repair Status (Handoff Baseline)

**Status:** Mostly executed — one commit on `main` (`b9367fd Initial commit: Family Hub v0.1`).

| Workstream | Status | Remaining |
|------------|--------|-----------|
| WS-1 Repo & secrets | Done | None |
| WS-2 Auth & deploy safety | Done | None |
| WS-3 Server contract & tests | Done locally | endeavor restart persistence (manual) |
| WS-4 Firmware thin client | Partial | LVGL on Waveshare — **DEFERRED-HARDWARE (W-HW01)**; prep docs complete |
| WS-5 Deploy & verification | Partial | Phase E best-effort (SSH blocked); Phases F–G deferred |

**Verification checklist:** Server automated rows pass; browser manual rows and ESP32 hardware rows open. See `docs/verification-checklist.md`.

---

## Pre-Approved Defaults (Vacation Mode)

Agents may act on these without CEO contact:

1. **Proceed with Phases E–I** per `v1-master-plan.md` and `v1-agent-runbook.md`.
2. **Use LAN trust mode** on endeavor (`WRITE_TOKEN=` empty) unless evidence shows unauthorized LAN writes — do not enable token mid-vacation without CISO gate escalation.
3. **Seed demo data** on first endeavor deploy if DB is empty (per deploy doc).
4. **Stop, document, and continue** when blocked by hardware/SSH — do not scope-creep to work around blockers.
5. **Capture evidence** under `docs/verification-evidence/` (gitignored) for every manual/hardware step.
6. **Mark checklist items WAIVED** only with entry in `v1-risks-and-waivers.md` and board log update — never silent skip.
7. **No new features** beyond v0.1 surface + LVGL usability — defer v1.1 items (calendar, MQTT, member CRUD polish, week-dinner planner UI).

---

## Implicit Waivers Michael Granted

By requesting **zero CEO input during vacation**, Michael implicitly waives:

- **Per-step approval gates** — replaced by pre-approved board pack (`09-board-decision-log.md`).
- **Phase 0 human gate** in `phase-repair-plan.md` — superseded by this handoff.
- **Phase D Michael handoff review** — replaced by Phase I agent sign-off + evidence bundle.
- **Live demo to CEO** before V1 — photo/serial/log evidence acceptable.
- **Purchasing** — no new hardware or cloud spend without CEO return (see CFO gate).
- **Production HA/MQTT changes** — remain out of scope; no waiver.

---

## Git / Commit Recommendation

Working tree is clean on `main`. **Do not commit** during vacation unless:

- Completing a phase with passing automated verification, **or**
- Adding evidence index files (non-sensitive) under `docs/verification-evidence/`.

**Suggested commit cadence (optional, agent discretion):**

| Trigger | Message pattern |
|---------|-----------------|
| Phase E complete | `deploy: endeavor production baseline` |
| Phase G complete | `firmware: LVGL panel UI for waveshare7` |
| Phase I complete | `release: Family Hub v1.0 verification sign-off` |

Michael may squash/review on return. Agents should note commit SHAs in evidence README.

---

## Success Statement for Michael's Return

**V1.0 (without panel)** is done when:

1. endeavor runs `family-hub.service` with health + smoke pass.
2. Browser admin works on LAN (CRUD + offline UX).
3. `v1-verification-checklist.md` server/browser rows PASS; panel rows WAIVED (W-HW01).
4. `09-board-decision-log.md` shows gates GREEN or waived YELLOW.

**V1-full** adds when Waveshare arrives:

5. Panel shows readable LVGL UI, offline badge, and v0.1 writes with server confirmation (Phase F–G).

---

## First Actions (Vacation Day 1)

See `v1-agent-runbook.md` § Day 1. Summary:

1. Run automated baseline (`npm test`, smoke, firmware build).
2. Execute Phase E (endeavor deploy) if SSH available.
3. If no SSH: document blocker + **DEFERRED-HARDWARE** for Phase F–G (panel not received).

---

## Escalation (Only These Require CEO)

- Exposing port 3020 beyond LAN or enabling public DNS.
- Enabling WRITE_TOKEN without client re-provision plan.
- Dropping safety boundaries or adding device control.
- Spending money or replacing hardware.
- Deleting production family data without backup.

Everything else: execute per roadmap.
