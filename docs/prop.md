# Family Hub Reward and Discipline Add-On Proposal

## 1. Executive Summary

This proposal adds a child-focused reward, routine, and discipline system to the existing Family Hub.

The system is inspired by classroom star charts but adapted for the family’s actual needs:

* Children who are currently two and three years old
* A visual-first interface for children who cannot read
* Different schedules and reward periods for each child
* Immediate, daily, and multi-day reinforcement
* Parent-controlled configuration through the existing web UI
* Simple child interaction through the Family Hub screen
* Clear separation between positive rewards and discipline
* Configurable reward terms rather than hardcoded calendar weeks
* Controlled memory usage on the ESP32 panel

The existing Family Hub architecture will remain in place.

The web UI will continue to handle all creation, editing, approval, configuration, and history. The physical Family Hub screen will display simplified child-facing information and accept only limited child interactions.

---

# 2. Product Purpose

The add-on should help children understand four basic concepts:

1. What they are expected to do
2. What they did well
3. What they are working toward
4. How they can correct a problem

The system is not intended to replace parenting decisions or automatically judge behavior.

It provides a consistent framework for:

* Daily routines
* Age-appropriate responsibilities
* Positive reinforcement
* Short-term goals
* Multi-day goals
* Parent-approved rewards
* Warnings
* Corrective consequences
* Progress tracking

The system should reinforce behavior while remaining flexible enough for toddlers, preschoolers, and eventually older children.

---

# 3. Core Product Principles

The add-on should follow these principles.

## 3.1 Visual Before Textual

The child-facing interface should primarily communicate through:

* Profile pictures
* Reward photographs
* Task icons
* Rule icons
* Stars
* Progress circles
* Simple colors
* Large touch targets
* Basic animations

Text may still appear for parents and future readability, but the child should not need to read to understand the primary interaction.

## 3.2 Parent-Controlled

Parents create and control:

* Child profiles
* Tasks
* Star values
* Daily goals
* Term goals
* Rewards
* Rules
* Warnings
* Consequences
* Approvals
* Corrections
* Display settings

Children can interact with the system, but they cannot modify its rules or approve their own rewards.

## 3.3 Rewards and Discipline Remain Separate

Previously earned positive progress should not normally be erased because of unrelated poor behavior.

Rewards address positive behavior.

Discipline addresses:

* What happened
* Why it was a problem
* What consequence applies
* How the child can resolve it

The default behavior should not remove stars as punishment.

Star deductions should be reserved for actual reward redemption or administrative correction.

## 3.4 The System Should Reward Consistency, Not Perfection

A child should not automatically lose an entire multi-day goal because of one difficult moment.

The system should support:

* Successful days
* Partially successful days
* Recovery days
* Unsuccessful days
* Excused days
* Not-present days

Parents retain final judgment over whether the overall day or term was successful.

## 3.5 Each Child Uses Their Own Schedule

The system must not assume that every child shares the same calendar or reward duration.

Each child may have independently configured:

* Daily goals
* Reward term length
* Attendance schedule
* Qualification rules
* Reward catalog
* Reward difficulty
* Reset behavior

A calendar week is one possible configuration, not a system assumption.

---

# 4. Existing Family Hub Architecture

The existing architecture remains unchanged.

## Web UI

The web UI is the parent and administrator interface.

It handles:

* All editing
* All configuration
* Approval queues
* Detailed history
* Parent notes
* Media uploads
* Scheduling
* Rules
* Permissions
* Display configuration

## Family Hub Screen

The physical Family Hub screen is the simplified child-facing and household-facing interface.

It handles:

* Child profile selection
* Visual progress
* Task completion requests
* Reward goal selection
* Reward requests
* Simple warning displays
* Simple consequence displays
* Celebration feedback

## API and Server

The server remains the source of truth for:

* Child profiles
* Stars
* Tasks
* Rewards
* Reward cycles
* Approvals
* Rules
* Incidents
* Consequences
* History
* Display state

The panel should never independently own canonical reward or discipline data.

---

# 5. Main Reward Structure

Each child can have three simultaneous reinforcement layers.

## 5.1 Immediate Recognition

Immediate recognition occurs directly after a task, routine, or positive behavior is approved.

Examples:

* One star
* Celebration animation
* Completion sound
* Visual praise
* Progress added to the current day
* Parent-created praise message

This provides immediate cause-and-effect feedback.

## 5.2 Daily Reward

A daily reward is a smaller reward earned from progress during the current day.

Examples:

* Go to the park
* Choose a show
* Choose a snack
* Extra story
* Outdoor play
* Choose a game
* Pick the evening activity

Daily rewards should be understandable and relatively immediate.

## 5.3 Term Goal

A term goal is a larger reward earned through consistency across multiple qualifying days.

Examples:

* Swimming
* Special outing
* Indoor playground
* Movie night
* Favorite meal
* Family activity
* Small toy
* Special event

The duration of the term is configurable per child.

---

# 6. Configurable Reward Terms

The implementation should use the terms:

* `reward_term`
* `reward_cycle`
* `term_goal`

It should not use `weekly_reward` as the underlying model.

A week is only one type of term.

## 6.1 Supported Term Types

### Fixed-Length Term

Runs for a configured number of consecutive days.

Example:

```text
Duration: 3 days
Required successful days: 2
```

### Scheduled-Day Term

Counts only selected days.

Example:

```text
Qualifying days: Monday, Wednesday, Friday
Required successful days: 2 of 3
```

### Attended-Day Term

Counts only days when the child is present.

Example:

```text
Duration: 3 attended days
Reward available: Final attended day
```

This is the recommended model for the three-year-old.

### Calendar-Week Term

Uses a standard recurring week.

Example:

```text
Starts: Monday
Ends: Sunday
Required successful days: 5
```

### Rolling Term

Tracks the most recent configured number of qualifying days.

Example:

```text
Window: Most recent 4 qualifying days
Required successful days: 3
```

### Custom Date Term

Uses a parent-defined start and end date.

Useful for:

* Vacations
* School breaks
* Temporary goals
* Special events
* Travel
* Behavior transitions

### Manual Term

Starts and ends only when a parent chooses.

Useful for irregular schedules.

---

# 7. Example Child Configurations

## 7.1 Three-Year-Old

The three-year-old is present three days per week.

Recommended initial configuration:

