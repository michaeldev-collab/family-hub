# Child UI Recovery Record

Status: **P0 child bootloop recovery candidate built and statically verified;
physical-panel boot acceptance remains open**  
Date: 2026-07-17  
Child target: `waveshare7b-child-launcher`  
Sibling regression target: `waveshare7b-household-launcher`  
Method: Spec -> Plan -> Execute -> Verify

This is the working evidence record for the dedicated Family Hub child firmware. It
records the audited baseline, confirmed causes, implemented architecture, verification
evidence, and the checks that still require the Waveshare panel. It is not a claim of
hardware verification.

## Context boot and operating route

- `Operating/CURRENT_PRIORITIES.md` and `Operating/APPROVAL_GATES.md` were read on
  2026-07-17. Both were updated 2026-07-05 (12 days old).
- `Operating/PROJECT_MAP.md` was updated 2026-07-15 (2 days old), but has no Family
  Hub mapping. The repository is therefore the implementation source of truth.
- Risk class: **repo-mod**. Local source, tests, docs, and builds were authorized.
  Flashing, deployment, restart of a real panel/server, secrets handling, and
  production changes were not authorized or performed.
- Route: Matrix board -> CTO build execution, using IoT, UI/UX, architecture,
  API/security, quality-verification, and kill-critic lenses.

### Matrix skills used

| Skill | Purpose |
|---|---|
| `3dl-matrix` | Spec/Plan/Execute/Verify orchestration and board routing |
| `3dl-matrix-build`, `3dl-matrix-builder` | Architecture-first gated implementation |
| `3dl-matrix-iot` | ESP32-S3, LVGL, PSRAM, launcher, and hardware boundaries |
| `3dl-matrix-ui` | Fixed-panel hierarchy, touch sizing, and semantic child states |
| `3dl-matrix-architecture` | Runtime ownership and dependency-boundary audit |
| `3dl-matrix-quality-docs` | Evidence ledger and acceptance handoff |
| `3dl-matrix-killcritic` | Adversarial source, contract, and artifact review |
| `3dl-matrix-phased-engineering-plans` | Dependency-ordered recovery phases |
| `3dl-matrix-execute-planner-plan` | Incremental execution and verifier loops |

The supplied toddler-panel brief was treated as the approved design input. No browser
mockup was substituted for firmware implementation.

## P0 child bootloop incident — 2026-07-17

The first recovery artifact was reported to bootloop on the child panel. No serial
cycle/backtrace or attached serial device was available in this workspace, so the
reset mechanism is not claimed as hardware-confirmed. The source and ELF audit did
confirm the following child-only startup regression:

- current child startup initialized the 1024x600 RGB panel, enabled the 80% backlight,
  and reserved two 20,480-byte internal-DMA bounce buffers before starting Wi-Fi;
- the archived hardware-validated firmware and current household firmware both start
  Wi-Fi before display initialization;
- there is no unconditional reboot in child setup. The only application-authored
  `ESP.restart()` is behind the parent exit flow;
- static child RAM is 100,804 bytes, normal first-party stack frames fit the 8 KiB
  loop stack, the application fits its configured Launcher slot, and image buffers
  allocate lazily in PSRAM.

The P0 candidate therefore makes two bounded child-only changes:

1. Wi-Fi initialization again occurs before RGB/DMA/backlight initialization, matching
   both known-good startup paths. API synchronization still waits until the intentional
   selection/offline frame is on glass.
2. Panic, interrupt/task/other watchdog, brownout, power-glitch, and CPU-lockup resets
   bypass retained child auto-resume and automatic media loading. The panel lands on
   the lightweight selection view and requires an explicit child tap before retrying
   the richer child state. This contains a server-state/image-triggered warm-reset
   loop without deleting configuration or changing normal cold-boot behavior.

Serial remains the decisive hardware discriminator: reset reason 4 is panic; 5/6/7
are watchdog; 9 is brownout; 14 is power glitch. A watchdog stopping between the
`before screen construction` and `after screen construction` records would elevate
the remaining fixed 48 KiB LVGL-arena hypothesis. The candidate has not been flashed
or hardware-verified by this session.

## Parent Approvals browser compatibility incident — 2026-07-17

The parent Approvals page reported `crypto.randomUUID is not a function`. The
failure was reproduced with the LAN-HTTP browser capability set: `crypto` and
`getRandomValues` present, but `randomUUID` absent. The exception occurred while
evaluating the API call arguments, before `fetch`, so the reported click did not
reach the server and did not create a partial approval.

Confirmed causes and repairs:

- Fourteen parent mutation methods called `crypto.randomUUID()` directly. They now
  share one generator that prefers native `window.crypto.randomUUID()`, falls back to
  an RFC 4122 UUIDv4 made from 16 `window.crypto.getRandomValues()` bytes, and fails
  before transport with a clear compatibility message if no secure random source is
  available. No `Math.random`, timestamp, CSRF value, or static-key fallback is used.
- Task Reject and Excuse submitted `note`, while the domain contract requires
  `reason`. The shared private note field is now labeled for both uses, requires a
  nonblank value for Reject/Excuse, and maps that value to the server-authored reason.
  Approve keeps the note optional.

The server contract was not weakened: idempotency keys remain action-scoped,
parent-session actor-bound, request-hash-bound, and transactionally stored. Regression
coverage executes two approval mutations with `getRandomValues` but no `randomUUID`,
asserts distinct valid UUIDv4 headers, and proves a browser with no secure RNG sends no
request. The complete server/UI suite passes 69/69 and the real Chromium smoke passes.
The LAN capability mismatch is covered by the focused browser-global VM regression;
the Chromium smoke validates the surrounding real-browser UI flow but runs on trusted
localhost and therefore does not itself force the missing-`randomUUID` condition.

## Source-of-truth audit

The audit covered `platformio.ini`, all active child and shared firmware sources,
LVGL/display/touch ownership, state and API clients, panel-image decoding, M5 Launcher
partition/output scripts, server display routes and services, contract/domain tests,
project documentation, and `docs/prop.md`. No secret value, `.env`, live database, or
production data was read.

### Current file and runtime ownership map

| Area | Active owner |
|---|---|
| Child boot, loop, navigation, sync, image pools | `firmware/src/child/main.cpp` |
| Child display API, writes, HMAC header, JPEG decode | `firmware/src/child/child_api_client.cpp` |
| Compact DTO filters and validation | `firmware/src/child_focus_state.cpp` |
| Child LVGL screens and design system | `firmware/src/child/child_ui.cpp` |
| Child runtime types and API declarations | `firmware/include/child/child_api.h`, `firmware/include/child_focus_state.h` |
| Shared transport/auth defaults | `firmware/src/shared/api/api_client_core.cpp` |
| Shared RGB/touch driver | `firmware/src/shared/display/display.cpp` |
| Shared memory diagnostics | `firmware/src/shared/diagnostics/memory.cpp` |
| Backend child display DTO | `server/services/displayState.js` |
| Backend child display actions | `server/routes/display-rewards-behavior.routes.js` |
| Household clock, task/reward domain writes | `server/services/domainCore.js`, `server/services/taskCompletions.js`, `server/services/rewards.js` |
| Reward-term and admin availability validation | `server/services/rewardTerms.js`, `server/services/adminResources.js` |
| Launcher targets and output | `firmware/platformio.ini`, `firmware/launcher_copy.py` |
| Contract and regression tests | `server/tests/display-api.test.js`, `server/tests/domain-services.test.js`, `server/tests/admin-api.test.js` |

### Binary ownership

- `waveshare7b-child-launcher` links child, shared, and compact child-state sources.
  Its final object list contains no household object or household UI implementation.
- `waveshare7b-household-launcher` links household and shared sources. It links no
  child or child-focus object. The shared diagnostic counter retains the neutral
  `activeChildRoots` name in both binaries; it is not a linked child screen/runtime.
- Household-only and child-only `UiManager` members are compile-time guarded, so one
  firmware no longer allocates the other application's high-level state.
- The applications remain separate binaries and continue to share only stable
  low-level display/network/diagnostic code.

## Confirmed baseline root causes and resolutions

These findings were confirmed from executable paths or regression probes. Suspected
hardware symptoms are listed separately and are not presented as proven causes.

