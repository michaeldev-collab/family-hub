# Reward and Discipline Add-On — Implementation Record

**Source of truth:** `prop.md`  
**Current goal:** Phase 5 display API and media delivery complete  
**Current status:** Phases 1–5 complete against disposable data; Phase 5
contract frozen on 2026-07-17;
Michael opted out of Aikido for the remainder of this project and accepted the
documented offline/manual-review gap; no live migration, deployment, service
restart, environment edit, or hardware flash ran

## Repository audit

| Concern | Current baseline | Add-on direction |
|---|---|---|
| Backend | Express 4, CommonJS, Node built-in SQLite | Preserve; add domain modules and versioned routes |
| Persistence | `schema.sql` plus ad-hoc column migration | Ordered additive migrations before new tables |
| Auth | Optional broad `WRITE_TOKEN`; scoped chore `PANEL_TOKEN` | Separate parent session and scoped panel actions |
| Web | Vanilla HTML/CSS/JS, existing Family Hub shell | Extend existing UI; no second frontend architecture |
| Panel | LVGL thin client, compact filtered JSON, memory admission | Add on-demand Child Focus with active-screen lifecycle |
| Media | Static bundled assets only | Private originals plus versioned thumbnail/panel derivatives |
| Tests | Node API integration suite | Add migration, domain, auth, privacy, concurrency, UI contracts |

Baseline verification before planning edits:

```text
DB_PATH=<temporary sqlite> npm test
12 passed, 0 failed
```

No runtime database, environment file, production service, or hardware was
modified during the audit.

## Architecture decisions

1. `prop.md` is approved in full; no MVP scope reduction.
2. Server remains canonical; browser and ESP32 own no persistent domain truth.
3. New API seams are `/api/v1/admin` and `/api/v1/display`.
4. Existing family-ops routes/contracts remain stable through a tested cutover.
5. Ledger/audit/history are append-only; deletion is archival where practical.
6. Parent and panel authorization are separate; a UI-only PIN is invalid.
7. Display responses are allowlists and media uses panel derivatives.
8. Firmware loads child content on demand and retains only the active screen.

## Requirement-to-evidence register

| Requirement group | Planned evidence | Status |
|---|---|---|
| Children and independent schedules | migration fixtures + scenario A/B | Phase 2 domain PASS |
| Tasks, approvals, immediate stars | invariant/API/UI tests | Phase 2 domain PASS; API/UI pending |
| Daily and all term types | domain matrix tests | Phase 2 calculations PASS; broader matrix pending |
| Reward lifecycle/redemption | atomicity and replay tests | Phase 2 domain PASS; concurrency/API pending |
| Rules/incidents/consequences/recovery | lifecycle + preservation tests | Phase 2 domain PASS; API/UI pending |
| Admin API and web management | route auth tests + browser checklist | Phase 3 API PASS; Phase 4 UI/browser PASS; Aikido pending |
| Display API privacy/compactness | forbidden-key and byte-budget tests | Phase 5 PASS |
| Panel Child Focus and views | firmware builds + hardware screenshots | Pending |
| Media originals/derivatives/cache | adversarial upload + asset inspection | Phase 3 pipeline PASS; Phase 5 panel delivery/cache PASS |
| Audit history | action coverage + child-route negative tests | Parent API and display exclusion PASS |
| ESP32 memory/lifecycle | repeated-nav heap/PSRAM/LVGL evidence | Pending |
| CAO learning outcomes | three natural-use sessions per child | Pending |

## Verification scenarios

The final suite must include every scenario from the pasted execution brief:

- Three-year-old: three attended days, two successful, absent days excluded,
  final-day term readiness, parent approval required.
- Two-year-old: configurable two-day then three-day fixed term; no seven-day
  assumption; correct progress representation.
- Duplicate task approval creates one award.
- Repeated/concurrent reward approval creates one deduction atomically.
- Recovery preserves the incident and can restore configured progress.
- Reversal links to the original, recalculates balance, and rejects repetition.
- Display endpoints exclude parent notes, audit/admin metadata, and sibling
  discipline history.
- Screen navigation releases inactive state/assets and does not progressively
  consume memory.

## Work log

### Planning audit

- Read the complete pasted execution brief and `docs/prop.md`.
- Audited server, schema/migration approach, API/auth, web UI, firmware display
  model, media baseline, tests, and existing V1 plans/gates.
- Ran the existing API suite against an isolated temporary database: PASS 12/12.
- Completed Matrix board review across CHRO, CPO, COO, CTO, CISO, CFO, and CAO.
- Wrote the approved planning package. No source implementation started.

### Phase 1 — persistence and parent auth foundation

- Added ordered SQL migrations with SHA-256 checksums, per-migration
  transactions, drift detection, unknown-version rejection, and an auditable
  `schema_migrations` ledger.
