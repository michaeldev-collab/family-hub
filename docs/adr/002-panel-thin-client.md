# ADR-002 — Panel is a thin client

**Status:** Accepted  
**Date:** 2026-07-26  
**Pattern source:** Calendar Hub `docs/adr/002-panel-thin-client.md` (adapted)

## Context

Waveshare ESP32-S3 panel (1024×600) is a wall glance + constrained input device. Firmware cannot own business rules, CRUD, or generic layout engines.

## Decision

The household panel renders fixed LVGL screens from **typed server view models** (`schema_version` + nested VMs) and performs only **one** mutation: chore complete.

It does not CRUD members/grocery/dinner/notes, run rewards/child flows, or execute server-driven UI blocks.

## Consequences

- Firmware focuses on display, network, validation, sleep/gestures, diagnostics.
- View-model generation stays on the server ([panel-contracts.md](../api/panel-contracts.md)).
- Matches 3DL IoT edge-node guardrail and household-only product cut.