```text
Term type: Attended-day term
Term duration: 3 attended days
Daily reward: Park
Term reward: Swimming
Required successful days: 2 of 3
Reward availability: Final attended day
Reset behavior: End of visit
```

Example flow:

```text
First attended day
→ Daily routines completed
→ Park reward approved
→ One term marker earned

Second attended day
→ Daily routines completed
→ Park reward approved
→ Second term marker earned

Final attended day
→ Term threshold reached
→ Swimming reward becomes available
```

The child-facing interface would show:

```text
Park reward
🌳

Swimming reward
🏊

● ● ○
```

The screen does not need to explain custody schedules, dates, or term types.

## 7.2 Two-Year-Old

The two-year-old is present full time, but a full seven-day period may be too long initially.

Recommended initial configuration:

```text
Term type: Fixed-length term
Term duration: 2 or 3 days
Required successful days: 2
Daily reward: Small preferred activity
Term reward: Playground or special activity
```

The duration should be adjustable over time.

Possible progression:

```text
2 days
→ 3 days
→ 5 days
→ 7 days
```

There is no requirement to increase the term if a shorter period continues to work better.

---

# 8. Defining Daily Success

A successful day should be configurable.

## 8.1 Task Threshold

The child completes a configured number of tasks.

Example:

```text
Complete 3 of 4 daily tasks
```

## 8.2 Required Tasks

Specific tasks must be completed.

Example:

```text
Brush teeth
Put toys away
Complete bedtime routine
```

## 8.3 Progress Threshold

The child earns enough daily stars or progress points.

Example:

```text
Earn 4 daily progress points
```

## 8.4 Parent Determination

The parent manually decides whether the overall day was successful.

This should remain available because not every meaningful behavior can be reduced to a checklist.

## 8.5 Combined Qualification

The day requires both objective progress and parent approval.

Example:

```text
Complete the bedtime routine
AND
Parent approves the overall day
```

---

# 9. Daily Status Model

Each qualifying day may have one of the following states:

* Pending
* Successful
* Partially successful
* Recovery completed
* Unsuccessful
* Excused
* Child not present
* Term paused

## Successful Day

Contributes full term progress.

## Partially Successful Day

May contribute partial progress if enabled.

## Recovery Completed

Indicates that the child corrected an earlier issue and regained some or all daily credit.

## Unsuccessful Day

Does not contribute term progress.

## Excused Day

Does not count against the child.

Examples:

* Sick day
* Travel
* Unusual family event
* Parent chooses not to evaluate the day

## Not Present

Excluded from attended-day terms.

---

# 10. Recovery System

The system should support corrective recovery.

Examples:

* A child initially refuses to clean up but later completes it.
* A child uses rough behavior but later calms down, apologizes, and demonstrates gentle behavior.
* A routine is missed but completed later with support.

Recovery settings may include:

* Recovery enabled
* Parent approval required
* Full or partial daily credit
* Required corrective action
* Deadline for recovery
* Related rule or incident

The recovery mechanism teaches that correcting a problem still matters.

It should not erase the incident history.

---

# 11. Star System

Stars provide immediate positive recognition and may also act as general reward currency.

Children may earn stars for:

* Chores
* Routines
* Positive behavior
* Learning activities
* Special achievements
* Parent-created bonuses

## 11.1 Star Transactions

Every star change should be recorded.

Fields include:

* Child
* Amount
* Transaction type
* Reason
* Category
* Related task
* Related reward
* Issuing parent
* Timestamp
* Optional note
* Reversal reference

Possible transaction types:

* Award
* Bonus
* Reward redemption
* Administrative correction
* Reversal

## 11.2 Discipline and Stars

Stars should not be automatically removed for discipline.

In V0.1:

* Rewards may spend stars.
* Administrative errors may be corrected.
* Discipline does not deduct stars.
* Parents cannot create automated star penalties.

This preserves trust in the reward system.

---

# 12. Reward Types

## 12.1 Daily Rewards

Earned from daily progress.

Examples:

* Park
* Snack
* Show
* Story
* Outdoor play
* Choice of activity

## 12.2 Term Rewards

Earned from progress across the configured term.

Examples:

* Swimming
* Special outing
* Movie night
* Playground
* Favorite meal

## 12.3 Spendable Rewards

Deduct stars when redeemed.

Example:

```text
Reward cost: 10 stars
Balance before: 14
Balance after redemption: 4
```

## 12.4 Milestone Rewards

Unlock based on accumulated progress without deducting stars.

Example:

```text
Unlock after 25 lifetime stars
```

Spendable rewards may exist alongside daily and term goals.

---

# 13. Reward Lifecycle

A reward may move through the following states:

1. Draft
2. Active
3. Available
4. Selected as goal
5. In progress
6. Ready
7. Requested
8. Parent approved
9. Scheduled
10. Redeemed
11. Completed
12. Archived
13. Declined
14. Expired

The child may request a reward, but the parent remains responsible for final approval.

---

# 14. Child Profiles

Each child profile may contain:

* Name
* Profile image
* Avatar
* Age group
* Display color
* Current star balance
* Lifetime stars
* Daily reward
* Term reward
* Current term
* Current term progress
* Assigned tasks
* Active warnings
* Active consequences
* Display permissions
* Attendance settings
* Reward visibility settings

Sibling profiles should remain separate.

The initial version should not include a competitive sibling leaderboard.

---

# 15. Tasks and Routines

Parents create tasks through the web UI.

Each task may include:

* Internal name
* Child-facing image
* Optional child-facing audio
* Category
* Star value
* Daily progress value
* Assigned children
* Schedule
* Availability window
* Due time
* Parent approval requirement
* Automatic approval setting
* Required status
* Display priority
* Active status

Example tasks:

* Brush teeth
* Put toys away
* Get dressed
* Feed pet
* Put clothes in basket
* Complete bedtime routine
* Practice colors
* Reading time
* Gentle hands
* Help clean

---

# 16. Task Completion Workflow

1. Parent creates and assigns a task in the web UI.
2. The task appears as an image on the child’s screen.
3. The child or parent taps the task.
4. The child marks it complete.
5. The task enters `awaiting_parent` when approval is required.
6. The web UI displays the pending request.
7. The parent approves or rejects it.
8. The server records the result.
9. Stars and progress are updated.
10. The screen shows visual feedback.

Possible task states:

