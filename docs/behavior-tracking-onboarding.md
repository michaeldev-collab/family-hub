# Behavior Tracking and Child Onboarding Architecture

Status: functional implementation complete; final hardware/browser/full-suite reruns are constrained as documented in section 16  
Owner: Michael / Family Hub  
Implementation agent: Codex with 3DL Matrix  
Last updated: 2026-07-18

## 1. Purpose and product language

This change adds two connected parent workflows without turning the Family Hub into a clinical or punitive system:

1. A neutral, append-only record of observed behaviors, developing skills, attempts, and recovery.
2. A guided child setup flow that keeps incomplete configuration off the child panel until one safe activation.

An observation says what happened. It is not automatically an incident, a diagnosis, a score, or a punishment. Incidents remain the separate corrective/safety workflow. Reward accounting remains a separate append-only ledger. Challenging behavior never subtracts already-earned stars automatically.

Parent-facing language uses child profile, schedule, behaviors and skills, routines, daily reward, larger reward, and child panel. Internal enum names and transport DTO names do not appear as setup concepts.

## 2. Selected 3DL Matrix skills and operating roles

The following installed skills were inspected and are being followed:

| Skill | Use in this implementation |
| --- | --- |
| `3dl-matrix` | Board routing, risk classification, ownership, and cross-discipline gate review. |
| `3dl-matrix-architecture` | Additive bounded-context design, contracts, migration invariants, and compatibility decisions. |
| `3dl-matrix-cpo-ux-mgr` | Parent journey, quick-log interaction, wizard language, accessibility, and control-center information architecture. |
| `3dl-matrix-iot` | Child/household firmware separation, bounded payload/state, offline behavior, image lifecycle, and memory verification. |
| `3dl-matrix-ai-workflows` | Repository-wide dependency tracing, file-disjoint audit delegation, and evidence-return discipline. |
| `3dl-matrix-build` | Spec -> Plan -> Execute -> Verify gates and incremental build loop. |
| `3dl-matrix-phased-engineering-plans` | Dependency-ordered, testable implementation phases. |
| `3dl-matrix-agent-build-plans` | Bounded server/web/firmware work packages with one contract owner. |
| `3dl-matrix-execute-planner-plan` | Plan status, phase completion criteria, and verifier handoffs. |

The Matrix board assigned CTO as primary owner, with CPO/COO/CISO/CHRO/CFO support. The local work is classified as a material Type 1 repository modification. No production migration, deployment, auth replacement, secret handling, package installation, or hardware flashing is authorized by this implementation.

## 3. Current-state audit

### 3.1 Repository and recovery baseline

The workspace contains three projects:

- `server/`: Express 4, CommonJS, Node 22+, and `node:sqlite`; server is the domain authority.
- `web/`: dependency-free HTML/CSS/JavaScript parent application.
- `firmware/`: PlatformIO/Arduino ESP32-S3 project with separate child and household launcher environments.

The workspace root is not a usable Git worktree. Same-day source archives are therefore the pre-change recovery checkpoint:

- `server.tar.gz`: SHA-256 `e26d19e0e5152f81122e406165d623f5e2458f3bd38989f65c159d76bfdc698b`
- `web.tar.gz`: SHA-256 `ca2fea90e39d3cde99306e5332f6f1d0bc35907847b8e88f19e0862280c43c6f`
- `firmware.tar.gz`: SHA-256 `06917d979c307f84f4837617e16b1cd47984021306b2d85fe9b472ceb29bbe02`

Archive comparison was clean after excluding secrets, runtime data, dependency folders, and build output. These archives are checkpoints, not a substitute for normal source control.

### 3.2 Existing server model

- Children extend family members and are currently written immediately as active records.
- Attendance schedules, tasks, routines, task approvals, rewards, reward terms, term days, goals, redemptions, display profiles, focus sessions, and media assets already exist.
- Corrective behavior is represented by reusable `behavior_rules`, child assignments, `behavior_incidents`, consequences, and recovery actions.
- The star ledger is append-only and already enforces idempotent task/reward/reversal accounting.
- Domain audit events and idempotency records are append-only.
- Migrations are ordered, checksum-locked, forward-only SQL files applied in `BEGIN IMMEDIATE` transactions. There are no down migrations.
- The parent API is authenticated by the existing parent session, with origin and CSRF checks on mutation routes.
- The display API uses separate device-bound capability authentication.
- Media upload validation and panel derivatives already exist.
- The deployed domain is intentionally single-household: there is one shared parent credential/session authority and no household table or role hierarchy.

### 3.3 Existing display and firmware model

