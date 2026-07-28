# COO Ops Gate — Delivery & Evidence

**Gate ID:** COO-01  
**Status:** GREEN  
**Date:** 2026-06-18

---

## Delivery Phases (V1)

Continues from v0.1 repair (Phases A–D mostly complete). V1 execution:

| Phase | Name | Dependency | HITL |
|-------|------|------------|------|
| **E** | endeavor production deploy | SSH to endeavor | Yes |
| **F** | Hardware verification (devkit → panel) | USB + WiFi secrets | Yes |
| **G** | LVGL panel UI | Phase F build path proven | Yes (flash) |
| **H** | V1 gap fill | E + G progress | Partial |
| **I** | V1 sign-off | All prior | No |

Detail: `v1-phase-plan.md`.

---

## Dependencies

```text
[A–D v0.1 repair] ──► [E endeavor deploy]
                           │
                           ├──► [H browser + restart tests]
                           │
[F secrets + build] ──► [G LVGL + flash] ──► [H panel checklist]
                                                   │
                                                   ▼
                                              [I sign-off]
```

**Critical path:** E (server live) + G (panel UI) → I.

---

## Vacation Execution Schedule

Flexible calendar; **sequence enforced**:

1. **Day 1:** Automated baseline + Phase E (if SSH)
2. **Days 2–3:** Phase F — devkit serial proof, then Waveshare flash
3. **Days 4–6:** Phase G — LVGL screens + touch writes
4. **Days 7–8:** Phase H — manual browser + DB restart + gap items
5. **Day 9+:** Phase I — full `v1-verification-checklist.md`, board log GREEN

If SSH blocked entire vacation: complete F/G/H-local + document E as RED blocker for Michael's return.

---

## Rollback Procedures

| Phase | Rollback command / action |
|-------|---------------------------|
| E deploy | `ssh endeavor 'sudo systemctl stop family-hub'`; restore DB from `~/family-hub.sqlite.bak` |
| E config | Restore prior `/opt/family-hub` tarball if kept |
| F/G firmware | Reflash prior `.pio/build/waveshare7/firmware.bin` or devkit env |
| G LVGL bad | Revert git commit; flash previous binary |
| H data | SQLite backup restore (see endeavor-deploy.md) |

**Before Phase E:** `cp /var/lib/family-hub/family-hub.sqlite ~/family-hub.sqlite.bak` on endeavor (if DB exists).

---

## Evidence Requirements

All manual/HITL steps must produce artifacts in `docs/verification-evidence/`:

| Artifact | Naming | Required for |
|----------|--------|--------------|
| Deploy log | `E-deploy-YYYYMMDD.txt` | Phase E |
| `systemctl status` | `E-systemctl-YYYYMMDD.txt` | Phase E |
| Smoke on endeavor | `E-smoke-YYYYMMDD.txt` | Phase E |
| Serial boot log | `F-boot-YYYYMMDD.txt` | Phase F |
| Panel photo | `G-panel-home-YYYYMMDD.jpg` | Phase G |
| WiFi drop test log | `G-wifi-reconnect-YYYYMMDD.txt` | Phase G |
| Browser test notes | `H-browser-YYYYMMDD.md` | Phase H |
| Checklist sign-off | `I-checklist-YYYYMMDD.md` | Phase I |

Update `docs/verification-evidence/README.md` index after each phase.

---

## Stop Conditions (COO)

Stop phase and document; do not skip to sign-off:

1. Smoke test fails on endeavor after 2 fix attempts
2. Panel cannot build `waveshare7` after dependency install
3. LVGL remains serial-only at Phase G deadline → escalate waiver path (CEO brief)
4. Data loss risk on deploy without backup

---

## Exit Criteria (COO Gate)

- [x] Phases E–I defined with rollback
- [x] Evidence paths specified
- [x] Schedule compatible with vacation async work

**Gate:** GREEN