* Unavailable
* Available
* In progress
* Completion requested
* Awaiting parent
* Approved
* Rejected
* Skipped
* Excused
* Missed
* Not required today

---

# 17. Discipline Philosophy

Discipline should be:

* Clear
* Calm
* Direct
* Related to the behavior
* Corrective
* Time-limited or action-limited
* Parent-controlled
* Private to the relevant child

The system should not use:

* Public shame
* Negative leaderboards
* Permanent bad-behavior labels
* Automatic harsh escalation
* Frightening imagery
* Competitive discipline comparisons

---

# 18. Rules

Parents may create reusable household rules.

Each rule may include:

* Parent-facing name
* Child-facing icon
* Short child-facing meaning
* Parent description
* Default severity
* Default warning behavior
* Suggested consequence
* Suggested recovery action
* Applicable children
* Active status

Examples:

* Gentle hands
* Toys go away
* Stay near a parent
* No throwing food
* Listen during safety instructions
* Use words
* Inside voice
* Bedtime routine

---

# 19. Warning and Discipline Flow

A standard discipline flow may include:

1. Reminder
2. Warning
3. Consequence
4. Corrective action
5. Resolution

Parents may skip levels for safety-related or severe incidents.

## Reminder

A simple prompt that an expectation needs to be followed.

## Warning

A clear indication that continued behavior will lead to a consequence.

## Consequence

A specific result connected to the behavior.

## Correction

An action the child can complete to repair or improve the situation.

## Resolution

The consequence ends and the incident closes.

---

# 20. Consequences

Possible consequence types include:

* Warning only
* Temporary privilege pause
* Temporary item restriction
* Required corrective task
* Calm-down period
* Activity pause
* Parent-defined action
* Time-based consequence
* Action-based consequence
* Combined time-and-action consequence

Examples:

```text
Tablet paused for 20 minutes
```

```text
Toy remains paused until toys are cleaned up
```

```text
Calm down with parent, then try again
```

Each consequence may contain:

* Child
* Related rule
* Related incident
* Child-facing icon
* Parent-facing explanation
* Child-facing explanation
* Start time
* End time
* Required corrective action
* Status
* Issuing parent
* Resolution
* Resolution timestamp

---

# 21. Behavior Incidents

Behavior incidents are stored separately from star transactions.

Each incident may include:

* Child
* Rule
* Severity
* Parent-facing description
* Child-facing message
* Warning level
* Related consequence
* Related recovery action
* Issuing parent
* Timestamp
* Acknowledgment status
* Resolution status
* Resolution timestamp

Detailed parent notes should never be sent to the child-facing screen.

---

# 22. Web UI Proposal

The web UI remains the only editing and administration interface.

A new primary navigation section should be added:

```text
Rewards & Behavior
```

Suggested pages:

1. Overview
2. Children
3. Tasks & Routines
4. Approvals
5. Rewards
6. Reward Terms
7. Rules & Discipline
8. Star History
9. Display Settings

---

# 23. Web UI: Overview

The overview page provides a parent-level summary.

For each child, show:

* Current stars
* Daily progress
* Daily reward
* Current term
* Term reward
* Term progress
* Pending task approvals
* Pending reward requests
* Active warnings
* Active consequences
* Recent activity

Example:

```text
Three-Year-Old

Today
Daily reward: Park
Daily status: Ready for approval
Tasks complete: 3 of 4

Current visit
Term reward: Swimming
Progress: 2 of 3 attended days
Successful days: 2
Status: On track
```

```text
Two-Year-Old

Today
Daily reward: Choose a show
Progress: 2 of 3

Current term
Term duration: 3 days
Successful days: 1 of 2 required
Term reward: Playground
Status: In progress
```

The page should prioritize actions requiring parent attention.

---

# 24. Web UI: Children

The Children page manages profiles.

Parent actions include:

* Add child
* Edit profile
* Upload profile image
* Choose avatar
* Set age mode
* Set attendance behavior
* Configure screen visibility
* Configure allowed rewards
* Configure term defaults
* Archive profile

Profiles should be archived rather than deleted when historical records exist.

---

# 25. Web UI: Tasks and Routines

Parents create reusable tasks and routine groups.

Features include:

* Task editor
* Routine builder
* Child assignment
* Schedule editor
* Star value
* Daily progress value
* Required or optional status
* Parent approval requirement
* Icon or image upload
* Display ordering
* Active dates
* Exceptions

Routine examples:

* Morning routine
* Bedtime routine
* Cleanup routine
* Learning time
* Leaving-the-house routine

---

# 26. Web UI: Approvals

The Approvals page contains frequent parent actions.

## Task Approval Queue

Each card should show:

* Child
* Task
* Completion time
* Star value
* Daily progress impact
* Term impact

Actions:

* Approve
* Reject
* Approve with edited value
* Excuse
* Add note

## Reward Request Queue

Each card should show:

* Child
* Reward
* Reward type
* Cost
* Current balance
* Availability
* Requested time

Actions:

* Approve
* Decline
* Schedule
* Mark completed

## Daily Status Queue

Parents may approve:

* Successful day
* Partial day
* Recovery completed
* Unsuccessful day
* Excused day

---

# 27. Web UI: Rewards

Parents configure the reward catalog.

Each reward may include:

* Parent-facing name
* Child-facing image
* Optional child-facing audio
* Reward type
* Star cost
* Daily or term eligibility
* Available children
* Repeatability
* Usage limit
* Availability schedule
* Approval requirement
* Display priority
* Active status

Parents should be able to reorder rewards.

---

# 28. Web UI: Reward Terms

Each child receives configurable term settings.

Fields include:

* Term enabled
* Parent-facing term name
* Term type
* Duration
* Scheduled days
* Attendance behavior
* Required successful days
* Required progress markers
* Partial progress allowed
* Recovery allowed
* Excused-day handling
* Reset behavior
* Daily reward
* Term reward
* Reward availability timing
* Parent approval requirement
* Automatic restart behavior

Parent controls include:

* Start term
* Pause term
* Resume term
* Extend term
* End term
* Mark day successful
* Mark day partial
* Mark child absent
* Excuse day
* Add progress
* Remove incorrect progress
* Change reward
* Reset term

Every manual change should be recorded.

---

# 29. Web UI: Rules and Discipline

This area should include:

## Rules Tab

