# V1 Master Plan — Single Source of Truth

**Project:** Family Hub  
**Target:** v1.0 operational release  
**Repo:** `/home/stitch/Desktop/Operating/pi-iot/family-hub`  
**Supersedes:** `docs/phase-repair-plan.md` for execution (repair status: mostly done)

---

## Objective

Complete v0.1 repair gaps, deploy to endeavor, deliver browser-usable Family Hub on LAN, and sign off verification — **without CEO input**. Waveshare LVGL panel (Phase F–G) is **DEFERRED-HARDWARE** until panel arrives; V1.0 = server + browser.

---

## Phase Map

| Phase | Name | Owner role | Entry | Exit |
|-------|------|------------|-------|------|
| A–D | v0.1 repair | Done | — | Automated tests green locally |
| **E** | endeavor deploy | Build executor + HITL SSH | Clean git; deploy docs current | endeavor smoke pass + systemd running |
| **F** | Hardware verification | Build executor + HITL flash | **`DEFERRED-HARDWARE`** — Waveshare received + `secrets.h` | Panel/devkit boot + WiFi + API fetch |
| **G** | LVGL UI | Build executor (single writer) | **`DEFERRED-HARDWARE`** — F build OK | On-screen UI + touch writes |
| **H** | Gap fill | Build executor | E + G partial | Browser manual + DB restart pass |
| **I** | V1 sign-off | Validation reviewer | H complete | `v1-verification-checklist.md` done |
| **v1.1** | Reminders (n8n + Google Voice) | Build executor | V1.0 shipped; member phones configured | Outbound delivery via LAN n8n webhook; see `reminders-architecture.md` |

Detail steps: `v1-phase-plan.md`  
Agent procedure: `v1-agent-runbook.md`

---

## Owners (Agent Roles)

| Role | Phases | Subagent |
|------|--------|----------|
| Build executor | E, F, G, H | `generalPurpose` |
| Security reviewer | End E | `security-review` readonly |
| Validation reviewer | End E, F, I | `validation-review` |
| Compatibility | End G | `compatibility-scan-review` |
| Optional diff review | End each phase | `code-reviewer` |

---

## Parallelization Rules

### MAY run in parallel

- **E (deploy)** and **doc/prep for G** — LVGL coding on dev machine without panel (compile-only; no flash)
- **Automated tests** on dev machine anytime
- **Doc/evidence** updates anytime

### MUST serialize

| First | Then | Reason |
|-------|------|--------|
| F devkit proof | G hardware flash | Validate secrets + API before LVGL flash |
| G LVGL complete | H panel checklist rows | Checklist needs on-screen UI |
| E deploy | H restart persistence test | Needs production DB on endeavor |
| B.3 dashboard contract | G home screen | Already done — no rework |

### MUST NOT parallelize

- Two agents editing `ui_manager.cpp`
- Deploy to endeavor while rollback backup skipped

---

## Entry Criteria (V1 Track)

- [x] Architecture Book approved (`docs/architecture-book.md`)
- [x] Safety boundaries approved (`docs/safety-boundaries.md`)
- [x] v0.1 repair Phases A–B complete (tests, auth, git)
- [x] Board pack GREEN (`09-board-decision-log.md`)
- [ ] `npm install` run on dev machine before trusting test results

---

## Exit Criteria (V1 Complete)

All in `02-cpo-product-gate.md` acceptance section, evidenced in `docs/verification-evidence/`, board log Phase I filled.

---

## Stop → Escalate

See `09-board-decision-log.md` RED items. Otherwise: stop phase, log blocker in evidence README, continue other parallel work if safe.

---

## Document Hierarchy

```text
00-ceo-handoff-brief.md     ← CEO decisions + waivers
09-board-decision-log.md    ← Authority
v1-master-plan.md           ← THIS FILE — phases + rules
v1-phase-plan.md            ← Steps + commands
v1-agent-runbook.md         ← Agent procedure
v1-decisions-register.md    ← Defaults (no questions)
v1-verification-checklist.md← Pass/fail rows
v1-risks-and-waivers.md     ← YELLOW/RED mitigations
```

---

## Rollback Master Reference

COO gate `05-coo-ops-gate.md` + phase-repair-plan rollback table (still valid).

---

## Version Tags

| Milestone | Tag suggestion |
|-----------|----------------|
| Post E (V1.0) | `v1.0.0` — server + browser (panel waived W-HW01) |
| Post G (V1-full) | `v1.1.0` or re-tag `v1.0.0` + firmware `FIRMWARE_VERSION=\"1.0.0\"` on panel |
| Post reminders delivery | `v1.1.x` — n8n webhook + Google Voice (UI prep may land in v1.0.x patch) |

---

*Execute with 3DL OS discipline: no scope changes, verify each phase, evidence mandatory.*
