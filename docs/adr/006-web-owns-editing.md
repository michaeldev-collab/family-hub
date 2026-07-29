# ADR-006 — Web application owns editing

**Status:** Accepted  
**Date:** 2026-07-26  
**Pattern source:** Calendar Hub `docs/adr/006-web-owns-editing.md` (adapted)

## Context

Creating and editing household data on a 1024×600 wall panel is high-friction and expands firmware attack/UX surface.

## Decision

All create/edit/delete for members, grocery, chores (definitions), dinner, notes, and settings configuration happen in the **browser admin** (SPA served by the server).

Panel actions are limited to: navigation, sync/refresh, sleep/wake/diagnostics gestures, **chore complete**, and **grocery-state toggle**.

## Consequences

- Web app is required for day-to-day household management.
- Panel remains glance + complete oriented.
- Rewards/child admin surfaces stay archived — not reopened by this ADR.
