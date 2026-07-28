# Reward and Discipline Add-On — UX Workflow Plan

**Status:** planning baseline  
**Source of truth:** `prop.md`  
**Product owner:** Michael  
**UX owner:** CPO / UX Workflow  
**Execution owner:** COO UI (web) and CTO IoT (panel)

## Outcomes

The parent can configure, review, approve, correct, and audit the system without
using the panel. Each child can visually understand: who they are, what to do,
whether an action is waiting, what was approved, what daily reward is near, what
term reward is progressing, and how to recover from one active correction.

## Surface contract

| Capability | Parent web | Child panel |
|---|---:|---:|
| Create/edit/archive configuration | Yes | Never |
| Approve/reject/adjust requests | Yes | Never |
| View private notes, audit, full history | Yes | Never |
| Select profile and view assigned tasks | Preview/admin | Yes |
| Request task completion or reward | Review | Yes, when allowed |
| View current correction | Full record | Child-safe current state only |

## Parent journeys

### Initial setup

1. Unlock the parent surface.
2. Create or select a child profile and image.
3. Configure attendance behavior and screen visibility.
4. Create reusable image-first tasks and assign a schedule.
5. Configure immediate stars, daily success, daily reward, and a child-specific
   term with reward.
6. Preview the exact child payload and panel presentation.
7. Activate only after validation succeeds.

### Daily approval loop

1. Overview prioritizes pending task, day-status, recovery, and reward actions.
2. Parent opens a request with child, task/reward image, timestamp, and current
   rule-derived effect.
3. Parent approves, rejects, or applies an allowed edited value.
4. The UI shows the committed ledger/progress result and audit reference.
5. A repeated submit resolves to the same result without duplicate effects.

### Correction and recovery

1. Parent records the rule/incident and chooses a calm, bounded consequence.
2. Parent defines the child-facing first-then message and explicit end condition.
3. Panel shows only the relevant child-safe correction.
4. Parent records recovery or resolution; the incident remains intact.
5. Normal reward workflow returns without automatic star loss.

## Child journeys

### Enter Child Focus

1. Tap the top-bar Child Focus control.
2. Select a large profile image unless a parent lock fixes the profile.
3. Load that child's compact dashboard with an unmistakable loading state.
4. Show profile, today, daily reward, term reward, and the highest-value task.

### Task request

1. Tap a large task image.
2. See image, simple label/audio where enabled, value, and one primary action.
3. Confirm completion request.
4. Show `waiting for parent`; do not show stars as earned yet.
5. On refresh, show approved celebration/progress or calm rejected/try-again state.

### Reward request

1. View only rewards allowed for the selected child.
2. Select a goal when permission and lifecycle state allow it.
3. When ready, request the reward.
4. Show waiting, approved/scheduled, declined, redeemed, or completed state based
   on server truth.

### Correction priority

An active correction may replace the normal focus content, but must retain an
obvious parent-controlled exit/resolution path and cannot expose adult notes,
severity history, or sibling information.

## Required states

Every applicable flow specifies: loading, empty, ready, confirm, pending,
approved, rejected, invalid, unauthorized, offline, stale, server error, and
archived/unavailable. Offline taps never imply successful writes.

## Embedded comprehension rules

- Stable image and color semantics; no reading required for the primary action.
- Minimum touch target appropriate for the 1024x600 panel; one clear primary
  action per child detail screen.
- Daily and term rewards occupy distinct, stable positions.
- Short terms use large dots; medium terms use smaller dots; long terms use a
  compact bar or ring.
- Calm correction imagery; no shame, frightening graphics, leaderboards, or
  sibling comparison.
- Low animation and a reduced-motion/sounds setting.

## UX acceptance criteria

- Both children have isolated schedules, rewards, progress, and corrections.
- A non-reader can identify profile, task, waiting state, daily reward, and term
  reward with normal parent support.
- Parent-only data is absent from every child state and network response.
- Profile lock, protected exit, and allowed action flags are server-enforced.
- Daily and term progress never visually merge.
- Offline, rejection, and server failure never resemble success.
- Frequent parent approvals are reachable from the overview without hunting.

## Learning evidence (CAO)

Before increasing term length or correction complexity, observe at least three
natural-use sessions per child. Record assistance level, confusion, mis-taps,
successful repetition, recognition of waiting versus approved, and recognition
of daily versus term reward. A parent must independently configure both example
schedules and explain the reward/discipline separation consistently.
