# ADR-007 — Locked stack defaults

**Status:** Accepted  
**Date:** 2026-07-26  
**Pattern source:** Calendar Hub `docs/adr/007-stack-defaults.md` (adapted; FH product boundary)

## Context

Family Hub needs locked defaults so contracts and firmware stay consistent. Calendar Hub is a **pattern reference**, not a shared deploy or shared API.

## Decision

| Layer | Default |
|-------|---------|
| Product boundary | Standalone Family Hub repo/service; **household-only**; reuse calendar **patterns**, not merge repos or share deploy |
| Domain | Grocery, chores, dinner, notes, members — not calendar RRULE/month grid; not live rewards/child |
| Backend | Node.js + Express |
| Database | SQLite on home LAN |
| Auth (v1 browser) | Optional Clerk (ADR-008); else LAN trust / `WRITE_TOKEN` |
| Auth (panel) | LAN + optional `PANEL_TOKEN` / `WRITE_TOKEN` |
| Exposure | LAN for panel; optional Cloudflare Tunnel for browser (ADR-008) |
| Firmware toolchain | PlatformIO + Arduino |
| Primary panel | Waveshare ESP32-S3-Touch-LCD-7B, 1024×600, RGB + GT911 + LVGL 8.x |
| Secondary panel | Elecrow (stub / deferred stick gestures) |
| Sync | HTTP polling ([ADR-005](005-http-polling-first.md)); `schema_version` VMs ([ADR-003](003-fixed-screen-contracts.md)) |

## Consequences

- New work scaffolds against these choices.
- Override requires updating this ADR (and CEO ★ where gates apply) before coding.
- Anti-list in [panel-ux-cleanup-addendum.md](../panel-ux-cleanup-addendum.md) §2 remains binding.