* Create rule
* Edit rule
* Assign icon
* Set default severity
* Set suggested consequence
* Set suggested recovery action

## Incidents Tab

* Record incident
* Add parent note
* Select rule
* Choose severity
* Add warning
* Add consequence
* Mark resolved

## Active Consequences Tab

* View active consequences
* Modify end time
* Mark corrective action complete
* Resolve consequence
* Cancel incorrect consequence

## History Tab

* Filter by child
* Filter by rule
* Filter by date
* Review resolution
* Review parent action

---

# 30. Web UI: Star History

The Star History page provides a complete ledger.

Columns may include:

* Date and time
* Child
* Transaction type
* Amount
* Reason
* Related task
* Related reward
* Issuing parent
* Balance after transaction

Actions include:

* View transaction
* Reverse incorrect transaction
* Add administrative correction
* Filter history
* Export later if needed

Transactions should not be silently edited.

---

# 31. Web UI: Display Settings

Parents control what appears on the physical Family Hub screen.

Settings may include:

* Enable reward module
* Show child profiles
* Show exact star totals
* Show only visual progress
* Show daily reward
* Show term reward
* Show task completion buttons
* Allow child reward selection
* Allow reward requests
* Show active consequence indicators
* Show celebration animations
* Enable sounds
* Default profile timeout
* Return-to-home timeout
* Reward card ordering
* Maximum visible reward cards
* Image quality preset
* Cache refresh behavior

---

# 32. Family Hub Screen Proposal

The Family Hub screen should remain simple.

Primary screens:

1. Family Hub reward summary
2. Child selection
3. Child dashboard
4. Tasks
5. Rewards
6. Reward-ready state
7. Correction or consequence screen

---

# 33. Screen: Home Module

The main Family Hub dashboard should show a compact reward module.

Possible content:

* Child profile image
* Current daily reward image
* Current term reward image
* Simple progress markers
* Reward-ready indicator

The module should not show:

* Parent notes
* Detailed incidents
* Full transaction history
* Configuration controls

---

# 34. Screen: Child Selection

The screen should show large child profile images.

Each profile may include:

* Child photo
* Assigned color
* Simple visual status
* Reward-ready indicator

The child should identify their profile visually.

---

# 35. Screen: Child Dashboard

The child dashboard may show:

* Profile image
* Current stars
* Daily reward
* Daily progress
* Term reward
* Term progress
* Today’s tasks
* Active correction indicator

Example:

```text
[Child photo]

Today
[Park image]
● ● ○

Big reward
[Swimming image]
● ● ○

Tasks
[Toothbrush] [Toy bin] [Clothes basket]
```

The child-facing interface should not require understanding written dates or schedules.

---

# 36. Screen: Tasks

Each task appears as a large image card.

A task card may show:

* Task image
* Star value
* Completion status
* Parent approval status

When tapped:

1. Show large task image.
2. Show a large completion button.
3. Submit completion request.
4. Display waiting state.
5. Update after parent approval.

---

# 37. Screen: Rewards

The reward screen displays only rewards approved for that child.

Each reward card may show:

* Reward image
* Visual cost
* Available or locked state
* Selected-goal state
* Ready state

The child may:

* View rewards
* Select an eligible goal
* Request a ready reward

The child cannot:

* Change reward costs
* Add rewards
* Approve redemption
* Bypass availability rules

---

# 38. Screen: Daily and Term Progress

Daily and term progress should be visually separate.

Example:

```text
Today
🌳
● ● ● ○

Big reward
🏊
● ● ○
```

For shorter terms, use large progress circles.

For longer terms, use:

* Smaller circles
* Progress bar
* Ring indicator

The display should adapt to the configured term length.

---

# 39. Screen: Reward Ready

When a reward becomes ready:

* Reward image becomes prominent
* Progress fills completely
* Simple celebration plays
* Parent approval icon appears
* Child may submit a request

The screen should then show:

```text
Requested
Waiting for parent
```

---

# 40. Screen: Discipline and Correction

The screen should show only the child-facing information.

Possible elements:

* Rule icon
* Short phrase
* Pause icon
* Consequence image
* Timer
* Corrective action image
* Completion status

Example:

```text
Gentle hands
✋

Toy paused
⏸️

Try again after calm-down
```

The panel should never show the parent’s detailed description of the incident.

---

# 41. Surface Separation

## Web UI Only

* Editing profiles
* Uploading images
* Creating tasks
* Creating rewards
* Setting costs
* Creating schedules
* Configuring terms
* Writing parent notes
* Approving tasks
* Approving rewards
* Creating consequences
* Reviewing history
* Reversing transactions
* Managing permissions
* Editing display behavior

## Screen UI Only

* Child profile selection
* Visual task completion
* Visual reward selection
* Reward request
* Immediate progress display
* Celebration animation
* Simple active correction display

## Both Surfaces

The same data may appear differently.

| Data               | Web UI                   | Family Hub Screen         |
| ------------------ | ------------------------ | ------------------------- |
| Child              | Full editable profile    | Photo and visual identity |
| Stars              | Exact balance and ledger | Star or progress display  |
| Task               | Full editor and schedule | Current visual task       |
| Reward             | Full configuration       | Image and progress        |
| Term               | Detailed configuration   | Visual markers            |
| Completion request | Approval controls        | Waiting state             |
| Incident           | Detailed record          | Current rule icon         |
| Consequence        | Full controls            | Simple active correction  |

---

# 42. Proposed Data Model

## children

```text
id
display_name
profile_asset_id
age_mode
display_color
active
created_at
updated_at
```

## child_attendance_rules

```text
id
child_id
attendance_type
scheduled_days
manual_attendance_enabled
created_at
updated_at
```

## task_definitions

```text
id
name
child_facing_label
media_asset_id
category
default_star_value
default_daily_progress
requires_approval
active
created_at
updated_at
```

## task_assignments

```text
id
task_id
child_id
schedule_type
schedule_config
available_at
due_at
required
display_order
active
```

## task_completion_requests

```text
id
assignment_id
child_id
requested_at
requested_from
status
approved_by
approved_at
rejection_reason
```

## star_transactions

```text
id
child_id
amount
transaction_type
category
reason
task_completion_id
reward_redemption_id
issued_by
reversal_of
created_at
```

## reward_definitions

