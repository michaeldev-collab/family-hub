# Reward and Discipline Add-On — Phased Engineering Plan

**Program status:** Phases 1–5 source complete against disposable data only;
Phase 5 display contract frozen on 2026-07-17.  
**Product source:** `prop.md` (full scope; no silent reduction).  
**Outcome owner:** Michael.  
**WIP:** one implementation phase at a time.

## Ownership and HITL

| Area | Accountable | Executor | Human review |
|---|---|---|---|
| Product scope and residual risk | Michael | Matrix/CPO | Michael |
| Shared schema/domain/API contract | CTO | single server writer | CPO + CISO |
| Parent workflows/web UI | COO | UI writer | Michael |
| Display contract/ESP32 | CTO IoT | single firmware writer | Michael on hardware |
| Privacy/auth/media | CISO | CTO implementation | CISO gate |
| Learning evidence | CAO | parent-led observation | Michael |
| Verification/handoff | CTO Quality | independent review | Michael sign-off |

Parent judgment remains the HITL authority for task approval, daily status,
reward approval, incidents, consequences, recovery, and resolution.

## Phase 0 — Gate-ready design

**Deliverables:** Architecture Book extension, UX/UI plans, threat model, this
plan, implementation log, auth/session decision, migration design, endpoint
authorization matrix, payload/media/memory budgets.

**Exit:** CHRO capacity green/yellow, CPO product acceptance recorded, CAO
learning evidence plan accepted, COO sequence accepted, CTO build gate green,
CISO controls accepted, CFO T0 plan/band accepted, and CEO Execute approval.

## Phase 1 — Migration foundation and domain model

**Status:** COMPLETE — source and disposable-database verification passed;
Aikido unavailable and explicitly waived by CEO for Phase 1 only on 2026-07-16.

- Schema versioning and ordered migration runner
- Children/attendance, tasks/assignments/requests, ledger, rewards/access/terms,
  term days/goals/redemptions, behavior, consequences, media, display settings,
  and audit persistence
- Fresh and existing-shaped migration fixtures, constraints, indexes, archival

**Exit:** migrations pass without touching live data; invalid/orphan records are
rejected; both example child schedules are representable.

## Phase 2 — Domain services and invariants

**Status:** COMPLETE — source and disposable-database verification passed;
Aikido remains unavailable and is not waived for Phase 2.

- Awards, reversals, balances, completion approval/rejection
- Daily status and all approved term types
- Reward selection/readiness/request/approval/redemption/completion
- Incident, consequence, recovery, resolution, audit
- Central lifecycle transition validation and transactional idempotency

**Exit:** unit/integration tests cover every invariant in the pasted execution
brief and `prop.md`, including concurrent duplicate operations.

## Phase 3 — Parent authorization and administrative API

**Status:** COMPLETE — parent-protected API, additive migration source, and
private/versioned media pipeline passed disposable-database and temporary-media
verification; Aikido remains unavailable and is not waived for Phase 3.

- Implement the approved session/PIN design and CSRF/origin controls
- `/api/v1/admin` overview and complete resource/action surface
- Validation, consistent errors, authorization matrix, media upload pipeline

**Exit:** positive and negative auth/privacy tests pass; handlers stay thin;
uploads produce private originals plus versioned derivatives.

## Phase 4 — Parent web UI

**Status:** COMPLETE — implemented in the existing web architecture and verified
with disposable data. Michael opted out of Aikido for the remainder of this
project on 2026-07-17 and accepted the documented offline/manual-review gap.

- Implement all nine areas in the UI plan using the existing shell/design system
- Complete empty/loading/error/unauthorized states and child-preview boundary
- Desktop, narrow viewport, keyboard, focus, and approval-efficiency checks

**Exit:** parent can fully configure/administer the proposal without the panel;
no child-only administration is introduced.

## Phase 5 — Display API and media delivery

**Status:** COMPLETE — Execute approved by Michael on 2026-07-17; display API,
actions, media delivery, and frozen contract passed with disposable databases
and temporary media directories only.

- Compact allowlisted child DTOs and allowed-action booleans
- Bounded collections, sync/version metadata, panel derivative serving
- Privacy-negative, payload-size, stale/offline, and profile-isolation tests

**Exit:** stable contract fixture approved before firmware modification.

## Phase 6 — ESP32 Child Focus and reward module

- Home module, profile selection/lock, child dashboard, tasks, rewards, waiting,
  ready, correction, protected exit, persistence settings
- On-demand state/media, active-screen-only lifecycle, explicit failed writes
- Preserve legacy modules and server-authoritative behavior

**Exit:** both firmware environments build; repeated navigation is memory-flat;
Waveshare physical interaction/offline/reconnect evidence passes or is recorded
as a genuine hardware blocker.

## Phase 7 — End-to-end verification and learning gate

- Run proposal scenarios A–H and all Phase 1–6 checks
- Browser/panel contract, privacy, audit, accessibility, media, lifecycle checks
- Three natural-use observation sessions per child for CAO evidence
- Aikido scan and fix/rescan loop; unresolved critical findings stop release

## Rollback and stop conditions

- Never run migrations against production/live data without a verified backup,
  restore rehearsal, and explicit database-migration approval.
- Additive feature flags/default-off settings preserve the current Family Hub.
- Stop on unclear source of truth, invalid transition model, cross-child leak,
  unrestricted panel credential, unbounded payload/media, memory regression,
  unresolved critical security issue, or inability to restore an existing DB.
- Deployment, service restart, physical flashing, auth activation, and production
  data changes require their own explicit approvals.

## Tool and economics boundary

Default T0: existing Node, SQLite, browser stack, PlatformIO, LVGL, and local
storage. No paid service, cloud child-data processor, or new dependency is
authorized by this plan. Each phase reports elapsed effort and variance before
the next phase begins.
