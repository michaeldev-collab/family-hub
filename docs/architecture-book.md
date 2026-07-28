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

## Reward and discipline add-on (next version)

`prop.md` is the approved product specification. This section defines the
technical boundary for implementing it without changing the existing Family
Hub source-of-truth model.

### Bounded context and compatibility

- SQLite on the Family Hub server owns children, tasks, approvals, rewards,
  terms, star transactions, behavior records, media metadata, display settings,
  and audit history.
- The browser is the only creation, editing, approval, administration, and
  detailed-history surface.
- The panel is a thin child-facing display/input client. It may request an
  allowed action but never approves, adjudicates, or stores canonical progress.
- Existing `/api/dashboard-state`, family-ops CRUD, and panel behavior remain
  compatible until an explicit versioned cutover is verified.
- New routes use `/api/v1/admin/*` and `/api/v1/display/*`. Business rules live
  in domain services, not route handlers or UI clients.

### Persistence and migration policy

- Replace ad-hoc growth of `schema.sql` with ordered, additive migrations and a
  schema-version table before adding this subsystem.
- Migrations run transactionally where SQLite permits, preserve existing data,
  and are tested against both a fresh database and a copy-shaped existing
  database fixture.
- Foreign keys remain enabled. Parent-owned definitions archive or deactivate;
  ledger, incident, redemption, and audit history are not cascade-deleted.
- Star transactions are append-only. Corrections create linked reversal or
  administrative-correction entries.
- Approval, award/redemption, idempotency, and audit writes commit atomically.

### Domain invariants

- A completion request can award stars at most once.
- A redemption can deduct stars at most once and cannot overdraw the balance.
- A reversal targets one unreversed transaction and cannot itself be repeated.
- Discipline never automatically changes star balance.
- Recovery never deletes or rewrites its incident.
- Excused and not-present days do not count against progress; attended-day terms
  advance only on attended qualifying days.
- Term duration, schedule, and threshold are child-specific and never assume
  seven days.

### Trust boundaries

- Parent/admin authorization and panel authorization are separate capabilities.
- Every admin read or mutation requires a server-validated parent session.
- The panel credential permits only enumerated display actions for the selected
  child; it cannot access admin routes or approve requests.
- Display DTOs are explicit allowlists. Parent notes, audit records, original
  media paths, contacts, and sibling discipline data are forbidden.
- The service remains LAN-only. Public exposure requires a new security review
  and CEO approval.

The concrete auth/session choice is an Execute-gate decision. A cosmetic or
client-only PIN is not authorization. See `reward-discipline-threat-model.md`.

### Media architecture

- Original uploads are private server assets, stored outside the public web
  root with randomized names.
- Authenticated upload validates size, MIME and magic bytes, dimensions, and
  decode success; generated images are re-encoded with metadata removed.
- Versioned thumbnail and panel derivatives are generated and referenced by
  opaque asset IDs. Display endpoints never return originals.
- Quotas, retention, archival behavior, and derivative invalidation are enforced
  server-side.

### Offline and ESP32 lifecycle

- No reward, approval, redemption, or correction write succeeds optimistically.
  The panel shows pending only after a server-confirmed request and shows clear
  offline, stale, rejected, and retry states.
- Display payloads are bounded by explicit item counts and byte budgets. Child
  detail is fetched on demand rather than added wholesale to the legacy
  dashboard document.
- Only the active module or focus screen remains instantiated. Inactive images
  and transient JSON documents are released on navigation.
- Panel-sized assets are decoded one at a time and cached under a bounded policy.
- Verification records free internal heap, largest contiguous block, PSRAM,
  stack high-water mark, LVGL allocation admission, and repeated-navigation
  deltas on the Waveshare 7B.

### Acceptance boundary

Implementation is not complete until the scenarios in `prop.md` and
`reward-discipline-phase-plan.md` pass, including privacy-negative tests,
duplicate-action tests, migration tests, browser workflow checks, and physical
panel lifecycle evidence or an explicit hardware blocker.

## Reminders (v1.1 deferred)

Outbound SMS/voice reminders for groceries, chores, dinner, and notes — **server-owned schedule**, **n8n delivery layer** (Google Voice). UI fields and member phone mapping ship in v1.0.x prep; actual webhook delivery is v1.1.

Full spec: [reminders-architecture.md](reminders-architecture.md).

## V1 roadmap
Vacation-mode execution pack (Phases E–I): [v1-roadmap/v1-master-plan.md](v1-roadmap/v1-master-plan.md). Agent entry: [v1-roadmap/v1-agent-runbook.md](v1-roadmap/v1-agent-runbook.md).