- The compact display contract is version 1, revision 2.
- Child-mode payloads are capped at 24 KiB and list payloads at 12 KiB.
- The child launcher has bounded state, one LVGL screen root, lazy image loading, a shared image cache, logical idempotency keys, and a single pending-write guard.
- Child and household launchers are separate binaries and must remain separate.
- The child launcher already supports task completion requests, reward requests, corrections/recovery, and parent approval refreshes through the existing network stack.
- Offline mode uses last-known in-memory state, disables writes, and never fabricates server approval. Launcher builds do not currently persist a full child snapshot across reboot.

### 3.4 Existing web model

- Parent administration is one vanilla JavaScript shell with nine in-memory areas.
- Child creation writes immediately to live tables and exposes internal term configuration concepts.
- Behavior UI is correction-first (`rules`, incidents, consequences, recovery) and has no neutral observation path.
- The panel preview is visual-only rather than generated from a validated draft.
- API calls correctly include cookies, CSRF, and random idempotency keys for existing action routes.
- There is no route-aware wizard, saved draft, dirty-state protection, optimistic versioning, or activation recovery.
- The global refresh loads many resources after every mutation; the new areas need scoped loaders.

### 3.5 Baseline verification

Before edits:

| Check | Result |
| --- | --- |
| Server/web suite: `npm test` | PASS: 69 tests, 69 passed, 0 failed. Required normal localhost permissions because the sandbox blocks test listener binding. |
| Child launcher: `platformio run -e waveshare7b-child-launcher` | PASS: RAM 100,804 / 327,680 bytes (30.8%); flash 1,365,432 / 16,777,216 bytes (8.1%). |
| Household launcher: `platformio run -e waveshare7b-household-launcher` | PASS: RAM 97,116 / 327,680 bytes (29.6%); flash 1,252,576 / 16,777,216 bytes (7.5%). |

PlatformIO required access to its existing user package cache and lock files. No dependency was intentionally installed or upgraded.

## 4. Confirmed architectural gaps

1. There is no neutral definition/observation model, classification, prompt level, outcome, timeline, or descriptive summary.
2. Existing behavior rules encode correction and cannot safely become a general-purpose behavior definition.
3. There is no auditable amendment path for a neutral historical observation.
4. There is no explicit observation-to-ledger contribution record or exact-once behavior reward rule.
5. There is no child-originated skill request or parent approval queue.
6. The display DTO has no bounded skill goal state.
7. Child creation is immediately live and has no draft, resume, validation-by-step, or idempotent activation boundary.
8. The parent UI exposes implementation language and lacks a child-centered control center.
9. Household isolation is implicit in a single-household deployment, not modeled in tables. New private data needs explicit household keys without replacing stable auth.
10. Role-based parent authorization does not exist beyond the one authenticated household-parent authority. Adding roles is outside this change; all new parent endpoints inherit the current household-admin authority.

## 5. Final bounded-context model

```text
Household (current deployment: one server-owned default household)
├── Child
│   ├── Behavior-definition assignments
│   ├── Behavior observations (immutable facts)
│   │   ├── Amendments (append-only correction/archive facts)
│   │   ├── Optional incident link
│   │   └── Optional reward/goal contribution
│   ├── Child action requests (pending -> approved/rejected)
│   └── Existing tasks, rewards, terms, and panel profile
├── Behavior definitions (editable/deactivatable configuration)
└── Child onboarding drafts (mutable, versioned staging state)

Behavior incident (existing corrective/safety context)
├── Consequence
└── Recovery action

Star ledger (existing reward-accounting context)
└── Optional behavior-observation contribution link
```

### 5.1 Household compatibility boundary

The migration creates a server-owned default household and an explicit child-to-household mapping for existing children. Existing authentication remains unchanged and resolves only to that default household. New services never accept a client-supplied household id; it comes from the authenticated server context. Composite foreign keys and service checks prevent a definition, child, observation, incident, or draft from crossing that boundary.

This is a compatibility isolation layer, not a claim that the application is now a full multitenant identity platform. A future auth project can bind sessions to modeled households without rewriting observation history.

### 5.2 Behavior definitions

`behavior_definitions` is mutable configuration, not history. Implemented fields:

- `id`, `household_id`
- `name`, `description`, `preset_group`, `category`
- `classification_default`
- `icon`, `image_asset_id`
- `active`
- `default_star_value`, `reward_eligible`, `reward_classifications_json`
- `supports_prompt_level`, `supports_duration`, `supports_intensity`
- `can_create_incident`
- `child_visible`, `parent_only`, `goal_eligible`
- `created_at`, `updated_at`

`behavior_definition_children` assigns a definition to selected children and carries child-specific overrides for activation, reward eligibility/value, goal visibility/target, and child-panel visibility. Deactivation stops new use but never deletes prior observations.

### 5.3 Behavior observations

