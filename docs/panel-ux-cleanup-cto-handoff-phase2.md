# CTO Handoff — Family Hub Panel UX Phase 2 (Shell UX)

**From:** CTO planning (2026-07-26)  
**Gate:** CEO ★ required before firmware Execute  
**Plan:** [panel-ux-cleanup-phase2-plan.md](panel-ux-cleanup-phase2-plan.md)  
**Product lock:** [panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md) §§4–5

## Objective

Sleep pipeline + `ShellGestures` (touch chrome + stick stub) + diagnostics overlay on household Waveshare panel. No Phase 3 contracts.

## Paste-ready Execute prompt (after CEO ★)

```text
/3dl-matrix-cto Execute Family Hub Panel UX Phase 2 shell UX per docs/panel-ux-cleanup-phase2-plan.md.

Authority: CEO ★ Phase 2 authorized.
Follow WP-A → WP-E. Cite Done= from that plan and docs/panel-ux-cleanup-addendum.md §7 Phase 2.

Constraints:
- Do NOT start Phase 3 contracts
- Do NOT remount archives or deploy systemd
- Waveshare primary; joystick no-op OK until elecrow7
- Aikido scan modified first-party sources before complete
- Prefer reversible display sleep API; no burn-in path
```
