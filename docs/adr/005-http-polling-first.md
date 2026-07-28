# ADR-005 — HTTP polling first

**Status:** Accepted  
**Date:** 2026-07-26  
**Pattern source:** Calendar Hub `docs/adr/005-http-polling-first.md` (adapted)

## Context

Household glance freshness does not require sub-second push. WebSockets/SSE add firmware and server complexity.

## Decision

Version 1 uses HTTP polling of `GET /api/dashboard-state` while the panel is active, with immediate refresh after successful chore complete. Exponential backoff on network failure.

**Implemented (borrow Phase C2):** monotonic `state_version` in the dashboard body and as `ETag`; conditional GET with `If-None-Match` → `304` so idle panel polls keep last state and skip full UI rebuild.

## Consequences

- Simpler panel networking and recovery.
- Revisit push transport only if polling proves insufficient.
- C2 (`state_version`/ETag) is implemented; further transport changes still need CEO ★.