`behavior_observations` is append-only. Implemented fields:

- Identity/scope: `id`, `household_id`, `child_id`, `behavior_definition_id`
- Fact: `classification`, `occurred_at`, `context`, `antecedent`, `observable_behavior`
- Support: `prompt_level`, `intensity`, `duration_seconds`
- Response: `response_type`, `response_description`, `outcome`, `effective`
- Accounting snapshot: `star_value_awarded`, `goal_contribution`
- Optional relationship: `incident_id`
- Provenance: `recorded_by`, `source`, `idempotency_key`
- Timestamps: `created_at`, `updated_at` (equal at insertion; no in-place historical edit)

Classifications:

- `positive_independent`
- `positive_prompted`
- `skill_attempt`
- `skill_progress`
- `recovery`
- `challenging`
- `safety`

Prompt values:

- `independent`, `gestural`, `verbal`, `modeled`, `partial_physical`, `full_assistance`, `not_applicable`

Outcome values:

- `successful`, `partially_successful`, `resolved_immediately`, `resolved_after_prompt`, `resolved_after_support`, `recovered`, `continued`, `escalated`, `unknown`

Sources:

- `web`, `child_firmware`, `system`, `import`

Database triggers reject updates and deletes. `behavior_observation_amendments` records a correction, annotation, or archive decision with a per-observation `sequence_no`, reason, actor, timestamp, and canonical validated replacement fields. Historical corrections are validated against the recorded classification rather than later mutable definition settings. The original observation remains readable for audit. Parent APIs return the original plus amendment history/effective view.

### 5.4 Reward and goal contributions

`behavior_observation_contributions` links one observation to at most one existing star transaction and stores the explicit goal contribution. `behavior_reward_links` configures daily/larger eligibility per assigned child, and `behavior_observation_reward_contributions` records each applied reward scope. Daily and larger progress use separate non-negative term-day accumulators. The server evaluates all contributions inside the same database transaction that inserts the observation.

V1 reward rules:

1. Definition must be reward eligible.
2. Child assignment must permit the reward and supplies any child override.
3. Classification must be in the definition's configured qualifying set. The safe preset default is only `positive_independent`.
4. Award is zero or positive; challenging/safety observations never create deductions.
5. Observation and ledger contribution are protected by household/source/idempotency uniqueness and a unique contribution link.

Thus a retried eligible observation contributes exactly once, a prompted attempt defaults to zero stars, and already-earned progress remains intact.

Tasks follow the same separation. A task definition and child assignment can configure independent daily and larger-reward progress values. A completion request snapshots both effective values, and approval applies each exactly once to its own term-day accumulator. The migration mirrors legacy daily values into the new larger value for existing records so current children do not lose progress; newly configured tasks can set either scope to zero.

Reward approval policy is enforced, not merely stored. Daily, spendable, and milestone rewards use the reward definition's approval setting; a term reward uses the active term's approval setting. When approval is disabled, the request is approved atomically, including an exact-once spendable balance deduction. No challenging observation is part of this decision.

Term reset preserves the configured strategy, approval/reset policy, window span, and enabled state while rebasing date windows. It refuses to reset a term whose linked larger-reward goal is already ready or requested, and it relinks or creates the appropriate next goal without fabricating progress.

### 5.5 Incidents

An observation may reference an existing incident only when household and child match. Creating an observation does not create an incident by default. Parent UI can explicitly link or start the existing incident workflow for challenging/safety events. Existing historical incidents are not rewritten and no historical observations are fabricated.

### 5.6 Presets

Presets are a server-owned registry exposed by API, not hardcoded firmware data. Selecting a preset creates an editable household definition. Groups include:

- Self-care: potty independently, tried potty, washed hands, brushed teeth, dressed, shoes, ate independently.
- Social/emotional: kind hands, shared, turns, asked for help, words instead of hitting, transition, recovery, waiting.
- Routines/responsibility: cleanup, morning, bedtime, followed direction, household help.
- Challenging/safety: hitting, throwing, screaming, biting, running away, unsafe climbing, refusal, aggressive grabbing.

Recommended age defaults affect grouping and suggested configuration only. They make no diagnostic or medical claim.

## 6. Database migration strategy

Three forward migrations keep review and rollback boundaries small:

### Migration 006: behavior observation domain

- Create `households` and seed the server-owned default row.
- Create `child_households` and map every existing child without changing child visibility or activity.
- Create behavior definitions, child assignments, daily/larger reward links, observations, ordered amendments, star/goal contribution links, per-reward-scope contribution facts, and child action requests.
- Add separate daily and larger behavior-progress accumulators to existing reward-term days so one observation can contribute explicitly to either or both without double-counting.
- Add composite scope foreign keys, child/incident validation triggers, enum/check constraints, append-only triggers, and retry uniqueness.
- Add indexes for household, child + occurred time, classification + occurred time, definition + occurred time, source, incident, and action-request status.