| Confirmed cause | Resolution |
|---|---|
| Firmware omitted the required task occurrence date, making completion requests deterministically fail with 422. | Contract revision 2 adds server-authored `taskDate`; firmware validates and returns that exact date. The route rejects mismatches. |
| Task idempotency was per random request key, so different keys could award one assignment/date twice. | Stable logical firmware keys plus a transactional server-side assignment/child/date identity prevent duplicate writes and awards. |
| Rejected/cancelled completion history could join back as duplicate task cards. | The display query selects only the newest current non-terminal request. A rejected-then-resubmitted DTO regression asserts one card and the new request. |
| Term reward selection omitted `termId`; the domain also accepted another child's term. | Firmware sends the active term; the service validates a current term owned by the selected child. |
| Per-child visual-only mode disabled buttons in the DTO but did not authorize server writes. | Every display mutation now enforces the effective per-child interaction policy server-side. |
| Hidden corrections could still be acknowledged, and arbitrary incident IDs were trusted. | Acknowledge requires the correction module/focus/indicator policy and the exact currently visible incident. |
| Sync owned the current page and could steal normal Home/Tasks/Rewards navigation. | `ChildFocusRuntime` owns local navigation; only authoritative correction/first-then/waiting/celebration transitions may preempt it. |
| A polling first-then state could replace an active parent PIN screen. | PIN owns the screen until resolved/cancelled; only a safety correction may preempt it. |
| Full asset pools were cleared before every poll/tab render, freeing buffers still referenced by the current LVGL tree and destroying safe offline media. | Changed assets decode into a temporary candidate. The current root is cleaned immediately before a referenced old buffer is released; unchanged references are reused without a rebuild. |
| Focus and grid image slots could both appear to own the same decoded buffer during navigation, risking a later double-free or dangling LVGL descriptor. | `movePanelImage` now performs an explicit ownership transfer after root cleanup and nulls the source. Page starts are tracked, and focus images move back to their matching grid slot. |
| Every selected-child asset was eagerly fetched, causing serial stalls, allocator pressure, and unnecessary PSRAM retention. | Pools are page-specific and bounded. Selection profiles load one per 250 ms pump; task/reward grids use six-image page pools; selected-child screens retain only currently useful images. |
| Empty/missing media could retire currently visible images before all required replacements received a network attempt. | Missing-slot retirement is staged until non-empty required media has had its bounded attempt; failed replacements render the fallback without retaining a wrong asset. |
| Background network/image work and a reconnect render could begin during a touch; display ticking also happened too late to make the gate authoritative. | `displayTick()` now runs first. Reconnect, sync, image, poll, serial, and idle commits defer until release; a Wi-Fi render latch commits the status afterward. |
| Child startup had been reordered to reserve RGB/DMA resources and enable the backlight before Wi-Fi, unlike both hardware-validated startup paths. This could create an internal-DMA allocation failure or panel-plus-radio power transient. | Wi-Fi initialization again precedes display initialization. The intentional selection/offline frame still renders before synchronous API state work. Abnormal reset reasons also suppress retained-child/media auto-resume until an explicit tap. |
| The render fingerprint omitted action, progress, correction, reward, label, media, and child/config state. | The fingerprint now covers the complete visible state plus transient pending/result state; image setters also mark the screen dirty. |
| Raw API text could reach an LVGL font without guaranteed glyph coverage. | Visible API strings are bounded and converted to supported printable ASCII; active child source uses no emoji or `LV_SYMBOL`. |
| Empty arrays generated fake task/reward cards and the Home screen had competing equal-weight content. | Home has one dominant current/next task, distinct daily and term rewards, and intentional empty/completed states. Lists render only actual items. |
| Task/reward actions stayed visible during writes or on stale/offline snapshots. | Local pending state renders before transport, blocks repeated taps, and offline/stale state disables writes. A definite 4xx refreshes authoritative state before controls return. |
| A selected task/reward could disappear during sync while its old detail target remained active. | Authoritative commit normalizes vanished targets back to Home. Empty reward targets render locked/back-only and cannot dispatch an empty identifier. |
| Parent PIN verification entered a blocking POST without guaranteeing that the waiting state was visible; cancel could leave a stale focus page. | PIN renders and flushes waiting before transport, shows a clear ASCII error on failure, and cancel reapplies authoritative focus. |
| Correction UI stopped at acknowledge even when the domain required a recovery request. | The DTO now carries a bounded recovery action; firmware presents acknowledge then recovery, with child/incident/recovery ownership checks and a stable logical key. |
| A non-safety correction could lose screen priority to First-Then, and correction fingerprinting omitted acknowledgment/severity transitions. | Every active correction now resolves before First-Then; the UI hash covers acknowledgement, severity, recovery ID/status, and allowed actions. PIN remains interruptible only for safety. |
| Reward availability was fail-open for malformed configuration and filtering after a SQL limit could starve valid rewards or hide active goals. | Runtime and admin validation accept only bounded `days`/`start`/`end` objects, fail closed otherwise, retain every active selected goal, then fill remaining slots with available rewards. |
| Raw timestamp string ordering could celebrate an older approval or a future-dated approval; term progress could include future rows. | SQLite `julianday` ordering plus an upper bound selects the latest real past instant, and term progress is bounded by the household as-of date. |
| Launcher exit PIN success returned to selection instead of attempting the configured launcher flow. | PIN purpose is separated into `exit` and `change-child`; authorized exit releases child resources and reboots. Actual resident Launcher return remains a hardware acceptance item. |

### Investigated but not confirmed as baseline causes

