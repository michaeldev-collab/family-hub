# Family Hub — Panel UX Phase 4 Plan (Polish + prod path)

**Status:** Phase 4 polish **COMPLETE** in-repo (2026-07-26); systemd S8/S9 = **operator script** (agent cannot chown `/opt`)  
**Date:** 2026-07-26  
**Product lock:** [panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md) §7 Phase 4  
**Out of scope:** rewards/child remount, WRITE_TOKEN force-on, public exposure, calendar domain merge

---

## 1. Goal

Close panel UX cleanup with acceptance evidence, keep browser admin coherent with contracts, and put the household API on a durable systemd path (S8/S9) on this LAN host.

**Done =**

1. Panel UX acceptance checklist recorded (Phases 1–3 + sleep/wake UX)
2. README / addendum reflect household + Phase 4 complete
3. Smoke test asserts `schema_version` VMs
4. Builds still green (or documented skip if no firmware change)
5. `family-hub.service` active; smoke PASS; S8 restart preserves data; S9 `active`
6. Follow-on calendar API/web borrow plan committed (planning only)

---

## 2. Work packages

### WP-A — Acceptance rollup

- `docs/verification-evidence/phase4-panel-ux-acceptance-20260726.md`
- Cite Phase 2 touch scorecard, Phase 3 boot serial, sleep/wake fix notes

### WP-B — Docs polish

- README: Phase 0 “no Execute” → Phases 1–4 complete; how to run + gestures
- Addendum §7 Phase 4 → COMPLETE
- Panel contracts: wake = any tap while asleep

### WP-C — Smoke / contract assert

- `tools/smoke-test.sh` requires `schema_version` + `home` key
- `npm test` still green

### WP-D — Prod path (this host)

Host: `dev-pc` with `192.168.1.134` (panel target) + `192.168.1.137`.

**Operator one-shot** (sudo required; blocked from agent sandbox):

```bash
bash tools/phase4-local-systemd-deploy.sh
```

Preserves `server/data/family-hub.sqlite` → `/var/lib/family-hub/`. Unit binds `0.0.0.0:3020`. Evidence lands in `docs/verification-evidence/E-*-20260726.*`.

### WP-E — Calendar borrow plan (docs only)

- `docs/calendar-arch-borrow-plan.md` — ordered future phases; no Execute

---

## 3. Explicit non-goals

- Force-enable `WRITE_TOKEN`
- Remount archives
- Cloudflare / public admin
- Implementing calendar ETag/`state_version` in this phase (plan only)

---

## 4. CEO ★ gate

Phase 4 Execute authorized by CEO request 2026-07-26. Deploy is LAN systemd on this workstation (same path as endeavor-deploy “dev-pc” section).

---

## 5. Execute record (2026-07-26)

| Check | Result |
|-------|--------|
| WP-A acceptance rollup | Done — `phase4-panel-ux-acceptance-20260726.md` |
| WP-B docs | Done — README, addendum, panel-contracts gestures |
| WP-C smoke schema assert | Done — `tools/smoke-test.sh`; `npm test` 25/25 |
| WP-D systemd | Operator script `tools/phase4-local-systemd-deploy.sh` (agent blocked on `/opt` chown) |
| WP-E calendar borrow plan | Done — `docs/calendar-arch-borrow-plan.md` (C0–C6, ★ before Execute) |
| Live API | Still on ad-hoc node `:3020` until operator deploy |
