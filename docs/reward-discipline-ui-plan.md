# Reward and Discipline Add-On — UI Design Plan

**Status:** code-ready planning baseline; Figma execution pending if a connector
is chosen. This document does not authorize source execution.

## Existing visual system

Reuse the current Family Hub web shell, navigation, cards, typography, spacing,
brand colors, modals, and API error conventions. Add a `Rewards & Behavior`
section without replacing the existing grocery, chores, dinner, notes, members,
or settings workflows.

## Parent web information architecture

1. Overview
2. Children
3. Tasks & Routines
4. Approvals
5. Rewards
6. Reward Terms
7. Rules & Discipline
8. Star History
9. Display Settings

### Overview hierarchy

1. Attention queue: pending approvals, reward requests, active warnings, active
   consequences.
2. One summary card per child: identity, balance, daily progress/reward, active
   term/progress/reward.
3. Recent append-only activity.

### Management patterns

- List/detail or card/detail layouts on wide screens; stacked cards on phones.
- Archive/inactivate as the normal removal action.
- Forms expose child-facing preview beside parent-only configuration.
- Destructive or ledger-affecting actions require confirmation and show their
  audit consequence before submit.
- Approval queues use image, child, state, requested time, effect, and primary
  approve/reject controls; never rely on color alone.

## Child panel structure

### Home module

Compact reward summary with profile image, star or progress summary, daily reward,
term reward, and readiness indicator. It is a gateway, not an admin surface.

### Child selection

Large profile cards with photo/avatar, stable color, minimal label, and visible
ready/waiting state. Locked mode removes switching rather than merely hiding it.

### Child dashboard

Header: profile and protected exit affordance.  
Primary: today/task card.  
Secondary left: daily reward and progress.  
Secondary right: term reward and adaptive progress.  
Footer: minimal Tasks and Rewards navigation plus offline/stale status.

### Detail and lifecycle states

- Task: large image, one primary complete/request button, clear pending/result.
- Rewards: bounded allowed cards, goal selection, ready/request/waiting states.
- Correction: calm image, short first-then instruction, visible completion
  condition; no history or parent explanation.

## Component inventory

- `AttentionCard`, `ChildSummaryCard`, `ProgressIndicator`
- `TaskDefinitionForm`, `AssignmentEditor`, `ScheduleEditor`
- `ApprovalCard`, `RewardRequestCard`, `DailyStatusCard`
- `RewardCard`, `TermBuilder`, `AttendanceEditor`
- `RuleCard`, `IncidentForm`, `ConsequenceCard`, `RecoveryAction`
- `LedgerTable`, `ReversalDialog`, `AuditTimeline`
- `MediaPicker`, `ChildPreview`, `DisplaySettingsForm`
- Panel: `ProfileTile`, `ImageActionCard`, `DailyRewardCard`, `TermRewardCard`,
  `WaitingState`, `OfflineBadge`, `CorrectionCard`, `ProtectedExit`

## Responsive and accessibility requirements

- Web keyboard access, visible focus, semantic labels, error summaries, and
  non-color status cues.
- Parent UI supports reduced motion and readable contrast.
- Panel uses large targets, short paths, consistent icon placement, low motion,
  bounded text, and no dense tables.

## Data and performance boundaries

- Lists paginate or cap results; images use thumbnails in web lists.
- Panel uses panel derivatives only and loads images for the active view.
- UI never calculates authoritative balances, term qualification, permissions,
  or lifecycle transitions.
- Render every empty/error/offline/stale state before polish is accepted.

## UI verification

- Parent workflows verified at desktop and narrow phone widths.
- Panel views verified at 1024x600 and on physical Waveshare hardware.
- Screenshot/state checklist covers every lifecycle state in the UX plan.
- Repeated panel navigation shows no progressive retained-screen or image growth.