- Multiple LVGL roots, application timers surviving transitions, duplicate persistent
  handler registration, animation use-after-free, incorrect touch transforms, stack
  overflow, and a progressive heap leak were not found in static ownership paths.
- There is one active LVGL root, no child application timer, and no child animation.
  Runtime leak, touch, and stack conclusions still require serial evidence on hardware.

## Implemented architecture

### Screen and transition map

```text
Boot
  -> configured/only child -> authoritative priority state or Home
  -> otherwise -> Child Selection

Child Selection -> Home
Home <-> Tasks -> Task Detail -> Waiting/Completed
Home <-> Rewards -> Reward Detail -> Requested/Waiting

Authoritative overlays/replacements:
  Correction (highest ordinary state) -> Home
  First-Then -> Task Detail when a valid current task exists -> Home
  Waiting / Celebration -> Home on authoritative state edge

Parent PIN:
  safety correction may interrupt; other correction states wait
  change-child -> Child Selection
  exit -> release resources -> reboot; resident Launcher return is hardware-gated
```

There are no visible Routines or household destinations. Persistent child navigation
contains only Home, Tasks, and Rewards; Tasks is removed when the effective child
profile hides the task grid.

### State ownership

1. API responses are filtered and contract-validated before commit.
2. `displayHomeDoc` contains selection identities/config only; it does not expose
   sibling stars, corrections, task state, or discipline state.
3. `childModeDoc` is the selected child's bounded authoritative snapshot.
4. `ChildFocusRuntime` owns selected child, local page/tab/target, stale state,
   authoritative focus identity, pending action, and last interaction.
5. `UiManager` owns only LVGL object pointers, visible view state, and event requests.
6. `PanelImage` slots own decoded buffers; LVGL receives non-owning pointers only
   while the corresponding tree is alive.

### Timer, event, and refresh ownership

- One loop-driven poll schedule uses the server-provided bounded refresh interval.
- Display ticks remain in the Arduino loop; no child LVGL timers or animations exist.
- Every screen rebuild cleans the single active root and resets all stored pointers.
- Event handlers exist only on the current tree and are destroyed with it.
- The display/touch tick runs before the loop's input gate. Reconnect, sync,
  profile/view-image, polling, idle, serial, and Wi-Fi status commits defer while
  touch or a queued input action is active.
- Idle return applies only to ordinary detail/secondary pages, never PIN,
  first-then, correction, waiting, or celebration.

### Image ownership and memory bounds

- Server panel media must be an exact 128x128 JPEG. Encoded input is capped at 96 KiB.
- One decoded RGB565 image is 32,768 bytes in PSRAM.
- Selection loads at most eight profile images, one network attempt per 250 ms pump;
  it does not load any child's task/reward state or images.
- Selected-child pages release profile-selection assets. Home normally keeps header,
  current-focus, daily reward, and term reward images. Task and Reward use separate
  six-image pools for the current 2x3 page, with tracked page starts, bounded
  pagination, and 80x176 previous/next targets. Focused pages use one focus image;
  Treats is bounded to two visible rewards.
- Grid/focus reuse is a unique ownership transfer: the root is cleaned, the image
  descriptor/buffer moves, and the source slot is nulled. First-Then's THEN asset is
  held in bounded secondary slot zero and restored only after old panel references
  are gone.
- Temporary replacement uses one candidate decoded image, one bounded encoded buffer,
  and a 4 KiB internal JPEG work buffer. Every failure clears the candidate; obsolete
  image retirement is staged until required non-empty media has had its network
  opportunity. Failed changed assets fall back instead of retaining the wrong image.
- Image connect/total timeouts are 800/1500 ms. API state requests remain bounded at
  1000/2500 ms.

### Child UI design system and screens

- Native panel geometry is 1024x600.
- Reusable colors, cards, actions, waiting/completed/locked states, progress rows,
  fallback tiles, headers, and navigation replace ad hoc per-screen styling.
- Primary and navigation targets are at least 80 px high; focal actions are larger.
- Home prioritizes one current/next task, then daily reward and larger term reward.
- Task list/detail bind status and image to the actual assignment. Awaiting/completed
  states replace the completion action.
- Reward list/detail render locked, available, selected, ready, and requested states.
  Conflicting same-type goals and term rewards without an active term are locked.
- First-then contains FIRST and THEN image cards plus a GO action when its current
  assignment is actually available. Otherwise it shows a non-actionable WAIT state.
  It has no child navigation; the only escape is the 160x80 parent-protected control.
- Correction contains one rule visual and a sequential acknowledge -> recovery
  request -> waiting/approved flow. It has no child navigation, parent note, incident
  narrative, or history; only the parent-protected escape remains.
- Missing media uses a simple shape/letter fallback. No Unicode emoji or unsupported
  icon font is required.