```text
id
name
media_asset_id
reward_type
star_cost
repeatable
approval_required
active
created_at
updated_at
```

## reward_child_access

```text
id
reward_id
child_id
available
display_order
custom_cost
```

## reward_terms

```text
id
child_id
name
term_type
term_config
required_successful_days
required_progress
allow_partial
allow_recovery
daily_reward_id
term_reward_id
status
starts_at
ends_at
created_at
updated_at
```

## reward_term_days

```text
id
term_id
calendar_date
attendance_status
day_status
progress_value
approved_by
approved_at
note
```

## child_reward_goals

```text
id
child_id
reward_id
goal_type
status
selected_at
completed_at
```

## reward_redemptions

```text
id
child_id
reward_id
term_id
requested_at
status
approved_by
approved_at
scheduled_for
completed_at
star_cost
```

## behavior_rules

```text
id
name
child_facing_label
media_asset_id
parent_description
default_severity
active
created_at
updated_at
```

## behavior_incidents

```text
id
child_id
rule_id
severity
parent_note
child_message
recorded_by
created_at
acknowledged_at
resolved_at
status
```

## consequences

```text
id
incident_id
consequence_type
media_asset_id
child_message
parent_description
starts_at
ends_at
completion_requirement
completed_at
resolved_by
status
```

## media_assets

```text
id
asset_type
original_path
panel_path
thumbnail_path
mime_type
width
height
version
created_at
```

## display_settings

```text
id
device_id
reward_module_enabled
show_star_totals
show_daily_reward
show_term_reward
allow_task_requests
allow_reward_selection
allow_reward_requests
sound_enabled
animation_enabled
updated_at
```

---

# 43. API Proposal

Administrative endpoints should remain separate from display endpoints.

## Administrative API

```text
GET    /api/v1/admin/rewards-behavior/overview

GET    /api/v1/admin/children
POST   /api/v1/admin/children
PATCH  /api/v1/admin/children/:id

GET    /api/v1/admin/tasks
POST   /api/v1/admin/tasks
PATCH  /api/v1/admin/tasks/:id

GET    /api/v1/admin/task-approvals
POST   /api/v1/admin/task-approvals/:id/approve
POST   /api/v1/admin/task-approvals/:id/reject

GET    /api/v1/admin/rewards
POST   /api/v1/admin/rewards
PATCH  /api/v1/admin/rewards/:id

GET    /api/v1/admin/reward-terms
POST   /api/v1/admin/reward-terms
PATCH  /api/v1/admin/reward-terms/:id

POST   /api/v1/admin/reward-terms/:id/day-status
POST   /api/v1/admin/reward-terms/:id/pause
POST   /api/v1/admin/reward-terms/:id/resume
POST   /api/v1/admin/reward-terms/:id/end

GET    /api/v1/admin/reward-requests
POST   /api/v1/admin/reward-requests/:id/approve
POST   /api/v1/admin/reward-requests/:id/decline
POST   /api/v1/admin/reward-requests/:id/complete

GET    /api/v1/admin/rules
POST   /api/v1/admin/rules
PATCH  /api/v1/admin/rules/:id

GET    /api/v1/admin/incidents
POST   /api/v1/admin/incidents
POST   /api/v1/admin/incidents/:id/resolve

GET    /api/v1/admin/consequences
POST   /api/v1/admin/consequences
POST   /api/v1/admin/consequences/:id/complete
POST   /api/v1/admin/consequences/:id/resolve

GET    /api/v1/admin/star-transactions
POST   /api/v1/admin/star-transactions
POST   /api/v1/admin/star-transactions/:id/reverse

GET    /api/v1/admin/display-settings
PATCH  /api/v1/admin/display-settings
```

## Display API

```text
GET  /api/v1/display/rewards-behavior/home
GET  /api/v1/display/children
GET  /api/v1/display/children/:id
GET  /api/v1/display/children/:id/tasks
GET  /api/v1/display/children/:id/rewards
GET  /api/v1/display/children/:id/progress
GET  /api/v1/display/children/:id/active-correction

POST /api/v1/display/actions/complete-task
POST /api/v1/display/actions/select-reward
POST /api/v1/display/actions/request-reward
POST /api/v1/display/actions/acknowledge
```

Display API responses should be compact and contain no private parent notes.

---

# 44. Example Display Payload

```json
{
  "child": {
    "id": "child_3yo",
    "name": "Child",
    "profile_asset": "profile_child_3yo",
    "stars": 8
  },
  "daily": {
    "reward": {
      "id": "reward_park",
      "asset": "reward_park_96"
    },
    "progress": {
      "current": 3,
      "target": 4
    },
    "status": "in_progress"
  },
  "term": {
    "type": "attended_day",
    "reward": {
      "id": "reward_swimming",
      "asset": "reward_swimming_128"
    },
    "progress": {
      "current": 2,
      "target": 3
    },
    "status": "on_track"
  },
  "tasks": [
    {
      "id": "task_brush_teeth",
      "asset": "task_toothbrush_96",
      "status": "available",
      "star_value": 1
    }
  ],
  "active_correction": null
}
```

---

# 45. Offline Behavior

The server remains authoritative.

When offline, the screen may show cached:

* Child profiles
* Last known star totals
* Reward images
* Task images
* Daily progress
* Term progress
* Selected reward

The screen should clearly display an offline indicator.

V0.1 should disable offline writes for:

* Task completion
* Reward selection
* Reward redemption
* Parent approvals
* Discipline actions
* Consequence resolution

This avoids duplicate or conflicting transactions.

---

# 46. Media and Image Handling

Because the panel is under PSRAM pressure, images must be controlled.

The server should:

* Store original uploads
* Generate panel-sized versions
* Generate thumbnails
* Version media files
* Return compact asset references

The panel should:

* Cache panel-sized assets locally
* Load only visible images
* Release decoded images when leaving a screen
* Avoid loading the full reward catalog at startup
* Reuse shared icons
* Avoid full-resolution photos

Reward and task images should be generated at the actual required display size.

---

# 47. ESP32 Performance Requirements

The add-on must follow these constraints:

* Render only the active screen.
* Do not retain every child screen in memory.
* Do not retain every reward image in decoded form.
* Request child details only after profile selection.
* Request tasks only when needed.
* Request reward catalog only when the rewards view opens.
* Use compact display-specific JSON.
* Clear temporary JSON after parsing.
* Reuse existing screen components.
* Avoid large animations.
* Limit visible reward cards.
* Cache assets on local storage when available.

