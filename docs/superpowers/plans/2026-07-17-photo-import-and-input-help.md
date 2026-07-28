# Photo Import + Input Help Notes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Parents can attach photos to children and rewards directly from their forms, and every input shows a helper note explaining what belongs in it.

**Architecture:** Frontend-only change to the Rewards & Behavior admin UI. A new `mediaField()` widget replaces the raw media-ID text inputs on the child and reward forms, wired to the existing `POST /api/v1/admin/media` upload API via the existing event-delegation handlers. Help notes reuse the existing `options.help` support in `field()`.

**Tech Stack:** Vanilla JS template strings (`web/js/rewards-admin.js`), `ParentAPI` client (`web/js/rewards-api.js`, unchanged), CSS (`web/css/rewards-admin.css`), node:test string-assertion tests (`server/tests/web-admin.test.js`).

## Global Constraints

- **This project is NOT a git repository — skip all commit steps.**
- No server-side changes. `ASSET_TYPES` already includes `child_profile` and `reward`; `ParentAPI.media.upload(assetType, file)` returns a DTO with `id`.
- Inline upload/select/clear must NOT call `render()` on success — a re-render rebuilds the forms and destroys unsaved edits. Update the widget's DOM in place and push the new asset into `state.data.media` silently.
- Follow the file's existing style: template literals, `esc()` for all interpolated values, `data-ra-*`/`data-media-*` attributes for delegation, no inline `onclick=`.

---

### Task 1: `mediaField()` widget on child + reward forms

**Files:**
- Modify: `web/js/rewards-admin.js` (add `mediaField()` near `mediaPicker()` ~line 124; replace line 139 in `childForm()` and the media field in `rewardForm()` ~line 248; extend `onChange()` and `onClick()`)
- Modify: `web/css/rewards-admin.css` (append widget styles)
- Test: `server/tests/web-admin.test.js`

**Interfaces:**
- Consumes: `mediaThumb(id)`, `esc()`, `state.data.media`, `ParentAPI.media.upload(assetType, file)` → `{ id, assetType, ... }`, `humanError(error)`.
- Produces: `mediaField(label, name, currentId, assetType, help)` returning an HTML string; delegation attributes `data-media-field`, `data-media-upload`, `data-media-select`, `data-media-clear`.

- [ ] **Step 1: Write the failing test** — append to `server/tests/web-admin.test.js`:

```js
test('child and reward forms use the inline photo picker instead of raw media-ID inputs', () => {
  const admin = read('js/rewards-admin.js');
  assert.ok(admin.includes('function mediaField('), 'mediaField widget exists');
  assert.match(admin, /mediaField\('Photo', 'profileAssetId'/);
  assert.match(admin, /mediaField\('Photo', 'mediaAssetId'/);
  assert.doesNotMatch(admin, /field\('Profile media ID'/);
  assert.doesNotMatch(admin, /field\('Image media ID'/);
  assert.match(admin, /accept="image\/png,image\/jpeg,image\/webp"/);
  for (const hook of ['data-media-upload', 'data-media-select', 'data-media-clear']) {
    assert.ok(admin.includes(hook), hook);
  }
  const css = read('css/rewards-admin.css');
  assert.ok(css.includes('.ra-media-field'), 'widget styles present');
});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd server && node --test --test-concurrency=1 tests/web-admin.test.js`
Expected: FAIL — `mediaField widget exists`.

- [ ] **Step 3: Implement the widget** — in `web/js/rewards-admin.js`, directly below `mediaPicker()`:

```js
function mediaField(label, name, currentId, assetType, help) {
  const items = (state.data.media || []).filter((item) => item.assetType === assetType && item.active !== 0);
  return `<div class="ra-field ra-media-field" data-media-field data-asset-type="${esc(assetType)}">
    <span>${esc(label)}</span>
    <input type="hidden" name="${name}" value="${esc(currentId || '')}" />
    <div class="ra-media-field-controls">
      <span class="ra-media-field-preview">${mediaThumb(currentId)}</span>
      <label class="btn secondary ra-media-field-upload">Upload photo<input type="file" accept="image/png,image/jpeg,image/webp" data-media-upload /></label>
      <button class="btn secondary" type="button" data-media-clear ${currentId ? '' : 'hidden'}>Remove photo</button>
      <small class="muted" data-media-note></small>
    </div>
    ${items.length ? `<details class="ra-media-choose"><summary>Choose existing (${items.length})</summary><div class="ra-media-grid">${items.map((item) => `<button type="button" class="ra-media-option" data-media-select="${esc(item.id)}">${mediaThumb(item.id)}</button>`).join('')}</div></details>` : ''}
    <small class="muted">${esc(help)}</small>
  </div>`;
}

function setMediaFieldValue(holder, id) {
  holder.querySelector('input[type="hidden"]').value = id || '';
  holder.querySelector('.ra-media-field-preview').innerHTML = mediaThumb(id);
  holder.querySelector('[data-media-clear]').hidden = !id;
  holder.querySelector('[data-media-note]').textContent = id ? 'Photo selected — save the form to keep it.' : '';
}

async function uploadMediaField(input) {
  const holder = input.closest('[data-media-field]');
  const file = input.files[0];
  if (!file) return;
  const note = holder.querySelector('[data-media-note]');
  note.textContent = 'Uploading…';
  try {
    const asset = await ParentAPI.media.upload(holder.dataset.assetType, file);
    (state.data.media = state.data.media || []).unshift(asset);
    setMediaFieldValue(holder, asset.id);
  } catch (error) {
    note.textContent = '';
    state.error = humanError(error);
    render();
  } finally {
    input.value = '';
  }
}
```

In `childForm()`, replace `${field('Profile media ID', 'profileAssetId', child.profileAssetId || '')}` with:

```js
${mediaField('Photo', 'profileAssetId', child.profileAssetId || '', 'child_profile', 'Shown on the panel — kids find themselves by photo, not name. PNG, JPEG, or WebP; the server makes panel-sized copies automatically.')}
```

In `rewardForm()`, replace `${field('Image media ID', 'mediaAssetId', reward.mediaAssetId || '')}` with:

```js
${mediaField('Photo', 'mediaAssetId', reward.mediaAssetId || '', 'reward', 'The picture kids see for this reward on the panel. A real photo of the actual reward works best.')}
```

In `onChange(event)`, add as the first line:

```js
if (event.target.matches('[data-media-upload]')) return void uploadMediaField(event.target);
```

In `onClick(event)`, add before the `[data-ra-action]` lookup:

```js
const mediaSelect = event.target.closest('[data-media-select]');
if (mediaSelect) return setMediaFieldValue(mediaSelect.closest('[data-media-field]'), mediaSelect.dataset.mediaSelect);
const mediaClear = event.target.closest('[data-media-clear]');
if (mediaClear) return setMediaFieldValue(mediaClear.closest('[data-media-field]'), '');
```

Append to `web/css/rewards-admin.css`:

```css
.ra-media-field-controls { display: flex; align-items: center; gap: 12px; flex-wrap: wrap; }
.ra-media-field-upload { position: relative; overflow: hidden; }
.ra-media-field-upload input[type="file"] { position: absolute; inset: 0; opacity: 0; cursor: pointer; }
.ra-media-choose summary { cursor: pointer; color: var(--acid); margin-top: 8px; }
.ra-media-option { padding: 0; border: 2px solid transparent; border-radius: 12px; background: none; cursor: pointer; }
.ra-media-option:focus-visible, .ra-media-option:hover { border-color: var(--acid); }
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `cd server && node --test --test-concurrency=1 tests/web-admin.test.js`
Expected: PASS (all tests, including pre-existing ones).

Note on empty value: `values()` serializes the hidden input; `clean()` drops `''`, so clearing a photo won't send `profileAssetId: ''`. Removing a photo from an already-photographed child therefore needs `null`, which `clean()` also drops — acceptable for now (matches current behavior of blanking the text field; document in help note text if it comes up).

---

### Task 2: Help notes under every input

**Files:**
- Modify: `web/js/rewards-admin.js` (add `help:` to every `field()` call lacking one; add `<small class="muted">` to hand-rolled inputs in `assignmentEditor()`, `routineTaskEditor()`, `childAccessEditor()`)
- Test: `server/tests/web-admin.test.js`

**Interfaces:**
- Consumes: `field()`'s existing `options.help` rendering (`<small id="help-<name>" class="muted">`).
- Produces: nothing consumed by other tasks.

- [ ] **Step 1: Write the failing test** — append to `server/tests/web-admin.test.js`:

```js
test('every field call carries a help note for parents', () => {
  const admin = read('js/rewards-admin.js');
  const calls = admin.match(/field\('[^']+',\s*'[^']+'/g) || [];
  const withHelp = admin.match(/help:\s*'/g) || [];
  assert.ok(calls.length > 30, `expected many field calls, saw ${calls.length}`);
  assert.ok(withHelp.length >= calls.length, `every field needs help text: ${withHelp.length} helps for ${calls.length} fields`);
  for (const sample of ['1,3,5 = Mon, Wed, Fri', 'Stars earned when', '4–12 digits']) {
    assert.ok(admin.includes(sample), sample);
  }
});
```

- [ ] **Step 2: Run test to verify it fails**

Run: `cd server && node --test --test-concurrency=1 tests/web-admin.test.js`
Expected: FAIL on the help-count assertion.

- [ ] **Step 3: Add help text.** Add a `help:` entry to the options of every `field()` call that lacks one (merge into existing options objects; add `{ help: '…' }` where there are none). Use this text (adjust only if a field was missed here — same tone: one short sentence, concrete example where format matters):

| Form | Field | help text |
|---|---|---|
| Login | Parent PIN | `4–12 digits. Set on the server with npm run set-parent-pin.` |
| Child | Display name | `The name shown across the parent pages and audit history.` |
| Child | Avatar | `Fallback emoji when no photo is set, e.g. 🦖.` |
| Child | Age mode | `Adjusts panel layout hints; toddler mode is fully visual.` |
| Child | Display color | `Accent color for this child on the panel and here.` |
| Child | Attendance | `Attended-day terms only count days the child is present.` |
| Child | Scheduled weekdays | `Comma-separated 0–6, where Sunday is 0. Example: 1,3,5 = Mon, Wed, Fri.` (existing, keep) |
| Child | Allow manual attendance | `Lets you mark present/absent per day under Approvals.` |
| Child | Visible on screen | `Uncheck to hide this child from the panel entirely.` |
| Task | Parent name | `Internal name for lists and approvals; kids never see it.` |
| Task | Child-facing label | `Optional short label the panel may show next to the picture.` |
| Task | Task media ID | `Paste an ID from the media library above (photo picker coming to tasks later).` |
| Task | Category | `Free grouping word, e.g. morning, bedtime, cleanup.` |
| Task | Default stars | `Stars earned when this task is approved.` |
| Task | Daily progress | `How much this task fills the daily goal meter.` |
| Task | Parent approval required | `Checked: completions wait in Approvals. Unchecked: instant stars.` |
| Routine | Routine name | `Internal name, e.g. Bedtime routine.` |
| Routine | Child-facing label | `Optional label the panel may show for the whole routine.` |
| Routine | Task media ID | `Paste an ID from the media library above.` |
| Approvals (task decision) | Stars | `Adjust to award more or fewer stars than the task default.` |
| Approvals (task decision) | Daily progress | `Adjust the daily-goal credit for this completion.` |
| Approvals (task decision) | Parent note | `Private note kept in history; never shown on the panel.` |
| Approvals (reward decision) | Schedule for | `Optional date/time you plan to deliver the reward.` |
| Approvals (reward decision) | Reason / note | `Private note kept with the request history.` |
| Day status | Date | `The day you are recording. Defaults to today.` |
| Day status | Attendance | `Whether the child was present that day.` |
| Day status | Day status | `Successful and partial days move the term forward.` |
| Day status | Progress value | `Term progress markers this day earns (usually 1).` |
| Day status | Parent note | `Private note kept in the term history.` |
| Reward | Name | `Parent-facing name, e.g. Swimming trip.` |
| Reward | Type | `Daily = earned today. Term = multi-day goal. Spendable = costs stars. Milestone = unlocks at lifetime stars.` |
| Reward | Star cost | `Stars deducted when a spendable reward is redeemed. 0 for daily/term rewards.` |
| Reward | Milestone threshold | `Lifetime stars that unlock a milestone reward. Leave blank otherwise.` |
| Reward | Repeatable | `Unchecked: can only ever be redeemed once per child.` |
| Reward | Parent approval required | `Checked: child requests wait for you under Approvals.` |
| Reward | Usage limit | `Maximum redemptions per child. Blank = unlimited.` |
| Reward | Display priority | `Lower numbers show first on the panel.` |
| Reward | Available weekdays | `Comma-separated 0–6. Blank means any day.` (existing, keep) |
| Reward | Available after / until | `Time window when the panel offers this reward. Blank = all day.` |
| Terms (all fields in termForm) | — | one sentence each following the same pattern: what the value does on the panel/term engine, with a concrete example for any formatted value (weekday lists `1,3,5`, dates `2030-01-01`) |
| Rules / incidents / consequences / recovery forms | — | same pattern; note on every child-facing field: `Kids see this on the panel` and on every parent note field: `Private — never sent to the panel.` |
| Star corrections (ledger) | Amount | `Positive adds stars, negative removes. Recorded as an audit-visible correction.` |
| Display settings | every toggle/number | one sentence stating the panel behavior it controls, e.g. sync interval: `Seconds between panel refreshes (15–3600).` |

For the hand-rolled inputs (assignment rows, routine task rows, child access rows), add one `<small class="muted">` per row group (not per input — rows repeat per child) at the bottom of the fieldset legend area, e.g. in `assignmentEditor()` after `<legend>`:

```js
<small class="muted">Check a child to assign the task. Weekdays: 1,3,5 = Mon, Wed, Fri. Overrides beat the task defaults; blank uses the default.</small>
```

In `routineTaskEditor()` after `<legend>`:

```js
<small class="muted">Check tasks to include and order them; Order 0 shows first. Required tasks must be done for the routine to count.</small>
```

In `childAccessEditor()` after `<legend>`:

```js
<small class="muted">Check a child to offer this reward to them. Custom cost overrides the star cost; blank uses the default.</small>
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `cd server && node --test --test-concurrency=1 tests/web-admin.test.js`
Expected: PASS.

---

### Task 3: Full-suite + in-browser verification

**Files:** none modified.

- [ ] **Step 1: Full server test suite**

Run: `cd server && npm test`
Expected: all tests pass (includes web-browser-smoke).

- [ ] **Step 2: Browser verification against the temp server** (server already running on `:3020`)
  1. Open `http://192.168.1.135:3020`, go to Rewards & Behavior, unlock with the parent PIN.
  2. Children → lily → photo widget shows placeholder; Upload photo with a small PNG → preview appears, note says "Photo selected — save the form to keep it." → Save child → reload → photo persists in summary + form.
  3. Rewards → create/edit a reward → attach a photo via Choose existing (the one just uploaded won't be there — it's `child_profile`; upload a fresh one) → save → thumbnail shows in the reward list.
  4. Confirm the panel display payload references the child photo: `curl -H "x-family-hub-token: <derived>" -H "x-family-hub-device-id: familyhub-panel-1B44" http://127.0.0.1:3020/api/v1/display/child-mode/<lily-id>` includes a `mediaRef`/asset for the child.
  5. Spot-check help notes render under inputs in every area tab.
- [ ] **Step 3: Report** — summarize what passed/failed with the actual outputs.
