# Family Hub — Architecture Book (v0.1)

## ADRs

Canonical decisions live under [docs/adr/](adr/README.md) (API SoT, thin panel, fixed contracts, polling, web-owns-editing, stack defaults). This book remains the narrative overview; ADRs win on conflict for the topics they cover.

## Source of truth
- **Server database (SQLite on endeavor)** owns all family ops data.
- **ESP32 panel** caches last-known display state in SPIFFS only.
- **Home automation** (MQTT/HA/ESPHome) is a separate future domain.

## System roles
| Component | Owns | Must not own |
|-----------|------|--------------|
| endeavor server | groceries, chores, dinner, notes, members | GPIO, LVGL, device control |
| ESP32 panel | display cache, UI settings (NVS) | canonical data, automation rules |
| Browser | nothing persistent | independent data silos |
| MQTT/HA (future) | device state, routines | grocery/chore data |

## Communication (v0.1)
- ESP32 → `GET /api/dashboard-state`
- ESP32 → `POST /api/events` (idempotent)
- Browser → REST CRUD
- HTTP-only is acceptable for v0.1
- Optional `WRITE_TOKEN`: when set on the server, clients send `x-family-hub-token` on writes; empty token = LAN trust mode

**Calendar:** The `events` table exists in SQLite but v0.1 has no calendar API or UI — deferred to v0.2.

## Offline behavior
- Panel shows cached data + stale/offline badge
- Writes not shown as successful unless server confirms
- Reconnect uses exponential backoff
- `dashboard-state.connection` reports `sourceOfTruth: server` only; clients infer offline/stale locally

## Safety
See [safety-boundaries.md](safety-boundaries.md).

## Product boundary (household-only)

Rewards/behavior tracking and the child-facing panel are **not** part of the live
Family Hub product. They were archived out of tree on 2026-07-26.

| Topic | Status | Where to look |
|-------|--------|---------------|
| Household grocery / chores / dinner / notes + wall panel | **Live** | This book, [adr/](adr/README.md), [api/panel-contracts.md](api/panel-contracts.md) |
| Rewards & discipline add-on | **Archived** (separate product cut) | [archive/REWARDS_BEHAVIOR_MOVED.md](../archive/REWARDS_BEHAVIOR_MOVED.md); historical design: [archive/reward-discipline-architecture-historical.md](../archive/reward-discipline-architecture-historical.md) |
| Child panel | **Archived** (separate product cut) | [archive/CHILD_PANEL_MOVED.md](../archive/CHILD_PANEL_MOVED.md) |

Do not treat `prop.md`, reward-discipline plans, or child UI rebuild notes as live
Family Hub architecture.

## Reminders (v1.1 deferred)

Outbound SMS/voice reminders for groceries, chores, dinner, and notes — **server-owned schedule**, **n8n delivery layer** (Google Voice). UI fields and member phone mapping ship in v1.0.x prep; actual webhook delivery is v1.1.

Full spec: [reminders-architecture.md](reminders-architecture.md).

## V1 roadmap
Vacation-mode execution pack (Phases E–I): [v1-roadmap/v1-master-plan.md](v1-roadmap/v1-master-plan.md). Agent entry: [v1-roadmap/v1-agent-runbook.md](v1-roadmap/v1-agent-runbook.md).