### Migration 007: onboarding drafts

- Add nullable `birth_date` and safe legacy defaults only where required.
- Create `child_onboarding_drafts` with canonical JSON, schema version, completed steps, current step, optimistic `version`, status, activation idempotency key, activated child id, and timestamps.
- Index household/status/update time and unique household/client idempotency keys.

### Migration 008: reward-scope progress and panel switching

- Add task-definition and child-assignment larger-reward progress configuration, plus an immutable effective larger-progress snapshot on task completion requests.
- Add a separate task larger-progress accumulator to reward-term days. Backfill existing task defaults, explicit overrides, requests, and term-day values from their legacy daily equivalents so existing reward behavior is preserved.
- Add a dedicated `profile_switching_enabled` display setting. Its safe default is enabled, matching existing firmware behavior; it is distinct from the existing single-child picker shortcut.

The repository does not support down migrations. Rollback is restore-from-checkpoint before applying to a live database. Migration tests use fresh temporary databases and an upgrade fixture representing migration 005. No live database is modified by automated verification.

## 7. API contract changes

All parent routes live under the existing authenticated `/api/v1/admin` router and inherit session, origin, CSRF, validation, body-size, and safe-error middleware. Mutating retryable actions require `Idempotency-Key` following existing conventions.

### 7.1 Definitions and assignment

- `GET /behavior-presets`
- `GET /behavior-definitions?childId=&active=&category=&classification=`
- `POST /behavior-definitions`
- `PATCH /behavior-definitions/:id`
- `POST /behavior-definitions/:id/deactivate`
- `PUT /behavior-definitions/:id/children/:childId`
- `DELETE /behavior-definitions/:id/children/:childId` (deactivates assignment; it does not delete history)

### 7.2 Observations

- `GET /behavior-observations?childId=&definitionId=&classification=&from=&to=&source=&incidentLinked=&rewardEligible=`
- `POST /behavior-observations`
- `POST /behavior-observations/quick-log`
- `GET /behavior-observations/:id`
- `POST /behavior-observations/:id/amendments`
- `POST /behavior-observations/:id/archive`
- `GET /behavior-analytics?childId=&from=&to=&definitionId=`

Quick log requires only `childId`, `behaviorDefinitionId`, `classification`, and `occurredAt`. Prompt level, intensity, context, antecedent, outcome, note/response description, duration, and incident id are optional. The response includes the observation, any explicit contribution, and replay metadata for an idempotent retry.

Analytics returns descriptive counts by definition/classification, independent-versus-prompted, positive-versus-challenging, hour-of-day, day-of-week, average duration, common outcomes, goal progress, and period trend. It contains no diagnostic language.

### 7.3 Child action requests

- `GET /behavior-action-requests?childId=&status=`
- `POST /behavior-action-requests/:id/approve`
- `POST /behavior-action-requests/:id/reject`
- `POST /api/v1/display/actions/request-skill-observation`

The display action accepts only a child id and an allowed goal/definition id plus a logical idempotency key. It creates a pending request. It cannot submit challenging/safety classifications or parent notes. Approval creates the observation and contribution transactionally; rejection creates no observation.

### 7.4 Onboarding and control center

- `GET /child-onboarding-drafts`
- `POST /child-onboarding-drafts`
- `GET /child-onboarding-drafts/:id`
- `PATCH /child-onboarding-drafts/:id`
- `PUT /child-onboarding-drafts/:id/steps/:step`
- `POST /child-onboarding-drafts/:id/validate`
- `GET /child-onboarding-drafts/:id/panel-preview`
- `POST /child-onboarding-drafts/:id/activate`
- `GET /children/:id/control-center`

Draft updates use optimistic `version` checks. Activation accepts a stable idempotency key, validates all required steps, and materializes the child plus selected configuration in one transaction. A replay returns the same activated child. An incomplete draft is not a child and cannot enter any display query.

Panel preview returns the effective shared-device configuration, including independent `requirePinExit` and `requirePinChangeChild` flags, the effective profile-switch policy, reward visibility, and whether an existing default child is preserved. It does not imply that one PIN setting protects an action governed by the other.

### 7.5 Additive task, reward, and panel contracts

- Task definitions accept `defaultLargerProgress`; child assignments accept `largerProgressOverride`. Blank parent input sends explicit `null` to clear an override, while omission remains compatible with older clients.
- Task completion requests snapshot daily and larger progress separately, and approval applies both exactly once.
- Reward requests auto-approve only when the authoritative definition/term policy disables approval. Spendable balance deduction remains transactional and idempotent.
- Display settings accept `profileSwitchingEnabled`; this is not an alias for `skipChildSelectionSingle`.