- Added the complete approved reward/discipline persistence model, including
  audit and domain-idempotency tables, child-specific terms, independent daily
  and term progress, append-only ledger relationships, and history-preserving
  foreign keys.
- Chose `children.id` as a child-specific extension of `family_members.id` so
  existing member, chore, seed, and dashboard identities remain compatible.
  Existing chores were deliberately not reinterpreted as the richer task model.
- Added local-only PIN provisioning using salted `scrypt`; no plaintext PIN is
  persisted or accepted as a command argument.
- Added opaque parent sessions with hashed tokens/CSRF secrets, idle and absolute
  expiry, PIN-version revocation, exact-origin checks, strict cookies, CSRF
  enforcement, and persistent exponential throttling.
- Added `/api/v1/auth/login`, `/api/v1/auth/session`, and
  `/api/v1/auth/logout`. Full `/api/v1/admin` resources remain a later phase;
  legacy APIs and panel token behavior were preserved.

Verification:

```text
Disposable init: migrations 001 and 002 applied
SQLite integrity_check: ok
SQLite foreign_key_check: 0 findings
npm test with disposable DB: 26 passed, 0 failed
npm audit --omit=dev --offline: 0 vulnerabilities
JavaScript syntax checks: pass
```

Coverage includes fresh and existing-shaped databases, no-op rerun, migration
rollback, baseline-row preservation, child schedule representability, invalid
orphan/cost/duplicate constraints, PIN hashing, origin enforcement, opaque
cookie sessions, token rotation/tampering/expiry, CSRF, logout replay, PIN-change
revocation, capability separation, and persistent throttling.

The Phase 1 completion audit additionally proved every approved term type and
daily status can persist without a seven-day assumption, installed database
guards against ledger/audit rewrites or deletes, preserved incident/history
rows, and confirmed that recording discipline has no automatic star effect.

### Phase 2 — domain services and invariant tests

- Added approved additive migration `003_phase2_domain_contract.sql`. It extends
  task snapshots/rejections, term qualification and release timing, reward goal
  and redemption attribution, domain request fingerprints, child-scoped rules,
  consequence resolution, and structured recovery actions. It does not alter
  or apply against the runtime database.
- Added transactional domain primitives for request fingerprints, actor-bound
  idempotency replay, atomic rollback, and append-only audit events.
- Added star ledger, task completion, term/day, reward/redemption, and
  incident/consequence/recovery services. Discipline never changes stars;
  recovery progress requires explicit configuration and parent approval.
- Hardened the legacy API test harness so it always creates and removes a
  temporary SQLite database instead of using `server/data/family-hub.sqlite`.

Verification:

```text
Migration 003 isolated test: pass
Phase 2 domain invariant groups: 6 passed, 0 failed
Full server suite, disposable DBs: 33 passed, 0 failed
npm audit --omit=dev --offline: 0 vulnerabilities
JavaScript syntax checks: pass
```

The invariant suite covers actor/payload-bound idempotency, one-time task award,
edited approval values, rejection without stars, exact reversal linkage,
three-attended-day and two-fixed-day scenarios, absence exclusion, independent
term lengths, pause/resume/extend/reset history, exact-balance redemption,
one-time spendable deduction, milestone non-deduction, and approved recovery
that preserves the incident and star balance.

### Phase 3 — parent-protected administrative API and media pipeline

- Added additive migration `004_phase3_admin_contract.sql` for routine groups,
  task exceptions/resolution, richer rewards/terms/consequences/display settings,
  and versioned media lineage. It was applied only to disposable test databases.
- Added the complete `/api/v1/admin` resource and action surface for overview,
  children, tasks/routines, task approvals, rewards, terms/day status, reward
  requests, rules, incidents, consequences, recovery, star transactions,
  display settings, audit history, and media.
- Bound every administrative route to the opaque parent session. Mutations also
  require trusted origin and CSRF, and derive audit/idempotency identity from the
  authenticated session rather than caller-supplied actor fields.
- Added validated private media ingestion for PNG/JPEG/WebP with content/type
  agreement, bounded asynchronous ImageMagick processing, private originals,
  stripped panel/thumbnail WebP derivatives, version lineage, containment checks,
  and authenticated thumbnail delivery.
- Added consistent domain/validation error handling and production secure-
  transport enforcement for parent authentication and administration.

Verification:

```text
Full server suite with disposable databases/media: 40 passed, 0 failed
npm audit --omit=dev --offline: 0 vulnerabilities
JavaScript syntax checks: pass
Focused manual security/media review: pass with limitations below
```

No live database, environment file, deployed service, or hardware was changed.

### Phase 4 — parent web UI

- Added a `Rewards & Behavior` parent workspace to the existing vanilla
  HTML/CSS/JavaScript shell without replacing or removing Home, Grocery,
  Chores, Dinner, Notes, or Setup.