The screen UI should remain a presentation and interaction layer, not a full administrative application.

---

# 48. Permissions

## Parent or Administrator

Can:

* Manage children
* Create tasks
* Configure terms
* Create rewards
* Approve tasks
* Approve rewards
* Award stars
* Correct mistakes
* Record incidents
* Create consequences
* Resolve consequences
* View full history
* Edit display settings

## Child

Can:

* Select their profile
* View tasks
* Mark eligible tasks complete
* View progress
* Select eligible rewards
* Request rewards
* View active correction instructions

## Household Display

Can:

* Show positive summary information
* Show child profile entry
* Avoid private parent notes
* Avoid administrative changes

---

# 49. Audit and Data Integrity

Important changes should be auditable.

Audit events should include:

* Star award
* Star correction
* Star reversal
* Task approval
* Task rejection
* Daily status change
* Term progress change
* Reward request
* Reward approval
* Reward completion
* Incident creation
* Consequence creation
* Consequence resolution
* Term reset
* Manual attendance change

Records should be corrected through reversal or status transitions rather than silent deletion.

---

# 50. V0.1 Scope

The first release should include:

1. Separate child profiles
2. Image-based profile selection
3. Image-based tasks
4. Star awards
5. Parent task approval
6. Star transaction ledger
7. Daily reward configuration
8. Daily progress tracking
9. Configurable reward terms
10. Fixed-length terms
11. Attended-day terms
12. Calendar-week terms
13. Child-specific term duration
14. Term progress markers
15. Daily and term rewards
16. Image-based reward selection
17. Reward requests
18. Parent reward approval
19. Basic household rules
20. Reminders
21. Warnings
22. Simple consequences
23. Corrective actions
24. Consequence resolution
25. Parent web dashboard
26. Compact display API
27. Child-facing screen module
28. Parent PIN protection where already supported
29. Panel-sized media generation
30. Controlled screen lifecycle

---

# 51. Explicitly Excluded From V0.1

The first version should not include:

* AI behavior analysis
* Automatic punishment decisions
* Behavioral predictions
* School integrations
* Public sibling leaderboards
* Negative rankings
* Real-money banking
* Complex allowance accounting
* Automated star penalties
* Star expiration
* Full offline write synchronization
* Remote discipline by outside users
* Social sharing
* Permanent negative labels
* Advanced voice assistant control
* Detailed child-facing text explanations

---

# 52. Future Expansion

Possible later additions:

* Parent-recorded audio prompts
* Spoken task names
* Spoken reward names
* Older-child reading mode
* Allowance and savings goals
* Family-wide shared goals
* Chore rotation
* Calendar integration
* Mobile notifications
* Multiple parent accounts
* Parent approval from phone
* Reward scheduling
* School-age weekly summaries
* Printable progress reports
* Optional analytics
* Accessibility modes
* Additional display devices

---

# 53. Implementation Plan

## Phase 1: Domain and Database

Build:

* Child reward settings
* Tasks
* Star ledger
* Rewards
* Reward terms
* Term-day tracking
* Reward requests
* Rules
* Incidents
* Consequences
* Display settings

### Phase 1 Acceptance Criteria

* Each child can have a different term duration.
* Attended days can be excluded or included correctly.
* Stars are transaction-based.
* Discipline does not automatically remove stars.
* Terms can be started, paused, completed, and reset.
* Daily and term progress are independently stored.

---

## Phase 2: Administrative API

Build:

* CRUD endpoints
* Task approvals
* Reward approvals
* Daily status controls
* Term controls
* Discipline controls
* Star correction and reversal
* Display settings

### Phase 2 Acceptance Criteria

* All state can be managed through API endpoints.
* Parent notes never appear in display responses.
* Invalid term transitions are rejected.
* Duplicate approvals do not create duplicate stars.
* Reward redemption is atomic.
* Audit records are created.

---

## Phase 3: Web UI

Build:

* Overview
* Children
* Tasks and routines
* Approvals
* Rewards
* Reward terms
* Rules and discipline
* Star history
* Display settings

### Phase 3 Acceptance Criteria

* Parents can configure both children differently.
* Parents can configure a two-day term for one child.
* Parents can configure an attended three-day term for another child.
* Parents can approve tasks and rewards.
* Parents can manually mark days successful, partial, excused, or absent.
* Parents can record and resolve consequences.

---

## Phase 4: Display API

Build compact child-facing endpoints.

### Phase 4 Acceptance Criteria

* Responses contain only screen-required fields.
* No parent notes are returned.
* Payloads remain small.
* Asset references use panel-sized media.
* Child-specific daily and term progress are returned correctly.

---

## Phase 5: Family Hub Screen Module

Build:

* Home reward card
* Child selection
* Child dashboard
* Tasks
* Rewards
* Reward request
* Term progress
* Correction screen
* Celebration feedback

### Phase 5 Acceptance Criteria

* A child can recognize profiles visually.
* A child can complete a task request without reading.
* Daily and term rewards are visually distinct.
* A two-day term renders correctly.
* A three-day term renders correctly.
* The panel does not load unnecessary screens or assets.
* Leaving the module releases temporary screen resources.

---

## Phase 6: Verification

Verify:

* Star integrity
* Reward qualification
* Term calculations
* Attendance handling
* Duplicate request handling
* Reward approval
* Consequence resolution
* Offline display state
* ESP32 memory behavior
* Child-facing usability

### Verification Scenarios

#### Scenario A: Three-Year-Old Visit

```text
Term type: Attended days
Length: 3
Required successful days: 2
Daily reward: Park
Term reward: Swimming
```

Expected result:

* Non-attendance days do not count.
* Two successful attended days qualify the term.
* Swimming becomes available on the final attended day.
* Parent approval is still required.

#### Scenario B: Two-Year-Old Short Term

```text
Term type: Fixed length
Length: 2 days
Required successful days: 2
Term reward: Playground
```

Expected result:

* Two progress markers appear.
* Each approved day fills one marker.
* The reward becomes ready after both days.
* The configuration can later be changed to three days.

#### Scenario C: Recovery

Expected result:

* Parent records a warning.
* Child completes corrective action.
* Parent marks recovery complete.
* Daily progress is restored according to configuration.
* The original incident remains in history.