## 8. Child display contract and firmware

Display contract version remains `v1`; additive revision becomes 3. Older child firmware continues to use revision-2 fields and ignores additions. Revision-3 `child-mode` may include:

```json
{
  "skillGoals": [
    {
      "id": "definition-id",
      "title": "Kind hands",
      "mediaRef": "/bounded-panel-asset",
      "progress": 1,
      "target": 3,
      "state": "available",
      "allowedAction": "request-observation"
    }
  ],
  "recentSkillCelebration": null
}
```

Invariants:

- Maximum three goals; zero is valid.
- Only positive, developing-skill, or recovery goals may be present.
- No observation history, sibling data, incident detail, antecedent, context/note, recorded-by identity, analytics, or admin metadata.
- Existing 24 KiB child-mode budget remains enforced.
- Goal image uses the existing panel derivative and image cache; only the active goal image is held by the goal screen.
- Existing transport, device auth, request signing, logical idempotency, pending-write guard, lifecycle, and shared components are reused.
- States: `available`, `requested`, `waiting`, `approved`, `celebrating`, `completed`, `offline`. Repeated taps while waiting are ignored.
- `modeConfig.profileSwitchingEnabled` controls whether the child launcher creates or accepts the profile-switch action. It is independent of the default-child and skip-picker settings.
- Approved, scheduled, redeemed, and completed reward states project as completed/approved child-facing state; internal parent workflow detail is not exposed.
- Offline shows last-known safe progress, disables writes, and never creates a local approval.

Memory instrumentation records free heap/PSRAM around payload fetch, image load, goal-screen construction, request submission, and return home. Verification repeats navigation 25 times and checks for progressive loss. Hardware behavior remains unverified until a physical-panel smoke test.

## 9. Guided onboarding architecture

The default is Quick setup; Advanced setup reveals optional configuration without changing the canonical draft. The server draft is authoritative. The browser keeps a scoped working copy with dirty, saving, saved, validation-error, conflict, and activation states.

Hash routes fit the existing dependency-free web application:

- `#rewards/onboarding`
- `#rewards/onboarding/:draftId/profile`
- `#rewards/onboarding/:draftId/schedule`
- `#rewards/onboarding/:draftId/behaviors`
- `#rewards/onboarding/:draftId/routines`
- `#rewards/onboarding/:draftId/daily_reward`
- `#rewards/onboarding/:draftId/larger_reward`
- `#rewards/onboarding/:draftId/panel`
- `#rewards/onboarding/:draftId/review`

### Step 1: Profile

Name, photo/avatar, birth date or age, identity color, and future panel visibility. Only the name is required for quick setup. Creating/saving this step does not create a child.

### Step 2: Schedule and reward period

Parent choices are Every day, After attended days, Calendar week, or Custom days. The adapter maps these labels to existing term strategy fields. Attendance days, length, missed-day behavior, and reset behavior receive safe defaults. Invalid or internally contradictory configurations block Next with field-linked errors.

### Step 3: Behaviors and skills

Grouped presets plus custom definitions. Parents choose positive/developing/challenging/safety/recovery language, image/icon, prompt support, explicit reward value/eligibility, goal contribution, child visibility, and incident eligibility. Challenging presets default to parent-only, zero stars, and no panel visibility.

### Step 4: Tasks and routines

Morning, bedtime, potty, cleanup, leaving home, and mealtime templates. Parents can remove, reorder, change images, set frequency/stars, and choose parent approval. Templates materialize through adapters into existing task/routine models.

### Step 5: Daily reward

Name, image, required progress, available days, parent approval, and optional eligible behavior contribution. Validation updates the review model and preview.

### Step 6: Larger reward

Name, image, parent-facing period choice, target, eligible tasks/behaviors, parent approval, and reset behavior. Default policy preserves earned stars and never makes challenging behavior an automatic blocker.

### Step 7: Child panel

Enabled, default child, profile switching, PIN requirement, visible tabs/goals, task/reward visibility, sounds, animations, and refresh interval. Preview is generated from the validated draft and merged through the same shared-device preservation rules used during activation. It is functional effective state, not a separate mock DTO.

### Step 8: Review and activate

Concise summaries for all seven sections with Back, Save draft, and Activate. Activation is single-flight in the browser and idempotent/transactional on the server. A validation failure keeps the draft intact and routes to the first invalid step. A transport retry cannot duplicate the child.

Navigation preserves state. `beforeunload` protection appears only when a local change is unsaved. Forms use semantic labels/fieldsets/buttons, focus the new step heading, announce save/error state through a restrained live region, and remain single-column usable at mobile widths.