## API contract and privacy decisions

- Existing backend/domain services remain the source of truth; no second backend was
  added.
- Display contract stays version 1 with `contractRevision: 2` for the compatible
  task-date/action revision.
- Household timezone is authoritative for date, minutes, and weekday. Invalid legacy
  timezone values fall back to UTC. Task windows compare instants, including offset
  timestamps, instead of raw strings.
- Child payloads are bounded to eight profiles, twelve tasks, twenty-four rewards,
  12 KiB list responses, and 24 KiB child-mode responses.
- The task route accepts only the server-authored date and assignments present in the
  child's current visible task DTO.
- Task completion semantic identity is assignment + child + date, independent of the
  transport idempotency key.
- Firmware refuses empty task/reward/correction identifiers before transport and
  accepts only the supported reward domains (`daily`, `term`, `spendable`, and
  `milestone`).
- Reward term ownership/current-state and goal-type conflicts are enforced by the
  domain service, not only by UI state.
- Reward availability supports only bounded `days`, `start`, and `end` fields,
  including overnight windows. Malformed, scalar, empty, or unknown-key configs fail
  closed and are rejected by admin validation. Availability is evaluated before the
  visible limit; every active goal is retained before valid unselected rewards fill
  remaining slots.
- Term progress excludes records after the household as-of date. Celebration excludes
  future approvals and orders offset timestamps as real instants with `julianday`.
- Correction recovery exposes only ID/action/status and allowed actions. Its write
  requires the displayed child, incident, and recovery to match, requires acknowledge
  first, and replays the stable idempotency key without duplicating a request.
- Selection DTOs carry identity/profile data only. Child-mode DTOs omit parent notes,
  administrative history, audit logs, schedules/config internals, secrets, session
  fields, sibling data, routines, and private media paths. The backend retains a
  separate routine endpoint for other consumers; child-mode never returns it.
- Display writes remain routed into existing task, reward, behavior, ledger, and
  audit-domain logic.

## Memory diagnostics and findings

Instrumentation now logs at:

- boot and boot complete;
- before and after every API GET attempt, including early Wi-Fi/HTTP-begin failure;
- after JSON parse and state commit;
- before and after image load;
- before and after screen construction;
- task detail, return Home, Rewards open, and repeated navigation;
- before launcher exit.

Each record includes:

- free, minimum, and largest internal 8-bit heap block;
- free, minimum, and largest PSRAM block;
- current task stack high-water mark and `StackType_t` unit;
- active decoded image count;
- active child root count.

Static ELF internal RAM is 100,804 bytes (30.76%) for child and 97,116 bytes
(29.64%) for household. These are compile-time `.dram0.data + .bss` figures, not
runtime free-heap or PSRAM results. The root count is a single-root design metric
(`displayReady ? 1 : 0`), not a recursive LVGL object-tree count.

## Phase and verifier ledger

### Phase 1 — audit

- [x] Source, screen, state, timer, image, API, launcher, and build maps completed.
- [x] Confirmed causes separated from hardware-only hypotheses.
- [x] Independent architecture, product, security, delivery, and artifact reviews run.

### Phase 2 — lifecycle and contracts

- [x] Screen/root, event, image, sync, and touch sequencing stabilized.
- [x] Task/reward/correction contracts and authorization repaired.
- [x] Stable logical idempotency and local pending state added.
- [x] Launcher exit/change-child purposes separated.
- [x] Required memory instrumentation added.

### Phase 3 — child design system and screens

- [x] Shared child visual constants/components established.
- [x] Home, Tasks/detail, Rewards/detail, First-Then, Correction, Waiting, and
  Celebration rebuilt against real DTO state.
- [x] Placeholder task/reward cards, unsupported glyph assumptions, and undersized
  primary controls removed.

### Phase 4 — local verification

- [x] Complete server suite passed.
- [x] Child and household firmware targets built from current sources.
- [x] Artifacts validated, copied, hashed, and checked for source/symbol isolation.
- [x] Static glyph, privacy, memory-boundary, lifetime, and state-transition reviews run.
- [ ] Physical visual/touch/runtime-memory/launcher verification (hardware required).

### Phase 5 — P0 bootloop recovery

- [x] Reopened acceptance after the child bootloop report; no prior build result was
  treated as runtime proof.
- [x] Audited explicit reset paths, startup ordering, RTC resume, Launcher artifact,
  stack/static RAM, PSRAM image bounds, and LVGL allocation behavior.
- [x] Restored Wi-Fi-before-display resource ordering and added abnormal-reset safe
  selection/media suppression.
- [x] Rebuilt the child target, rebuilt the household regression target, validated
  image headers/checksums/hashes, and confirmed build/dist byte identity.
