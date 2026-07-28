# ADR-001 — API is the source of truth

**Status:** Accepted  
**Date:** 2026-07-26  
**Pattern source:** Calendar Hub `docs/adr/001-api-source-of-truth.md` (adapted)

## Context

Browser admin and the ESP32 panel must share household state (grocery, chores, dinner, notes, members) without divergence.

## Decision

The LAN Family Hub server and its SQLite database are authoritative for household ops data, dashboard view models, and (when introduced) state versions.

The panel may cache last-validated display state for offline glance only. It must not invent cold-boot data.

## Consequences

- Panel and web never become independent sources of truth.
- Offline panel shows last validated payload + stale/offline indicator only.
- Cold boot without API → Diagnostics / clear offline, not a fake healthy Home.
- Aligns with [architecture-book.md](../architecture-book.md).