## 10. Post-onboarding child control center

Activated children use child-centered routes rather than replaying the wizard:

- `#rewards/children/:childId/overview`
- `#rewards/children/:childId/behaviors`
- `#rewards/children/:childId/tasks`
- `#rewards/children/:childId/rewards`
- `#rewards/children/:childId/schedule`
- `#rewards/children/:childId/panel`
- `#rewards/children/:childId/history`
- `#rewards/children/:childId/settings`

Overview uses one scoped aggregate endpoint and shows current reward period, today's task/progress state, positive observations, developing-skill progress, challenging observations, current reward, pending child requests, and panel sync state. Existing active children open here immediately and do not require onboarding.

## 11. Web behavior tracking

The parent admin adds scoped behavior routes and loaders rather than expanding the global 17-resource refresh:

- Quick log: child -> recent/favorite definition -> classification -> optional prompt/outcome -> save. Submitting is single-flight and retains retry identity until the response resolves.
- Timeline: time, child, definition, parent-friendly classification, prompt, outcome, incident link, and explicit reward contribution with accessible filters.
- Definitions: create/edit/deactivate, child assignments, image/icon, reward eligibility, goal visibility, and incident eligibility.
- Analytics: existing CSS/HTML/SVG primitives for descriptive cards/bars; no analytics dependency is added.

Private free text is never rendered in child preview or sent to display endpoints. API errors preserve field-level validation details. Optional field clearing uses explicit null semantics instead of the existing generic empty-value cleaner.

## 12. Compatibility decisions

1. Existing children remain active and visible exactly as configured; they are treated as legacy-complete and need no draft.
2. Existing incidents, consequences, recovery actions, tasks, rewards, terms, ledger entries, and audit records are unchanged.
3. No historical incidents are converted into observations.
4. Existing behavior rules remain the correction model. Definitions are a new model with adapters only where the UI offers an explicit incident link.
5. Existing display v1/revision-2 clients continue to function; revision 3 is additive and feature-checked.
6. Existing parent and device authentication are reused. Parent roles and generalized multitenancy are not introduced.
7. No new web, server, chart, or firmware dependency was added.
8. Media uses existing validated assets and child-safe derivatives.
9. Incomplete onboarding lives only in draft tables and cannot appear on the panel.
10. Existing administrative star corrections remain available; the new behavior path itself never deducts stars.
11. Migration 008 mirrors legacy task progress into both scopes and enables profile switching by default; the new split changes behavior only when a parent explicitly configures different values.

## 13. Security and privacy review

Risk classification is R2 because the feature stores sensitive child behavior data and permits child-device writes.

Controls:

- Parent endpoints require the existing authenticated parent session; mutations retain origin and CSRF enforcement.
- Display writes require the existing device-bound signed capability and server-side child authorization.
- Household id is derived by the server and never accepted from request bodies or query strings.
- All definition/child/incident lookups validate one household and, where relevant, the same child.
- Observation history is immutable; amendments and archives are append-only facts.
- Child action classifications are server-owned and allowlisted; challenging/safety self-report is rejected.
- Idempotency scope includes household, source/actor, action, and stable key; replay with a different request hash is rejected.
- Logs include ids/status but exclude context, antecedent, behavior descriptions, notes, and parent identity detail.
- Child DTO allowlists exclude private text, sibling data, incidents, analytics, and audit metadata; negative tests inspect serialized payloads.
- Existing request-size, media validation, session, CSRF, and secure-parent-transport controls stay in place.

Aikido tooling is not available in this Codex environment. Existing project documentation records Michael's 2026-07-17 decision to accept the manual/offline gap for the remainder of this local phase. Security verification therefore uses dependency-free static inspection, route/auth tests, negative privacy tests, idempotency tests, migration constraints, and existing suite coverage. This is not an Aikido-clear claim.

## 14. Phased execution and verification gates

### Phase A: Server model and contracts

- Add migrations and upgrade/fresh-database tests.
- Add definition, observation, contribution, analytics, child-request, draft, activation, and control-center services.
- Add authenticated parent/display routes and validation.
- Gate: syntax checks, focused unit/integration tests, full `npm test`, schema/index/trigger inspection, and diff review.

### Phase B: Firmware

- Add revision-3 filter/state, bounded goal UI, request transport, waiting/approval/rejection/offline behavior, and memory instrumentation.
- Gate: child launcher build, household launcher build if shared code changes, size comparison, static private-field search, and 25-navigation test plan. Physical hardware remains separately reported.

### Phase C: Web behavior tracking

- Add API client contracts, hash-routed quick log/timeline/definitions/analytics, scoped state, validation, and accessibility.
- Gate: JavaScript syntax checks, focused DOM/API tests, full server/browser suite, responsive/static review.