- Added parent PIN login/session/logout handling, browser-readable double-submit
  CSRF support, same-origin credentialed requests, explicit unauthorized/loading/
  error/success states, and no use of the legacy write token for admin routes.
- Implemented all nine planned parent areas: Overview, Children, Tasks &
  Routines, Approvals, Rewards, Reward Terms, Rules & Discipline, Star History,
  and Display Settings.
- Added structured child attendance, task assignment/schedule/exception, routine
  composition, reward availability/access/order, term lifecycle/day status,
  discipline/recovery, ledger reversal/correction, media, and panel-setting
  controls. Parent notes remain inside the authenticated discipline workspace.
- Added responsive stacking, visible keyboard focus, arrow-key primary tabs,
  non-color status labels, reduced-motion handling, confirmation for archival/
  lifecycle/ledger actions, and visual-only previews clearly separated from the
  future authoritative display API.
- Closed Phase 4 read-model seams without a schema change: term-day history,
  complete overview attention queues, child avatar round-trip, joined star
  ledger rows, and server-derived historical balance-after values.
- Fixed the CSRF cookie path discovered by real-browser verification: the
  HttpOnly session cookie remains scoped to `/api/v1`, while the non-secret
  CSRF cookie is scoped to `/` so the web shell can echo it on mutations.
- Prevented the legacy 30-second household-dashboard poll from replacing an
  in-progress parent administration form, and corrected task approval labels
  to resolve assignment IDs through their owning task definition.

Verification:

```text
Full server + UI suite with disposable databases/media: 45 passed, 0 failed
Headless Chromium: parent login + all nine areas + representative writes PASS
Rendered sizes: 1440x900 and 390x844; no document overflow; no console errors
Legacy Family Hub API regression suite: PASS
npm audit --omit=dev --offline: 0 vulnerabilities
JavaScript syntax and focused dangerous-pattern checks: pass
```

Rendered screenshots were inspected from temporary files only. Browser tests
created and removed their own SQLite database, media root, Chromium profile,
and localhost server process.

### Phase 5 — display API and media delivery

- Added a separate `/api/v1/display` contract without changing the legacy
  dashboard API or firmware renderer. The new surface has home, child, task,
  reward, progress, active-correction, aggregate child-mode, action, and panel
  media routes.
- Added a fail-closed, header-only `PANEL_TOKEN` capability boundary. The
  broader write token and query-string credentials are rejected, and an
  unconfigured panel capability returns service unavailable.
- Built explicit child-facing DTOs with active/screen-visible child checks,
  independently child-bound nested queries, deterministic collection limits,
  byte budgets, compact sync/staleness metadata, allowed-action booleans, and
  no raw database-row or domain-service response exposure.
- Added server-authoritative, idempotent task completion, reward selection,
  reward request, and incident acknowledgement actions. Display settings are
  enforced again on mutation, and cross-child targets are rejected.
- Added exact-version panel WebP delivery with immutable private caching,
  explicit ETags/304 responses, nosniff/same-origin headers, active-asset
  checks, and canonical containment under the panel derivative directory.
- Froze the firmware-facing v1 contract in
  `server/tests/fixtures/display-contract-v1.json` before any Phase 6 firmware
  modification.

Verification:

```text
Phase 5 display contract/API/media tests: 4 passed, 0 failed
Full server + UI suite with disposable databases/media: 49 passed, 0 failed
npm audit --omit=dev --offline: 0 vulnerabilities
JavaScript syntax checks: pass
Focused privacy/capability/media review: pass with Aikido gap below
```

Tests cover every frozen display endpoint, required offline metadata, recursive
forbidden-key exclusion, sibling/profile isolation, action idempotency,
cross-child rejection, disabled-action enforcement, reward selection/request,
header-only capability separation, exact media versions, immutable cache/ETag,
and adversarial private-path substitution. No live database, deployed service,
environment file, or hardware was changed.

## Current blockers / decisions

- Live database migration remains separately gated; no runtime DB was migrated.
- **Project-wide scanner decision:** on 2026-07-17 Michael explicitly opted out
  of Aikido for the remainder of the project and accepted the accumulated
  offline dependency-audit and manual security-review gap. Every remaining
  phase still runs offline audit, syntax, focused dangerous-pattern/privacy
  review, and negative tests; none may claim automated SAST clearance.
- The shared household PIN authenticates a parent session, not an individual
  named parent account. Audit records identify the specific authenticated
  session but cannot name a human parent until multi-parent identity is added.
- Physical panel availability determines final hardware verification, not build
  or server/web progress.

## Deviations from `prop.md`

No product scope deviation. Two sequencing/compatibility decisions were made:

1. The parent auth foundation was implemented in Phase 1 under explicit CEO
   approval, ahead of the full administrative API phase.
2. `children` extends existing `family_members` identity, while legacy `chores`
   remains separate from the richer new task model.
