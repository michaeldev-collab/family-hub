'use strict';

function esc(s) {
  if (window.API && typeof API.esc === 'function') return API.esc(s);
  return String(s ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function memberName(members, id) {
  const m = members.find((x) => x.id === id);
  return m ? m.name : 'Anyone';
}

const MEMBER_COLORS = [
  { value: '#4A90D9', label: 'Blue' },
  { value: '#E67E22', label: 'Orange' },
  { value: '#27AE60', label: 'Green' },
  { value: '#9B59B6', label: 'Purple' },
  { value: '#E74C3C', label: 'Red' },
  { value: '#F1C40F', label: 'Yellow' },
];

const MEMBER_EMOJI_PRESETS = [
  '👤', '👩', '👨', '👧', '👦', '🧑', '👶', '🧒',
  '🐶', '🐱', '🦊', '🐻', '🦁', '🐸', '🦄', '🐼',
  '⭐', '🌟', '💚', '💙', '💜', '🧡', '❤️', '🌈',
  '🏠', '🎮', '📚', '⚽', '🎨', '🎵', '🚗', '✈️',
];

const CHORE_ICON = "url('/assets/icons/chore-item.svg')";

function colorOptions(selected) {
  return MEMBER_COLORS.map(
    (c) => `<option value="${c.value}" ${selected === c.value ? 'selected' : ''}>${c.label}</option>`
  ).join('');
}

function weekDatesFrom(startIso) {
  const dates = [];
  const start = new Date(`${startIso}T12:00:00`);
  for (let i = 0; i < 7; i++) {
    const d = new Date(start);
    d.setDate(start.getDate() + i);
    dates.push(d.toISOString().slice(0, 10));
  }
  return dates;
}

function dinnerDayLabel(dateIso, todayIso) {
  const d = new Date(`${dateIso}T12:00:00`);
  const weekday = d.toLocaleDateString(undefined, { weekday: 'short' });
  const monthDay = d.toLocaleDateString(undefined, { month: 'short', day: 'numeric' });
  if (dateIso === todayIso) return `Today · ${weekday} ${monthDay}`;
  return `${weekday} ${monthDay}`;
}

function planForDate(plans, date) {
  const found = (plans || []).find((p) => p.date === date);
  return found || { date, main: '', side: '', side2: '', meal: '', cookId: null, notes: '' };
}

function planHasMeal(plan) {
  if (!plan) return false;
  return Boolean(
    (plan.main && plan.main.trim()) ||
      (plan.side && plan.side.trim()) ||
      (plan.side2 && plan.side2.trim()) ||
      (plan.meal && plan.meal.trim())
  );
}

function planMealSummary(plan) {
  if (!plan) return '';
  const parts = [plan.main, plan.side, plan.side2]
    .map((p) => (p || '').trim())
    .filter(Boolean);
  if (parts.length) return parts.join(' · ');
  return (plan.meal || '').trim();
}

function weekHasAnyMeal(plans, dates) {
  return dates.some((date) => planHasMeal(planForDate(plans, date)));
}

function cookName(members, cookId) {
  if (!cookId) return 'Unassigned';
  return memberName(members, cookId);
}

function cookOptions(members, selectedId) {
  return `<option value="">Unassigned</option>${members.map(
    (m) => `<option value="${m.id}" ${selectedId === m.id ? 'selected' : ''}>${esc(m.name)}</option>`
  ).join('')}`;
}

function memberById(members, id) {
  return (members || []).find((m) => m.id === id) || null;
}

function memberAvatarHtml(member) {
  const emoji = member?.avatarEmoji || '👤';
  const color = member?.color || 'transparent';
  return `<span class="member-avatar" style="border-color:${esc(color)}">${esc(emoji)}</span>`;
}

function emojiPickerHtml(targetInputId, selectedEmoji) {
  const current = selectedEmoji || '👤';
  return `
    <div class="emoji-picker-block" data-emoji-target="${esc(targetInputId)}">
      <span class="emoji-picker-label">Pick avatar</span>
      <div class="emoji-grid" role="listbox" aria-label="Avatar emoji">
        ${MEMBER_EMOJI_PRESETS.map((emoji) => `
          <button type="button" class="emoji-pick ${emoji === current ? 'selected' : ''}" data-emoji="${esc(emoji)}" aria-label="Avatar ${esc(emoji)}" role="option" aria-selected="${emoji === current ? 'true' : 'false'}">${esc(emoji)}</button>
        `).join('')}
      </div>
    </div>`;
}

function choreIconHtml() {
  return `<span class="list-icon chore-icon" style="--icon-url:${CHORE_ICON}" aria-hidden="true"></span>`;
}

function pendingReminderFor(reminders, entityType, entityId) {
  return (reminders || []).find(
    (r) => r.entityType === entityType && r.entityId === entityId && r.status === 'pending'
  );
}

function reminderKey(entityType, entityId) {
  return `${entityType}:${entityId}`;
}

function isoToDatetimeLocal(iso) {
  if (!iso) return '';
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return '';
  const pad = (n) => String(n).padStart(2, '0');
  return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}T${pad(d.getHours())}:${pad(d.getMinutes())}`;
}

function formatReminderBadge(reminder, members) {
  const at = new Date(reminder.remindAt);
  const timeStr = at.toLocaleString(undefined, { weekday: 'short', hour: 'numeric', minute: '2-digit' });
  const names = (reminder.notifyMemberIds || [])
    .map((id) => memberName(members, id))
    .filter((n) => n !== 'Anyone')
    .join(', ');
  return `Reminder: ${timeStr}${names ? ` → ${names}` : ''}`;
}

function reminderBlockHtml(state, entityType, entityId) {
  const members = state.members || state.membersFull || [];
  const reminder = pendingReminderFor(state.reminders, entityType, entityId);
  const editing = state.editingReminderKey === reminderKey(entityType, entityId);

  if (editing) {
    const remindAt = reminder ? isoToDatetimeLocal(reminder.remindAt) : '';
    const message = reminder ? reminder.message || '' : '';
    const selectedIds = new Set(reminder?.notifyMemberIds || []);
    return `
      <div class="reminder-panel" data-entity-type="${entityType}" data-entity-id="${esc(entityId)}">
        <label class="reminder-field">
          <span class="muted">When</span>
          <input type="datetime-local" data-field="remind-at" value="${esc(remindAt)}" />
        </label>
        <label class="reminder-field">
          <span class="muted">Message (optional)</span>
          <input type="text" data-field="message" maxlength="500" placeholder="Custom reminder text" value="${esc(message)}" />
        </label>
        <fieldset class="reminder-notify">
          <legend class="muted">Notify</legend>
          ${members.length
            ? members.map((m) => `
              <label class="reminder-member-check">
                <input type="checkbox" data-field="member" value="${m.id}" ${selectedIds.has(m.id) ? 'checked' : ''} />
                ${memberAvatarHtml(m)} ${esc(m.name)}
              </label>`).join('')
            : '<span class="muted">Add family members in Setup first.</span>'}
        </fieldset>
        <div class="row-actions reminder-actions">
          <button class="btn" data-action="save-reminder" data-entity-type="${entityType}" data-entity-id="${esc(entityId)}">Save</button>
          <button class="btn secondary" data-action="cancel-reminder-edit">Cancel</button>
        </div>
      </div>`;
  }

  const parts = [];
  if (reminder) {
    parts.push(`<span class="reminder-badge">${esc(formatReminderBadge(reminder, members))}</span>`);
  }
  parts.push(
    `<button class="btn secondary reminder-bell" data-action="edit-reminder" data-entity-type="${entityType}" data-entity-id="${esc(entityId)}" aria-label="Set reminder">${reminder ? 'Edit reminder' : '🔔 Set reminder'}</button>`
  );
  if (reminder) {
    parts.push(
      `<button class="btn secondary" data-action="remove-reminder" data-reminder-id="${reminder.id}">Remove</button>`
    );
  }
  return `<div class="reminder-row">${parts.join('')}</div>`;
}

const Views = {
  home(state) {
    const home = state.home || {};
    const dinnerMeal = home.dinner_today ?? state.dinner?.today?.meal ?? null;
    const dinnerCook = home.dinner_cook ?? state.dinner?.today?.cook_name ?? null;
    const choreList = state.choresFull || state.chores?.items || [];
    const groceryList = (state.groceryFull || state.grocery?.items || []).filter((g) => {
      const key = g.listKey || g.list_key || 'main';
      if (key === 'constant') return !!g.needed;
      return !g.checked;
    });
    const chores = choreList.slice(0, 5);
    const grocery = groceryList.slice(0, 5);
    const pinned = state.pinnedNotes || home.pinned || state.notes?.pinned || [];
    const notes = pinned.length ? pinned : (state.notes?.recent || []).slice(0, 3);
    const members = state.members || [];

    return `
      <section class="card">
        <h2>Tonight's Dinner</h2>
        ${dinnerMeal
          ? `<div class="hero-dinner">${esc(dinnerMeal)}</div>
             <div class="muted">Cook: ${esc(dinnerCook || 'Unassigned')}</div>`
          : `<div class="empty">No dinner planned — set it on the Dinner tab.</div>`}
      </section>

      <section class="card">
        <h2>Chores Today</h2>
        ${chores.length
          ? `<ul class="list">${chores.map((c) => `
              <li>
                ${choreIconHtml()}
                <span>${esc(c.title)}</span>
                <span class="chip">${memberById(members, c.assigneeId)?.avatarEmoji ? `${esc(memberById(members, c.assigneeId).avatarEmoji)} ` : ''}${esc(c.assignee ? c.assignee.name : (c.assignee_name || 'Anyone'))}</span>
              </li>`).join('')}</ul>`
          : `<div class="empty">All caught up!</div>`}
      </section>

      <section class="card">
        <h2>Grocery</h2>
        ${grocery.length
          ? `<ul class="list">${grocery.map((g) => `<li>${esc(g.text)}</li>`).join('')}</ul>`
          : `<div class="empty">List is empty.</div>`}
      </section>

      <section class="card">
        <h2>Notes</h2>
        ${notes.length
          ? `<ul class="list">${notes.map((n) => `<li>${esc(n.text)}</li>`).join('')}</ul>`
          : `<div class="empty">No notes yet.</div>`}
      </section>
    `;
  },

  grocery(state) {
    const rawItems = state.groceryFull || state.grocery?.items || [];
    const items = Array.isArray(rawItems) ? rawItems : [];
    const otherTitle =
      state.groceryOtherTitle || state.grocery?.other_title || state.grocery?.otherTitle || 'Other';
    const byList = (key) => items.filter((g) => (g.listKey || g.list_key || 'main') === key);

    const renderConstant = (list) =>
      list.length
        ? `<ul class="list">${list
            .map(
              (g) => `
              <li data-id="${g.id}">
                <div class="list-item-main">
                  <button class="check ${g.needed ? 'done needed' : ''}" data-action="toggle-grocery-needed" data-id="${g.id}" aria-label="Mark needed"></button>
                  <span class="${g.needed ? 'grocery-needed-text' : ''}">${esc(g.text)}</span>
                  <div class="row-actions">
                    <button class="btn secondary" data-action="delete-grocery" data-id="${g.id}">Delete</button>
                  </div>
                </div>
              </li>`
            )
            .join('')}</ul>`
        : `<div class="empty">No staples yet.</div>`;

    const renderBuyList = (list) =>
      list.length
        ? `<ul class="list">${list
            .map(
              (g) => `
              <li data-id="${g.id}" class="list-item-with-reminder">
                <div class="list-item-main">
                  <button class="check ${g.checked ? 'done' : ''}" data-action="toggle-grocery" data-id="${g.id}" aria-label="Toggle"></button>
                  <span style="${g.checked ? 'text-decoration:line-through;opacity:.6' : ''}">${esc(g.text)}</span>
                  <div class="row-actions">
                    <button class="btn secondary" data-action="delete-grocery" data-id="${g.id}">Delete</button>
                  </div>
                </div>
                ${reminderBlockHtml(state, 'grocery', g.id)}
              </li>`
            )
            .join('')}</ul>`
        : `<div class="empty">No items yet.</div>`;

    return `
      <section class="card grocery-board">
        <h2>Grocery Lists</h2>
        <div class="grocery-compass" role="note" aria-label="How these lists work">
          <p class="muted grocery-compass-lead">How to read this screen</p>
          <ul class="grocery-compass-map">
            <li><span class="grocery-compass-key">Constant</span> staples you always keep — tap the circle when you need more</li>
            <li><span class="grocery-compass-key">Main</span> this week’s store run — tap to check off at the store</li>
            <li><span class="grocery-compass-key">${esc(otherTitle)}</span> third list (rename its title) — same check-off as Main</li>
          </ul>
        </div>
        <div class="grocery-columns">
          <div class="grocery-col" data-list="constant">
            <h3>Constant</h3>
            <p class="muted grocery-col-hint">Staples — mark Needed when you run out</p>
            <div class="input-row">
              <input id="groceryInput-constant" type="text" placeholder="Add staple…" maxlength="200" />
              <button class="btn" data-action="add-grocery" data-list="constant">Add</button>
            </div>
            ${renderConstant(byList('constant'))}
          </div>
          <div class="grocery-col" data-list="main">
            <h3>Main</h3>
            <p class="muted grocery-col-hint">Everyday store list</p>
            <div class="input-row">
              <input id="groceryInput-main" type="text" placeholder="Add item…" maxlength="200" />
              <button class="btn" data-action="add-grocery" data-list="main">Add</button>
            </div>
            ${renderBuyList(byList('main'))}
          </div>
          <div class="grocery-col" data-list="other">
            <div class="grocery-col-title-row">
              <input id="groceryOtherTitle" type="text" maxlength="40" value="${esc(otherTitle)}" aria-label="Third list name" />
              <button class="btn secondary" data-action="save-grocery-other-title">Save name</button>
            </div>
            <p class="muted grocery-col-hint">Configurable third list</p>
            <div class="input-row">
              <input id="groceryInput-other" type="text" placeholder="Add item…" maxlength="200" />
              <button class="btn" data-action="add-grocery" data-list="other">Add</button>
            </div>
            ${renderBuyList(byList('other'))}
          </div>
        </div>
      </section>
    `;
  },

  chores(state) {
    const items = state.choresFull || state.chores?.items || [];
    const members = state.members || [];
    return `
      <section class="card">
        <h2>Chores</h2>
        <div class="input-row">
          <input id="choreInput" type="text" placeholder="New chore…" maxlength="200" />
          <select id="choreAssignee">
            <option value="">Anyone</option>
            ${members.map((m) => `<option value="${m.id}">${esc(m.name)}</option>`).join('')}
          </select>
          <button class="btn" data-action="add-chore">Add</button>
        </div>
        ${items.length
          ? `<ul class="list">${items.map((c) => `
              <li data-id="${c.id}" class="list-item-with-reminder">
                <div class="list-item-main">
                  <button class="check ${c.completed ? 'done' : ''}" data-action="toggle-chore" data-id="${c.id}"></button>
                  ${choreIconHtml()}
                  <span style="${c.completed ? 'text-decoration:line-through;opacity:.6' : ''}">${esc(c.title)}</span>
                  <span class="chip">${memberById(members, c.assigneeId)?.avatarEmoji ? `${esc(memberById(members, c.assigneeId).avatarEmoji)} ` : ''}${esc(memberName(members, c.assigneeId))}</span>
                  <div class="row-actions">
                    <button class="btn secondary" data-action="delete-chore" data-id="${c.id}">Delete</button>
                  </div>
                </div>
                ${reminderBlockHtml(state, 'chore', c.id)}
              </li>`).join('')}</ul>`
          : `<div class="empty">No chores yet.</div>`}
      </section>
    `;
  },

  dinner(state) {
    const members = state.members || [];
    const today = state.today;
    const plans = state.weekDinnerFull || state.weekDinner || [];
    const dates = weekDatesFrom(today);
    const inQueue = state.dinnerWeekInQueue === true;
    const queueDates = new Set(state.dinnerQueueDates || []);

    const savedDateSet = new Set(dates.filter((date) => !queueDates.has(date)));
    plans.forEach((p) => {
      if (p.date && !queueDates.has(p.date)) savedDateSet.add(p.date);
    });
    const savedDateList = [...savedDateSet].sort();

    const savedRows = savedDateList
      .map((date) => {
        const plan = planForDate(plans, date);
        const isToday = date === today;
        const summary = planMealSummary(plan);
        const sides = [plan.side, plan.side2].map((s) => (s || '').trim()).filter(Boolean);
        return `
          <article class="dinner-saved-row ${isToday ? 'dinner-today' : ''}" data-date="${date}">
            <div class="dinner-day-label">${esc(dinnerDayLabel(date, today))}</div>
            <div class="dinner-saved-body">
              <span class="dinner-saved-meal">${summary ? esc(plan.main || summary) : '<span class="muted">No meal planned</span>'}</span>
              <span class="chip">${esc(cookName(members, plan.cookId))}</span>
            </div>
            ${sides.length ? `<div class="dinner-saved-sides muted">${esc(sides.join(' · '))}</div>` : ''}
            ${plan.notes ? `<div class="dinner-saved-notes muted">${esc(plan.notes)}</div>` : ''}
            ${reminderBlockHtml(state, 'dinner', date)}
            <div class="row-actions">
              <button class="btn secondary" data-action="edit-dinner-day" data-date="${date}">Edit</button>
            </div>
          </article>`;
      })
      .join('');

    const queueDateList = [...queueDates].sort();
    const queueRows = queueDateList
      .map((date) => {
        const plan = planForDate(plans, date);
        const isToday = date === today;
        return `
          <article class="dinner-day-row dinner-queue-row ${isToday ? 'dinner-today' : ''}" data-date="${date}">
            <div class="dinner-day-label-row">
              <label class="dinner-date-field">
                <span class="muted">Day</span>
                <input type="date" class="dinner-date" value="${date}" />
              </label>
              <span class="dinner-day-hint">${date === today ? 'Today' : esc(dinnerDayLabel(date, today))}</span>
              <span class="queue-badge">Planning</span>
            </div>
            <div class="dinner-meal-fields">
              <input class="dinner-main" type="text" placeholder="Main" maxlength="120" value="${esc(plan.main || '')}" />
              <input class="dinner-side" type="text" placeholder="Side" maxlength="120" value="${esc(plan.side || '')}" />
              <input class="dinner-side2" type="text" placeholder="Second side (optional)" maxlength="120" value="${esc(plan.side2 || '')}" />
            </div>
            <div class="input-row dinner-day-meta">
              <select class="dinner-cook" aria-label="Who's cooking">${cookOptions(members, plan.cookId)}</select>
              <button class="btn secondary" data-action="save-dinner-day" data-date="${date}">Save day</button>
            </div>
            <div class="input-row">
              <textarea class="dinner-notes" placeholder="Notes (optional)" maxlength="500">${esc(plan.notes || '')}</textarea>
            </div>
          </article>`;
      })
      .join('');

    const queueSection = queueRows
      ? `<div class="dinner-queue">
          <h3 class="dinner-section-title">Planning queue</h3>
          <p class="muted">Pick the day, fill in main and sides, then save to add it to your plan.</p>
          <div class="dinner-week">${queueRows}</div>
          <div class="input-row dinner-week-actions">
            <button class="btn" data-action="save-dinner-week">Save whole week</button>
          </div>
        </div>`
      : '';

    const savedSection = savedRows
      ? `<div class="dinner-saved">
          <h3 class="dinner-section-title">This week's plan</h3>
          <div class="dinner-saved-list">${savedRows}</div>
        </div>`
      : '';

    const showPlanButton = queueDates.size === 0 && !inQueue;

    return `
      <section class="card">
        <h2>Meal Plan — This Week</h2>
        ${queueSection}
        ${savedSection}
        ${!queueSection && !savedSection ? `
          <div class="empty">No meals planned yet.</div>
          <button class="btn" data-action="plan-dinner-week">Plan this week</button>` : ''}
        ${showPlanButton && savedSection ? `
          <div class="input-row dinner-week-actions">
            <button class="btn secondary" data-action="plan-dinner-week">Edit whole week</button>
          </div>` : ''}
      </section>
    `;
  },

  notes(state) {
    const raw = state.notesFull
      ?? (Array.isArray(state.notes) ? state.notes : (state.notes?.recent || []));
    const items = [...raw].sort((a, b) => {
      const pinDiff = (b.pinned ? 1 : 0) - (a.pinned ? 1 : 0);
      if (pinDiff !== 0) return pinDiff;
      return String(b.updatedAt || '').localeCompare(String(a.updatedAt || ''));
    });
    return `
      <section class="card">
        <h2>Family Notes</h2>
        <div class="note-add-form">
          <textarea id="noteInput" placeholder="Add a note…" maxlength="1000" rows="3"></textarea>
          <div class="note-add-meta">
            <label class="note-pin-label"><input type="checkbox" id="notePinned" /> Pin to top</label>
            <button class="btn" data-action="add-note">Add Note</button>
          </div>
        </div>
        ${items.length
          ? `<ul class="list note-list">${items.map((n) => {
              const editing = state.editingNoteId === n.id;
              if (editing) {
                return `
                  <li data-id="${n.id}" class="note-edit-row">
                    <textarea data-field="text" maxlength="1000" placeholder="Note text…">${esc(n.text)}</textarea>
                    <div class="input-row note-edit-meta">
                      <label><input type="checkbox" data-field="pinned" ${n.pinned ? 'checked' : ''} /> Pin to top</label>
                    </div>
                    <div class="row-actions">
                      <button class="btn" data-action="save-note" data-id="${n.id}">Save</button>
                      <button class="btn secondary" data-action="cancel-edit-note">Cancel</button>
                    </div>
                  </li>`;
              }
              return `
                <li data-id="${n.id}" class="list-item-with-reminder">
                  <div class="list-item-main">
                    <span class="note-text">${n.pinned ? '📌 ' : ''}${esc(n.text)}</span>
                    <div class="row-actions">
                      <button class="btn secondary" data-action="edit-note" data-id="${n.id}">Edit</button>
                      <button class="btn danger" data-action="delete-note" data-id="${n.id}">Delete</button>
                    </div>
                  </div>
                  ${reminderBlockHtml(state, 'note', n.id)}
                </li>`;
            }).join('')}</ul>`
          : `<div class="empty">No notes yet.</div>`}
      </section>
    `;
  },

  setup(state, health) {
    const savedToken = (window.API && API.writeToken) || '';
    const auth = state.authMe;
    const clerkOn = Boolean(state.clerkEnabled);
    const isParent = !clerkOn || (auth && auth.role === 'parent');
    const members = state.membersFull || state.members || [];
    const lastSync = state.generatedAt
      ? esc(new Date(state.generatedAt).toLocaleString())
      : '—';

    const accountCard = clerkOn
      ? `
      <section class="card setup-card">
        <h3>Your account</h3>
        ${auth
          ? `<div class="setup-account">
               <p class="setup-account-line"><span class="setup-k">Signed in</span> <code class="setup-code">${esc(auth.userId)}</code></p>
               <p class="setup-account-line"><span class="setup-k">Role</span> <span class="chip">${esc(auth.role)}</span>
                 ${auth.member
                   ? `<span class="muted">· linked to <strong>${esc(auth.member.name)}</strong></span>`
                   : `<span class="muted">· not linked to a family member yet</span>`}
               </p>
             </div>`
          : `<p class="muted">Sign in from the header to manage this household.</p>`}
        ${isParent && auth
          ? `<p class="muted setup-hint">Link this login to a family member so chores and dinner show the right name.</p>
             <ul class="list setup-link-list">${members
               .map(
                 (m) => `
               <li data-id="${m.id}" class="setup-link-row">
                 <span class="chip member-chip" style="border-left:4px solid ${esc(m.color)}">${memberAvatarHtml(m)} ${esc(m.name)}</span>
                 <span class="muted setup-link-meta">${m.clerkUserId ? 'Linked' : 'Not linked'}</span>
                 <div class="row-actions">
                   ${m.clerkUserId === auth.userId
                     ? `<button class="btn secondary" data-action="unlink-clerk-member" data-id="${m.id}">Unlink</button>`
                     : `<button class="btn secondary" data-action="link-clerk-member" data-id="${m.id}">Link me</button>`}
                 </div>
               </li>`
               )
               .join('') || '<li class="muted">Add a family member below first.</li>'}</ul>`
          : ''}
      </section>`
      : `
      <section class="card setup-card">
        <h3>Sign-in</h3>
        <p class="muted">Cloud sign-in is off on this server. LAN access uses the write token below when configured.</p>
      </section>`;

    const advancedToken = `
      <section class="card setup-card setup-advanced">
        <details class="setup-details"${clerkOn ? '' : ' open'}>
          <summary>Advanced · LAN write token</summary>
          <p class="muted setup-hint">Only needed for local tools when cloud sign-in is off, or when the server requires a write token. Leave blank for normal use.</p>
          <div class="input-row">
            <input id="writeTokenInput" type="password" placeholder="Write token" value="${esc(savedToken)}" autocomplete="off" />
            <button class="btn secondary" data-action="save-write-token">Save</button>
          </div>
        </details>
      </section>`;

    return `
      <section class="card setup-card setup-status">
        <h2>Setup</h2>
        <p class="muted setup-lead">Household people, phones, and your account link.</p>
        <div class="setup-status-grid" role="list">
          <div class="setup-stat" role="listitem">
            <span class="setup-k">Server</span>
            <span class="setup-v ${health?.ok ? 'is-ok' : 'is-bad'}">${health?.ok ? 'Online' : 'Offline'}</span>
          </div>
          <div class="setup-stat" role="listitem">
            <span class="setup-k">Version</span>
            <span class="setup-v">${esc(health?.version || '—')}</span>
          </div>
          <div class="setup-stat" role="listitem">
            <span class="setup-k">Last sync</span>
            <span class="setup-v">${lastSync}</span>
          </div>
        </div>
      </section>

      <section class="card setup-card">
        <h3>Family members</h3>
        <p class="muted setup-hint">Names used for chores, dinner cook, and reminders.</p>
        <div class="input-row member-add-row">
          <input id="memberNameInput" type="text" placeholder="Name" maxlength="50" />
          <input id="memberEmojiInput" type="text" placeholder="Emoji" maxlength="8" value="👤" class="emoji-input" aria-label="Avatar emoji" />
          <select id="memberColorInput" aria-label="Color">${colorOptions('#4A90D9')}</select>
          <button class="btn" data-action="add-member">Add</button>
        </div>
        ${emojiPickerHtml('memberEmojiInput', '👤')}
        <ul class="list member-list">
          ${members.map((m) => {
            const editing = state.editingMemberId === m.id;
            if (editing) {
              return `
                <li data-id="${m.id}" class="member-edit-row">
                  <div class="input-row member-edit-fields">
                    <input type="text" data-field="name" value="${esc(m.name)}" maxlength="50" />
                    <input type="text" id="memberEditEmoji-${m.id}" data-field="emoji" value="${esc(m.avatarEmoji)}" maxlength="8" class="emoji-input" aria-label="Avatar emoji" />
                    <select data-field="color" aria-label="Color">${colorOptions(m.color)}</select>
                  </div>
                  ${emojiPickerHtml(`memberEditEmoji-${m.id}`, m.avatarEmoji)}
                  <div class="row-actions">
                    <button class="btn" data-action="save-member" data-id="${m.id}">Save</button>
                    <button class="btn secondary" data-action="cancel-edit-member">Cancel</button>
                  </div>
                </li>`;
            }
            return `
              <li data-id="${m.id}" class="member-row">
                <span class="chip member-chip" style="border-left:4px solid ${esc(m.color)}">${memberAvatarHtml(m)} ${esc(m.name)}</span>
                <div class="row-actions">
                  <button class="btn secondary" data-action="edit-member" data-id="${m.id}">Edit</button>
                  <button class="btn danger" data-action="delete-member" data-id="${m.id}">Delete</button>
                </div>
              </li>`;
          }).join('') || '<li class="muted">No members yet — add one above.</li>'}
        </ul>
      </section>

      <section class="card setup-card">
        <h3>Notification numbers</h3>
        <p class="muted setup-hint">Phone numbers used when reminders fire.</p>
        <ul class="list notification-list">
          ${members.map((m) => `
            <li data-id="${m.id}" class="notification-row">
              <span class="chip member-chip" style="border-left:4px solid ${esc(m.color)}">${memberAvatarHtml(m)} ${esc(m.name)}</span>
              <input type="tel" class="member-phone" placeholder="+1 555 123 4567" value="${esc(m.phone || '')}" maxlength="30" aria-label="Phone for ${esc(m.name)}" />
              <label class="notify-toggle">
                <input type="checkbox" class="member-notify" ${m.notifyEnabled !== false ? 'checked' : ''} />
                <span>Notify</span>
              </label>
              <button class="btn secondary" data-action="save-member-notify" data-id="${m.id}">Save</button>
            </li>`).join('') || '<li class="muted">No members yet — add one above.</li>'}
        </ul>
      </section>

      ${accountCard}
      ${advancedToken}
    `;
  },
};

window.Views = Views;
