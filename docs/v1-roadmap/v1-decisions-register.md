# V1 Decisions Register — Pre-Decided Defaults

**Rule:** If a question appears below, use the registered answer. **Do not ask Michael.**

---

## Auth & Security

| ID | Question | Decision | Reference |
|----|----------|----------|-----------|
| D-01 | Enable WRITE_TOKEN on endeavor for V1? | **No** — empty token, LAN trust mode | CEO brief |
| D-02 | Wire ADMIN_PIN? | **No** — removed entirely | `server/middleware/auth.js` |
| D-03 | Require HTTPS? | **No** for V1 LAN | CISO gate |
| D-04 | Expose API to internet? | **No** | Safety boundaries |
| D-05 | Store WRITE_TOKEN in firmware build flags? | **No** in git; NVS at runtime if ever enabled | waveshare-setup |

---

## Data & Deploy

| ID | Question | Decision | Reference |
|----|----------|----------|-----------|
| D-10 | Seed demo data on first deploy? | **Yes** if DB empty (`npm run seed`) | endeavor-deploy |
| D-11 | Empty production DB without demo? | Skip seed; add members via browser | endeavor-deploy |
| D-12 | DB path on endeavor | `/var/lib/family-hub/family-hub.sqlite` | systemd unit |
| D-13 | Backup before deploy? | **Yes** if DB exists | v1-phase-plan E.1 |
| D-14 | Git commit cadence | Optional per phase; note SHAs in evidence | CEO brief |

---

## Product Scope

| ID | Question | Decision | Reference |
|----|----------|----------|-----------|
| D-20 | Calendar / events API? | **Deferred v1.1** — schema stays, no API | Architecture book |
| D-21 | Drop events table? | **No** — leave schema, document deferred | Phase B decision |
| D-22 | MQTT / Home Assistant? | **Out of scope V1** | Architecture book |
| D-23 | Week-dinner planner UI? | **Defer v1.1** | CPO gate |
| D-24 | Member CRUD polish? | **Defer** — API + basic UI sufficient | CPO gate |
| D-25 | CORS / rate limits? | **Document only** — no implementation V1 | Out of scope |
| D-39 | Reminders delivery? | **n8n + Google Voice** (outbound only); UI + schema prep in v1.0.x; delivery worker + n8n workflow in **v1.1** | reminders-architecture.md |

---

## Firmware & Panel

| ID | Question | Decision | Reference |
|----|----------|----------|-----------|
| D-30 | Default server host | `DEFAULT_SERVER_HOST` in secrets.h = endeavor LAN IP | secrets.example.h |
| D-31 | Server port | 3020 unless NVS overridden | config.h |
| D-32 | Primary panel env | `waveshare7` for production | platformio.ini |
| D-38 | Dual 7" panel support | **Waveshare primary** (`waveshare7`); **Elecrow secondary** (`elecrow7`, V1.1 bonus) | panel-targets.md |
| D-33 | Dev testing env | `devkit` serial fallback OK for F, not for V1 done | waveshare-setup |
| D-34 | LVGL required for V1? | **V1-full yes; V1.0 no** — panel deferred W-HW01 (CEO 2026-06-18) | vacation-mode-complete |
| D-37 | Panel hardware not received? | **Defer Phase F–G** — ship V1-without-panel (server + browser) | CEO decision 2026-06-18 |
| D-35 | FIRMWARE_VERSION at V1 | Bump to `1.0.0` at Phase I | v1-master-plan |
| D-36 | Panel owns state? | **Never** — SPIFFS cache only | Architecture book |

---

## Verification & Process

| ID | Question | Decision | Reference |
|----|----------|----------|-----------|
| D-40 | Skip manual checklist rows? | **No** — PASS or WAIVED with ID | v1-verification-checklist |
| D-41 | Waive LVGL for V1.0? | **Yes** — W-HW01; V1.0 = server/browser; V1-full needs Phase G | CPO gate, vacation-mode-complete |
| D-42 | Who signs off V1? | Validation agent + board log | Phase I |
| D-43 | Edit `.cursor/plans/`? | **Forbidden** | User constraint |
| D-44 | Run agent-compatibility scan? | **If CLI available** after Phase G | phase-repair-plan |

---

## Connection / dashboard-state Contract

| ID | Question | Decision | Reference |
|----|----------|----------|-----------|
| D-50 | Server reports live offline? | **No** — clients infer; `connection.sourceOfTruth: server` | Architecture book |
| D-51 | False "online" from server? | **Fixed** — no misleading online flag | dashboardState.js |

---

## Conflict Resolution

If two documents conflict:

1. `00-ceo-handoff-brief.md` wins
2. Then `v1-decisions-register.md` (this file)
3. Then `09-board-decision-log.md`
4. Then `docs/architecture-book.md`

If still ambiguous: choose the **narrower scope** option and log in evidence.

---

*Last updated: 2026-06-18 — vacation pack*