### Phase D: Onboarding and control center

- Add wizard shell, all eight real steps, server draft resume/versioning, draft preview, atomic activation, and child-center sections.
- Gate: all required onboarding scenarios, retry/failure injection, existing-child compatibility, mobile/keyboard checks, and full suite.

### Phase E: Independent verification

- Inspect complete diff against this contract.
- Run Matrix critic and verifier passes with separate evidence.
- Fix root causes, rerun the smallest failed check, then rerun the full affected suite.
- Record final commands, results, artifact paths, memory/size deltas, risks, and unverified hardware behavior below.

## 15. Required scenario ledger

The implementation is not complete until automated or explicitly documented manual evidence covers:

- All seven observation classifications, including prompted/independent distinction and recovery.
- Challenging observation both without and with a same-child incident.
- Zero automatic star deduction and exactly-once eligible positive contribution.
- Duplicate firmware request rejection/replay, approval, rejection, and network-loss behavior.
- Cross-household/cross-child access rejection and private-field absence from display JSON.
- Date/child/definition/source/incident/reward filters and correct summary counts.
- Definition deactivation with retained history and append-only amendment/archive behavior.
- Zero/one/three bounded goals, missing image fallback, and repeated navigation memory stability.
- Draft create/save/leave/resume, presets/custom definition, routine templates, both rewards, panel preview, review/activate, injected activation failure, retry without duplicate child, and draft invisibility.
- Existing-child compatibility, back-navigation persistence, invalid-term blocking, and mobile usability.

## 16. Build/test results and implementation ledger

### 16.1 Implementation status

- Migrations 006-008, behavior services/routes, onboarding services/routes, task/reward policy adapters, display revision 3, the parent behavior UI, all eight onboarding steps, the child control center, and bounded child firmware support are implemented.
- Activation materializes one child and its selected attendance, definitions, assignments, routines/tasks, rewards, active term/goals, and panel profile in one SQLite transaction. Failure injection proves rollback; stable replay identity proves retry does not duplicate the child.
- Daily and larger task/behavior progress are separate immutable contribution paths. Eligible positive behavior and approved tasks contribute exactly once; challenging behavior never subtracts stars.
- The child contract contains at most three safe goals and never includes behavior history or parent free text. Child taps create pending requests, never self-approved challenging observations.

### 16.2 Verification evidence

| Scope and command | Result |
| --- | --- |
| Baseline `cd server && npm test` | PASS: 69/69 before edits. |
| Integrated `cd server && npm test` during implementation | PASS: 90/90 on the pre-final-critic snapshot, including authenticated admin/device API and Chromium smoke coverage. |
| Final changed JavaScript `node --check` sweep | PASS: 17 server/route/web files. |
| `node tests/domain-services.test.js` | PASS: 19/19 on final source. |
| `node tests/behavior-tracking.test.js` | PASS: 11/11 on final source. |
| `node tests/onboarding.test.js` | PASS: 8/8 on final source. |
| `node tests/migrations.test.js` | PASS: 12/12 on final source, including fresh and migration-005 upgrade paths through migration 008. |
| `node tests/web-admin.test.js` | PASS: 11/11 on final source. |
| Final non-listening focused total | PASS: 61/61. |
| `platformio run -e waveshare7b-child-launcher` | PASS after all firmware source changes: RAM 30.8%, flash 8.2%; binary 1,375,776 bytes. |
| `platformio run -e waveshare7b-household-launcher` | PASS: RAM 97,116 bytes (29.6%), flash 1,252,576 bytes (7.5%); later edits were child-only/excluded from this environment. |
| `platformio run -e devkit` | PASS during the implementation pass: RAM 44,876 bytes; flash 813,993 bytes. |
| Firmware allowlist/bounds inspection | PASS: goal filter contains only child-safe fields; validation/render/action paths cap goals at three. |
| Independent server, web, and firmware critic passes | PASS after repair: no remaining P0/P1 findings. |

The final-source listener/browser-integrated `npm test` command was not rerun after the last focused repairs because those suites require localhost listener permission and the environment's elevated-execution quota was exhausted. The last full integrated run is therefore the 90/90 pre-final-critic snapshot; every file changed afterward is covered by the final 61-test focused sweep and syntax checks. A current Chromium visual run remains an explicit handoff item.

Build artifacts:

- `firmware/dist/family-hub-child.bin` — 1,375,776 bytes; SHA-256 `d1f0329605972006de6076e6751c10a55fc7df0cfc73b548b97aa11c32f7429f`. The baseline artifact was 1,365,904 bytes, a 9,872-byte increase.
- `firmware/dist/family-hub-household.bin` — 1,252,944 bytes; SHA-256 `ac9780f69d8aeb09ccc0f932cd5efa80698bd90dd8dd827fc612b75070a80691`. It is unchanged from the recovery checkpoint.

