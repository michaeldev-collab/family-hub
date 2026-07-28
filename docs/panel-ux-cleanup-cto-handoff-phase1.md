# CTO Handoff — Family Hub Panel UX Phase 1 (Hygiene)

**From:** CPO / Phase 0 product lock (2026-07-26)  
**To:** `/3dl-matrix-cto` (build-mgr → hygiene Execute)  
**Gate:** CEO ★ required before any firmware/source Execute  
**Do not start** until Michael explicitly authorizes Phase 1.

## Product lock (already done)

- [docs/panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md) — steal/anti-list, gesture C, screens, sleep/diag, module map, Done= criteria
- [README.md](../README.md) — household-only scope + addendum link
- [firmware-split.md](firmware-split.md) — child sections marked ARCHIVED; archive paths linked
- [docs/panel-ux-cleanup-phase1-plan.md](panel-ux-cleanup-phase1-plan.md) — **Phase 1 engineering plan** (audit + WP-1…WP-5)

**Scope reminder:** household-only; rewards/child stay archived. Waveshare primary; Elecrow stick secondary for gesture parity later (Phase 2).

## Phase 1 objective (hygiene only)

Make the household firmware tree match the product cut — no shell UX, no contracts, no sleep port yet.

**Read first:** [panel-ux-cleanup-phase1-plan.md](panel-ux-cleanup-phase1-plan.md) — filters already exclude some orphans on Waveshare; headers and `waveshare7b` missing `FAMILY_HUB_APP_HOUSEHOLD` are the real debt.

| Task | Detail |
|------|--------|
| Macro alignment | Add `-DFAMILY_HUB_APP_HOUSEHOLD=1` to `waveshare7b`; fence `devkit` filters |
| Remove duplicate display | Delete/archive root `firmware/src/display.cpp`; keep `shared/display/display.cpp` (newer) |
| Strip child from household | Headers + `ui_manager_core.cpp`; remove/archive `child_focus_state.*` |
| Header / link cleanliness | Household ELF must not retain ChildFocus / reward panel symbols |

**Done =**

1. `pio run -e waveshare7b`, `-e waveshare7b-household-launcher`, `-e devkit` succeed
2. No child symbols in household link map (`nm` + `rg` per phase1 plan WP-5)
3. No production deploy, no auth policy change, no archive remount

**Out of scope for Phase 1:** sleep pipeline, `ShellGestures`, diagnostics overlay, view-model contracts, systemd Phase E.

## Paste-ready Execute prompt (after CEO ★)

```text
/3dl-matrix-cto Execute Family Hub Panel UX Phase 1 hygiene per docs/panel-ux-cleanup-phase1-plan.md.

Authority: CEO ★ Phase 1 authorized.
Follow WP-1 → WP-5 in order. Cite Done= from that plan and docs/panel-ux-cleanup-addendum.md §7 Phase 1.

Constraints:
- Do NOT start Phase 2 (sleep/gestures/diagnostics)
- Do NOT remount archives or deploy systemd
- Prefer archive-then-delete for root display.cpp and child_focus_state.*
- Aikido scan modified first-party sources before complete
```

## Phase 2 preview (not authorized)

Sleep + `ShellGestures` (touch + stick) + diagnostics overlay per addendum §4–5. Separate CEO ★.