#### Scenario D: Duplicate Approval

Expected result:

* Approving the same task twice does not award stars twice.

#### Scenario E: Incorrect Star Award

Expected result:

* Parent reverses the transaction.
* Original transaction remains visible.
* Reversal restores the correct balance.

---

# 54. Final Product Definition

The Family Hub Reward and Discipline Add-On is a visual, parent-controlled system for helping young children understand routines, positive behavior, goals, rewards, and correction.

The web UI owns:

* Configuration
* Editing
* Approval
* History
* Administration

The Family Hub screen owns:

* Visual progress
* Child interaction
* Task requests
* Reward selection
* Reward requests
* Immediate feedback
* Simple correction displays

The system supports:

* Immediate stars
* Daily rewards
* Configurable multi-day terms
* Different terms for each child
* Attendance-based schedules
* Parent-defined success
* Recovery after difficult behavior
* Clear consequences
* Image-first communication
* Controlled ESP32 memory use

The implementation should treat reward terms as a general scheduling and progress system rather than hardcoding calendar weeks. This allows the system to fit the two-year-old’s shorter attention span, the three-year-old’s three-day schedule, and future changes as both children grow.
# Child Focus Mode

## 1. Purpose

The Family Hub screen will include a dedicated **Child Focus Mode** designed to reduce visual distractions for young children.

The standard Family Hub dashboard contains household information and navigation intended for adults. The child experience should instead display only:

* The selected child
* The current expectation
* Today’s tasks
* Immediate token or star progress
* Daily reward progress
* Term reward progress
* Any active first-then instruction
* Any active correction

This creates a focused visual environment for the two- and three-year-old without changing the administrative web UI.

---

## 2. Top-Bar Toggle

The existing screen UI currently has an API sync button in the top-center area.

Add a nearby **Child Mode toggle**.

Recommended controls:

```text
[ Sync ]    [ Child Mode ]
   ↻              👧
```

The exact spacing should follow the existing top-bar layout and touch-target requirements.

The child-mode control should use a recognizable icon such as:

* Child profile
* Parent and child
* Star with child profile
* Simple child-face icon

Do not rely only on the words “Child Mode,” since the children cannot read.

### Normal State

```text
Child Mode: Off
```

The screen displays the existing Family Hub interface.

### Active State

```text
Child Mode: On
Selected child: child_id
```

The screen switches to the dedicated child interface.

The active state should be visually obvious to an adult through:

* Highlighted toggle state
* Child profile color
* Selected child avatar
* Optional small locked-mode indicator

---

## 3. Mode Entry Flow

When Child Focus Mode is enabled:

1. Destroy or unload the current normal dashboard screen.
2. Open the child-selection screen.
3. Display large profile images for enabled children.
4. Parent or child selects a profile.
5. Load only that child’s screen data.
6. Render the dedicated child dashboard.
7. Persist the selected mode and child locally.

Example:

```text
Normal Family Hub
        ↓
Tap Child Mode
        ↓
Select Child
        ↓
Dedicated Child Dashboard
```

If only one child profile is currently enabled for the display, the system may optionally skip child selection and open that child’s dashboard directly.

This behavior should be configurable in the web UI.

---

## 4. Dedicated Child Dashboard

Once a child is selected, the screen should no longer display unrelated Family Hub modules.

The dedicated child page should contain only a small number of large visual areas.

### Recommended Layout

```text
┌────────────────────────────────────┐
│ [Child Photo]               [Exit] │
│                                    │
│           CURRENT GOAL             │
│              🌳                    │
│          ● ● ○ ○                   │
│                                    │
│             TODAY                  │
│       [🪥]   [🧸]   [👕]           │
│                                    │
│          BIG REWARD                │
│              🏊                    │
│            ● ● ○                   │
└────────────────────────────────────┘
```

The final implementation should minimize or remove written headings in toddler mode. Icons, layout position, and visual repetition should communicate meaning.

Primary sections:

* Child identity
* Current or first-next task
* Token board
* Today’s tasks
* Daily reward
* Term reward
* Active correction when applicable

---

## 5. Focus Levels

Child Focus Mode should support two display levels.

### Child Dashboard Mode

Shows:

* Child profile
* Today’s task icons
* Daily reward
* Daily progress
* Term reward
* Term progress

This is the child’s main page.

### Single-Goal Focus Mode

Shows only one active expectation and one reward.

Example:

```text
FIRST
[🧸 Toys in bin]

THEN
[🌳 Park]
```

Or:

```text
[Current reward image]

⭐ ⭐ ○
```

Single-Goal Focus Mode is useful during:

* Transitions
* Cleanup
* Bedtime
* Getting dressed
* Leaving the house
* Corrective recovery
* Short teaching periods

A parent starts or configures the active first-then instruction through the web UI.

The panel may also allow entry into Single-Goal Focus Mode by tapping a task card.

---

## 6. Child Navigation

Child-mode navigation should be deliberately limited.

Allowed child actions:

* Open a task
* Mark an eligible task complete
* View token progress
* View daily reward progress
* View term reward progress
* Select an allowed reward
* Request an available reward
* Return from task detail to the child dashboard

Disallowed child actions:

* Open the normal Family Hub dashboard
* Change settings
* Edit tasks
* Change star values
* Approve tasks
* Approve rewards
* Dismiss consequences
* Change term settings
* Switch profiles without permission, when profile lock is enabled
* Trigger administrative API actions

The interface should avoid menus, nested settings, and unrelated navigation.

---

## 7. Exiting Child Focus Mode

A normal single tap should not immediately exit Child Focus Mode.

Toddlers will likely touch the top bar accidentally, so leaving the mode should require an adult-oriented action.

Recommended exit flow:

1. Press and hold the exit or Child Mode button.
2. Display the existing admin PIN interface.
3. Validate the PIN.
4. Destroy the child screen.
5. Restore the normal Family Hub dashboard.

Example:

```text
Hold Parent/Exit button
        ↓
Enter ADMIN_PIN
        ↓
Return to normal dashboard
```

Alternative configurable exit methods may include:

* Long press only
* Long press plus PIN
* Hidden corner sequence plus PIN
* Parent unlock from web UI

The default should be **long press plus PIN**.

---

## 8. Child Profile Switching

Profile switching should be configurable.

### Open Switching