Physical child-panel input, image fallback on the actual display, network interruption, approval/rejection refresh, and 25 repeated navigation cycles with heap/PSRAM observation could not be verified without hardware.

### 16.3 Files changed

Server:

```text
server/db/migrations/006_behavior_observation_domain.sql
server/db/migrations/007_child_onboarding_drafts.sql
server/db/migrations/008_reward_scope_progress.sql
server/routes/admin-rewards-behavior.routes.js
server/routes/display-rewards-behavior.routes.js
server/services/adminResources.js
server/services/behaviorTracking.js
server/services/displayState.js
server/services/domainCore.js
server/services/onboarding.js
server/services/rewardAvailability.js
server/services/rewardTerms.js
server/services/rewards.js
server/services/taskCompletions.js
server/tests/admin-api.test.js
server/tests/behavior-tracking.test.js
server/tests/display-api.test.js
server/tests/domain-services.test.js
server/tests/fixtures/display-contract-v1.json
server/tests/migrations.test.js
server/tests/onboarding.test.js
server/tests/web-admin.test.js
server/tests/web-browser-smoke.test.js
```

Web:

```text
web/css/rewards-admin.css
web/index.html
web/js/app.js
web/js/rewards-admin.js
web/js/rewards-api.js
web/js/rewards-behavior.js
web/js/rewards-child-center.js
web/js/rewards-onboarding.js
```

Firmware and integration record:

```text
firmware/include/api_client.h
firmware/include/child/child_api.h
firmware/include/child_focus_state.h
firmware/include/config.h
firmware/include/ui_manager.h
firmware/src/child/child_api_client.cpp
firmware/src/child/child_ui.cpp
firmware/src/child/main.cpp
firmware/src/child_focus_state.cpp
docs/behavior-tracking-onboarding.md
```

## 17. Known risks and current mitigations

- **No functional Git metadata:** use verified archives plus file-level diff commands; recommend restoring normal source control before production rollout.
- **Single-household auth:** explicitly scope new data to the server-owned household now; do not widen auth in this feature.
- **Large coordinated change:** server contracts were implemented before firmware/web consumers and reviewed through independent focused critic passes; a current full listener/browser rerun is still required before release.
- **SQLite forward-only migrations:** test both fresh and migration-005 upgrade paths; never apply to operator data during this work.
- **Sensitive free text:** use allowlist serializers and negative payload/log tests.
- **Child retry duplication:** server idempotency plus firmware pending-write state; never depend on UI debounce alone.
- **Activation partial failure:** one database transaction and stable replay identity; no compensating best-effort chain.
- **Firmware memory:** hard cap at three goals, one active image, existing 24 KiB payload budget, and before/after instrumentation.
- **Large observation periods:** the effective timeline folds amendments across all matching rows before applying API pagination, and analytics intentionally evaluates the selected period in memory. Current household-scale use is acceptable, but SQL-level effective-view pagination/aggregation should precede very large imports or long unbounded ranges.
- **Hardware behavior:** compilation and static state checks pass, but touch behavior, offline transitions, image fallback, approval refresh, celebration persistence, and 25-cycle memory stability still require a physical child panel.
- **Security scanner gap:** Aikido was unavailable, so this is a manual/static security review rather than an Aikido-clear release.
- **Visual refinement:** functional conservative UI only; Cursor owns final spacing, visual hierarchy, illustrations, and motion polish after contracts and flows pass.

## 18. Remaining UI polish for Cursor

After functional verification, Cursor may refine typography, spacing, responsive card/table treatment, preset imagery, age-aware content presentation, charts, panel-preview framing, microcopy, transitions, and celebratory motion. It must preserve semantic controls, keyboard flow, reduced-motion behavior, server validation, child-safe preview projection, route/state contracts, and the non-punitive language defined here.

## 19. Migration and release instructions

1. Back up the SQLite database and application sources.
2. Stop the server using the normal operator procedure only after explicit production approval.
3. Deploy server code and migrations 006, 007, and 008 together. `npm run init-db` or normal server startup applies them in checksum-locked order; run this first against a restored staging copy and inspect `schema_migrations` plus foreign-key integrity.
4. Run the documented health/admin/display smoke checks and verify current children before enabling new UI routes.
5. Deploy the compatible web client.
6. Flash revision-3 child firmware only after the server additive DTO/action routes are live; household firmware needs release only if shared source changed.
7. Roll back before live migration by restoring source only; roll back after migration by restoring the pre-migration database backup and compatible sources. There is no SQL down migration.

These are instructions only. No deployment, restart, live database migration, or device flashing is part of local implementation.