- [ ] Flash and serially verify the P0 candidate on the physical panel.

### Phase 6 — parent Approvals compatibility repair

- [x] Reproduced the LAN-HTTP `crypto.randomUUID` failure before transport.
- [x] Centralized all 14 affected mutations on a secure feature-detected UUIDv4
  generator with a no-transport fail-closed path.
- [x] Corrected task Reject/Excuse note-to-reason client contract mapping.
- [x] Passed focused VM compatibility tests, real Chromium smoke, full 69-test suite,
  syntax scans, an independent UUID/security critic, and an independent
  Approve/Reject/Excuse call-site and privacy critic.

## Build and verification results

| Command/check | Result |
|---|---|
| `cd server && npm test` | PASS: 69 passed, 0 failed, 0 skipped; 5.287 s Node duration after the Approvals compatibility and decision-payload repairs |
| `NODE_ENV=test node --test --test-concurrency=1 tests/web-admin.test.js` | PASS: native Web Crypto, `getRandomValues`-only UUIDv4 fallback, secure-RNG fail-closed, and Reject/Excuse reason mapping |
| `NODE_ENV=test node --test --test-concurrency=1 tests/web-browser-smoke.test.js` | PASS: real headless Chromium parent UI/auth/mutation/responsive smoke; 1.744 s test duration |
| `NODE_ENV=test node --test --test-concurrency=1 tests/display-api.test.js` | PASS: 11/11; final focused contract, privacy, timezone/availability, recovery, PIN, and idempotency verification |
| targeted display/admin edge verifier | PASS: 16/16; malformed availability, goal retention/starvation, append-only cleanup, and replay probes |
| `node --check` on `rewards-api.js`, `rewards-admin.js`, and `web-admin.test.js` | PASS; no syntax warnings |
| `platformio run -e waveshare7b-child-launcher` | PASS after the P0 ordering and safe-boot changes; RAM 100,804/327,680 (30.8%); program use 1,365,432/16,777,216 (8.1%); no compiler or custom-option warning |
| `platformio run -e waveshare7b-household-launcher` | PASS; RAM 97,116/327,680 (29.6%); program use 1,252,576/16,777,216 (7.5%); no compiler or custom-option warning |
| esptool 4.8.6 `image_info` | PASS: child is ESP32-S3, 6 segments, checksum/hash valid; household is ESP32-S3, 5 segments, checksum/hash valid |
| build/dist `cmp` and SHA-256 | PASS: each launcher copy is byte-identical to its build binary |
| source freshness and ELF object/symbol isolation | PASS: no relevant source is newer than its ELF; no counterpart high-level object is linked into either target |
| active child glyph/privacy scans | PASS: no `LV_SYMBOL`, emoji/icon escape in LVGL UI text, private DTO field access, or child-mode routines payload |
| independent final kill-critic | PASS statically: no remaining obvious UAF, double-free, duplicate persistent handler/timer, invalid target dispatch, or idempotency gap |
| independent P0 boot-fix critic | PASS after correction: boot direct entry, reconnect, 250 ms media pump, selected-view pump, and periodic refresh all preserve abnormal-reset safe mode until an explicit child tap |
| physical-panel boot/stress retest | NOT RUN on the P0 candidate; the predecessor was reported to bootloop |

### Generated binaries

| Target | Build binary | Launcher copy | Size | `0x180000` slot | Headroom | SHA-256 |
|---|---|---|---:|---:|---:|---|
| Child | `firmware/.pio/build/waveshare7b-child-launcher/firmware.bin` | `firmware/dist/family-hub-child.bin` | 1,365,904 B | 86.842% | 206,960 B | `a266f598549aa82aaca2e69a10f40f5bcb4807802a7630a8ae160e578b0b43a2` |
| Household | `firmware/.pio/build/waveshare7b-household-launcher/firmware.bin` | `firmware/dist/family-hub-household.bin` | 1,252,944 B | 79.660% | 319,920 B | `529eb7e0cf9d1904c7691afcb7b2c7dcd11c21252a6303248dbe1b3e69f85b74` |

No source dependency was newer than its corresponding final ELF. The child build
contains four child objects, `child_focus_state.cpp`, and five shared objects; the
household build contains four household objects, `dashboard_state.cpp`, and five
shared objects.

### Stress-test result

The automated/static verifier exercised rapid duplicate task/reward/recovery writes,
replay after state changes, malformed/empty payloads, vanished selections, correction
priority, offset/future timestamps, unavailable high-priority rewards, and missing
media failure paths. Those checks passed, and the final lifecycle review found no
obvious retained timer/handler, invalid pointer, or duplicate image owner.

