# ADR-003 — Fixed screen contracts

**Status:** Accepted  
**Date:** 2026-07-26  
**Pattern source:** Calendar Hub `docs/adr/003-fixed-screen-contracts.md` (adapted)

## Context

Generic UI-block / server-driven layout systems add firmware complexity and schema risk.

## Decision

Household v1 uses fixed screen-specific view models on a single dashboard blob:

- `home`, `grocery`, `chores`, `dinner`, `notes` (+ Settings/local status)
- Required root field: `schema_version` (currently `1`)
- Layout specs under [docs/screens/](../screens/)

No arbitrary block renderer. Unsupported or missing schema → reject payload; open/keep Diagnostics.

## Consequences

- API shapes are stable and testable (markdown tables today; JSON Schema+Ajv in borrow Phase C1).
- UI changes require coordinated contract + firmware updates.
- Overlay Diagnostics is not a nav tab; fail-open on invalid Home.