Children may return to the child-selection screen and choose another profile.

### Parent-Locked Profile

The selected child remains active until an adult unlocks the profile.

This is useful when:

* One child is currently using the screen
* Sibling tapping becomes distracting
* Active reward states should remain isolated
* A first-then session is in progress

Recommended default:

* Child selection is open when initially entering Child Mode.
* Once selected, changing profiles requires a long press or parent PIN.

---

## 9. Sync Behavior

Child Focus Mode must remain synchronized with the API, but the child should not need to interact with the sync control.

### Normal Dashboard

The existing manual sync button remains available.

### Child Focus Mode

Use:

* Automatic periodic refresh
* Refresh after child actions
* Refresh after task approval
* Refresh after reward approval
* Refresh after term progress changes
* Refresh after consequence changes

The manual sync button may be hidden in child mode to reduce distraction.

A small non-interactive status indicator can show:

* Connected
* Syncing
* Offline
* Last-known state

Example:

```text
● Connected
↻ Syncing
○ Offline
```

The child should not be able to repeatedly trigger network requests by tapping a sync button.

If manual sync remains visible, it should be parent-protected or rate-limited.

---

## 10. Child Mode Persistence

Child Focus Mode should survive normal interruptions.

Persist locally:

```text
child_mode_enabled
selected_child_id
active_child_page
last_successful_sync
```

Recommended behavior:

* Screen sleep and wake: return to the selected child page.
* Temporary network failure: retain the cached child page.
* API refresh: remain in Child Mode.
* Soft UI reload: restore Child Mode.
* Full device restart: configurable.

### Restart Setting

The web UI should provide:

```text
After restart:
- Return to normal dashboard
- Restore previous mode
- Always start in Child Mode
```

Recommended default for the mounted household panel:

```text
Restore previous mode
```

---

## 11. Web UI Configuration

Add a **Child Focus Mode** section to the existing display settings.

Configurable settings:

* Enable Child Focus Mode
* Show Child Mode toggle
* Child profiles available on this panel
* Default selected child
* Skip child selection when only one child is enabled
* Require PIN to exit
* Require PIN to change child
* Restore Child Mode after restart
* Auto-return to child dashboard after inactivity
* Child dashboard timeout
* Show exact star count
* Show visual progress only
* Show daily reward
* Show term reward
* Show task grid
* Allow child task completion
* Allow child reward selection
* Allow child reward requests
* Show active corrections
* Enable sounds
* Enable animations
* Automatic sync interval

The web UI remains the only surface where these options are edited.

---

## 12. API Requirements

The API should provide a compact Child Focus Mode state.

Example endpoint:

```http
GET /api/v1/display/child-mode/:childId
```

Example response:

```json
{
  "mode": "child_focus",
  "child": {
    "id": "child_3yo",
    "profile_asset": "child_3yo_profile",
    "display_color": "purple"
  },
  "focus": {
    "type": "dashboard",
    "active_task_id": null
  },
  "daily": {
    "reward_asset": "reward_park",
    "current": 2,
    "target": 3,
    "status": "in_progress"
  },
  "term": {
    "reward_asset": "reward_swimming",
    "current": 2,
    "target": 3,
    "status": "on_track"
  },
  "tasks": [
    {
      "id": "brush_teeth",
      "asset": "task_toothbrush",
      "status": "available",
      "can_complete": true
    },
    {
      "id": "put_toys_away",
      "asset": "task_toy_bin",
      "status": "awaiting_parent",
      "can_complete": false
    }
  ],
  "active_correction": null,
  "version": 12
}
```

The response should exclude:

* Parent notes
* Administrative settings
* Full task schedules
* Full reward definitions
* History
* Sibling discipline data
* Audit information

---

## 13. Memory and Screen Lifecycle

Child Focus Mode should not keep the normal dashboard and child dashboard fully instantiated at the same time.

Mode transition:

```text
Unload normal screen
        ↓
Release normal screen assets
        ↓
Fetch compact child payload
        ↓
Build child screen
```

Exit transition:

```text
Unload child screen
        ↓
Release child images and temporary data
        ↓
Fetch current home payload
        ↓
Rebuild normal dashboard
```

Requirements:

* Only the active mode’s primary screen tree remains loaded.
* Child reward images are loaded only when needed.
* Task detail views replace or temporarily suspend the task grid.
* Temporary JSON is released after state extraction.
* Navigation between child screens must not create progressive memory growth.
* Returning to the child dashboard should reuse compact state rather than duplicating it.

The mode switch should reduce distraction without increasing persistent PSRAM usage.

---

## 14. Active Correction Priority

When a child has an active correction or first-then instruction, that content may temporarily override the normal child dashboard.

Priority order:

```text
Safety correction
→ Active first-then instruction
→ Current task focus
→ Child dashboard
```

Example:

```text
FIRST
[🧸 Put toys away]

THEN
[▶️ Resume show]
```

Once the parent resolves the correction through the web UI, the screen returns to the normal child dashboard.

The software should not automatically trap the child on a punishment screen indefinitely. All correction states require an explicit end condition.

---

## 15. Acceptance Criteria

Child Focus Mode is complete when:

1. The normal screen contains a Child Mode toggle.
2. Activating it opens child selection or the configured default child.
3. Each child has a dedicated distraction-reduced dashboard.
4. Unrelated Family Hub modules are hidden.
5. The child can view tasks and reward progress without reading.
6. Daily and term rewards remain visually separate.
7. A task can open in a single-goal focus view.
8. Child actions refresh API state safely.
9. Manual sync is hidden or restricted in child mode.
10. Exiting requires the configured adult action.
11. Parent PIN protection works.
12. Child mode and selected profile can persist across screen sleep.
13. Restart behavior follows the configured setting.
14. Parent notes never appear.
15. Switching modes does not leave both screen trees loaded.
16. Repeated mode switching does not cause progressive PSRAM growth.
17. The two-year-old and three-year-old can use different child-page configurations.
18. Display behavior is managed exclusively through the web UI.

---

## 16. Product Definition

Child Focus Mode is a dedicated visual operating mode for the Family Hub screen.

It converts the panel from a general household dashboard into a focused child interface containing only the information and actions relevant to the selected child.

The feature should reduce visual distraction, prevent accidental navigation, preserve parent control, and support short reinforcement loops without duplicating administrative functionality from the web UI.