The required physical 50-cycle Home/Tasks/Rewards/detail test was **not run**. Runtime
heap/PSRAM plateaus, stack margins, touch behavior, and launcher return therefore
remain unverified and are not inferred from the successful build.

## Files changed

- `docs/child-ui-recovery.md`
- `firmware/platformio.ini`
- `firmware/launcher_copy.py`
- `firmware/include/api_client.h`
- `firmware/include/child/child_api.h`
- `firmware/include/child_focus_state.h`
- `firmware/include/display.h`
- `firmware/include/secrets.example.h`
- `firmware/include/shared/diagnostics/memory.h`
- `firmware/include/ui_manager.h`
- `firmware/src/child/child_api_client.cpp`
- `firmware/src/child/main.cpp`
- `firmware/src/child/child_ui.cpp`
- `firmware/src/child_focus_state.cpp`
- `firmware/src/household/main.cpp`
- `firmware/src/shared/api/api_client_core.cpp`
- `firmware/src/shared/diagnostics/memory.cpp`
- `firmware/src/shared/display/display.cpp`
- `server/routes/display-rewards-behavior.routes.js`
- `server/services/adminResources.js`
- `server/services/displayState.js`
- `server/services/domainCore.js`
- `server/services/rewardTerms.js`
- `server/services/rewards.js`
- `server/services/taskCompletions.js`
- `server/tests/admin-api.test.js`
- `server/tests/display-api.test.js`
- `server/tests/domain-services.test.js`
- `server/tests/fixtures/display-contract-v1.json`
- `server/tests/web-admin.test.js`
- `web/js/rewards-api.js`
- `web/js/rewards-admin.js`

## Known limitations and remaining blockers

1. **P0 hardware retest is open.** The candidate builds and its startup regression is
   source-confirmed, but no serial reset cycle or post-fix physical boot has been
   observed. Do not label the bootloop closed until the panel reaches `boot complete`
   and remains stable. Capture the full repeating boot banner/backtrace if it does not.
2. **LVGL exhaustion remains a conditional hypothesis.** The child and household
   share a fixed 48 KiB LVGL arena. Allocation assertions can halt until watchdog
   recovery. If the candidate reports reset reason 5/6/7 during screen construction,
   add admission/fallback behavior based on measured LVGL free/largest blocks rather
   than blindly increasing internal RAM.
3. **Production/security gate — parent PIN transport.** The firmware uses HTTP over
   `WiFiClient`, while `requireSecureParentTransport` correctly fails closed in
   production (HTTP 426 on an insecure request). Do not weaken that middleware. A
   TLS/reverse-proxy or equivalent approved transport decision is required before
   production PIN/change-child/exit acceptance. Offline parent-protected actions also
   fail closed.
4. **Physical runtime evidence is absent for the candidate.** Runtime PSRAM minima, fragmentation,
   stack margin, touch mapping, actual visual hierarchy, 50-cycle navigation, Wi-Fi
   recovery, and launcher return have not been measured on the panel.
5. **Launcher return is not proven.** The child exit path currently releases resources
   and calls `ESP.restart()`. Whether that returns to the resident M5 Launcher depends
   on the installed Launcher/version/partition flow and must be observed; no unsafe
   OTA partition guess or erase was added.
6. **Device provisioning was not inspected.** No secret was read, and a real
   `PANEL_TOKEN`/device-ID binding was not exercised. Authentication tests use isolated
   test capabilities only.
7. **Demo media fixtures are not visual evidence.** Existing seeded JPEG fixtures are
   four-byte marker files. They intentionally exercise failure paths but cannot prove
   decoded visual quality.
8. **Bounded synchronous transport remains.** State fetches can pause for up to the
   configured 2.5-second timeout. Image fetches are shorter and selection is pumped,
   but this must be observed under weak Wi-Fi.
9. **Time boundary needs physical/integration observation.** Configured household
   timezone behavior and invalid-zone UTC fallback have automated coverage, but panel
   sync across a real near-midnight rollover has not been observed.
10. **Root/image counts are bounded diagnostics, not proof of a leak-free runtime.**
   `active_child_roots` is `displayReady ? 1 : 0`, not a recursive LVGL object count;
   the six-card page pools still require a serial plateau check on the panel.
11. **Parent mutation transport replay is not single-flight.** Each deliberate UI
   invocation receives a fresh secure key. Domain lifecycle checks protect approvals
   and reward deductions, but rapid double taps or an ambiguous retry of repeatable
   operations (for example an administrative ledger correction) do not reuse the same
   key. Add an in-flight mutation guard and retain a key across transport retry before
   treating client-side idempotency as complete.
12. **Automated Aikido verification was unavailable.** Repository configuration opts
   out, so security verification used contract tests plus independent manual/static
   critic passes.

## Hardware acceptance checklist

Do not mark these complete from builds or static analysis.

- [ ] Install `firmware/dist/family-hub-child.bin` through the approved M5 Launcher
  process (do not use the launcher PlatformIO environment's upload target).
- [ ] Capture 115200-baud serial from the Family Hub banner through `boot complete`.
  Confirm Wi-Fi initialization begins before `[disp]`, the intentional selection/offline
  frame appears before synchronous API work, and household UI never loads.
- [ ] Force or reproduce one abnormal reset. Confirm the next boot logs
  `bypassing remembered child`, remains on lightweight selection without auto-loading
  media, and retries the full view only after an explicit child tap.
- [ ] Verify intentional empty, one-task, and multiple-task Home/Tasks layouts.
- [ ] Confirm every primary target is easy for a two-/three-year-old and touch
  coordinates match the displayed control.
- [ ] Rapidly tap task completion: one request is recorded, waiting appears
  immediately, and approval/rejection/resubmission resolves correctly.
- [ ] Remove/corrupt an asset: fallback renders without a crash, stale pointer, or
  wrong-child image.
- [ ] Disconnect Wi-Fi during every page and action: last-known safe state remains,
  writes disable, OFFLINE is clear, and reconnect commits authoritative state.
- [ ] Exercise locked, available, selected, ready, and requested reward states,
  including rapid request taps, same-type goal conflicts, every six-card page, and
  previous/next image reuse/release.
- [ ] Switch children repeatedly: previous state/media is released and never appears
  under the new identity.
- [ ] Trigger sync while touching/navigating and while PIN/first-then/correction is
  active; no unsafe rebuild or navigation theft occurs.
- [ ] Exercise first-then with valid, hidden, missing, and completed tasks. Exercise
  correction acknowledge -> recovery request -> waiting/idempotent replay -> parent
  resolution; verify ordinary corrections preempt ordinary UI, only safety interrupts
  an active PIN, and no correction exposes parent detail.
- [ ] Navigate Home/Tasks/Rewards/details at least 50 cycles. Compare free/min/largest
  internal heap, free/min PSRAM, stack HWM, active image count, and root count at each
  repeated boundary; require a stable plateau rather than progressive loss.
- [ ] Complete parent-protected change-child and exit over the approved secure
  transport, then confirm the configured M5 Launcher boot/menu behavior.
- [ ] Flash only through the approved operator process and record panel identity,
  binary SHA-256, serial log, and firmware version with the test evidence.

## Deviations encountered and resolved

- The first non-escalated `npm test` could not complete its loopback server work and
  was interrupted (exit 130). The final suite was rerun with explicit approved
  escalation and now passes 69/69 on the final source/fixture state.
- The focused Chromium smoke likewise could not launch/bind cleanly in the restricted
  sandbox and was interrupted (exit 130). Its isolated escalated rerun passed, as did
  the Chromium case inside the final full suite.
- PlatformIO required access to its installed home/cache; both final build commands
  were rerun with explicit approved escalation.
- No `/dev/ttyACM*` or `/dev/ttyUSB*` panel was present, and opening/flashing a real
  device was not authorized. The bootloop mechanism therefore remains hardware-gated.
- The first P0 safe-boot critic found that periodic selection polling could bypass
  media suppression after the refresh interval. That branch now mirrors reconnect:
  safe mode renders without media, and only an explicit child tap clears safe mode.
  The child image was rebuilt, revalidated, and rehashed after this correction.
- The repository's `.git` directory is empty/nonfunctional, so `git status`/diff could
  not be used. Reviews used direct source inspection, target object inventories,
  source-versus-ELF timestamps, hashes, `cmp`, and independent critic passes.
- One intermediate child compile exposed a stale `childExitBtn_` helper reference and
  was repaired before the final build.
- One intermediate household build exposed child handler declarations under the
  wrong compile guard and was repaired before the final household build.
- The new rejected/resubmitted DTO test first used a nonexistent parent fixture,
  causing a foreign-key failure; it was corrected to the credential actor path and
  then passed in the targeted and full suites.
- The final artifact critic found `/actions/request-recovery` missing only from the
  frozen contract fixture's action-route list. The fixture was corrected; the focused
  display suite passed 11/11 and the then-current full suite passed 67/67; the current
  final suite passes 69/69 after the two Approvals browser regressions were added.
- An old esptool 4.5.1 installation falsely identified the ESP32-S3 image as ESP8266.
  The build-associated esptool 4.8.6 was selected explicitly and validated both
  images, checksums, and hashes.
