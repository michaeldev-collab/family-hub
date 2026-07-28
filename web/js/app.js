'use strict';

const appEl = document.getElementById('app');
const connDot = document.getElementById('connDot');
const connText = document.getElementById('connText');

let state = {};
let health = null;
let currentView = 'home';
let refreshTimer = null;
let editingMemberId = null;
let editingNoteId = null;
let editingReminderKey = null;
let dinnerWeekInQueue = false;
let dinnerQueueDates = new Set();
let noteDraft = { text: '', pinned: false };
let noteEditDraft = null;

function weekDatesFromToday(today) {
  const dates = [];
  const start = new Date(`${today}T12:00:00`);
  for (let i = 0; i < 7; i++) {
    const d = new Date(start);
    d.setDate(start.getDate() + i);
    dates.push(d.toISOString().slice(0, 10));
  }
  return dates;
}

function initDinnerQueueIfNeeded() {
  if (currentView !== 'dinner') return;
  const plans = state.weekDinnerFull || state.weekDinner || [];
  const dates = weekDatesFromToday(state.today);
  const hasMeals = dates.some((date) => {
    const p = (plans || []).find((x) => x.date === date);
    if (!p) return false;
    return Boolean(
      (p.main && p.main.trim()) ||
        (p.side && p.side.trim()) ||
        (p.side2 && p.side2.trim()) ||
        (p.meal && p.meal.trim())
    );
  });
  if (!hasMeals && dinnerQueueDates.size === 0 && !dinnerWeekInQueue) {
    dinnerWeekInQueue = true;
    dinnerQueueDates = new Set(dates);
  }
}

function showToast(msg, kind = '') {
  API.showFlash(msg, kind || 'error');
}

function hideToast() {
  API.hideFlash();
}

function setConnection(online, stale = false) {
  connDot.className = `dot ${online ? (stale ? 'dot-stale' : 'dot-online') : 'dot-offline'}`;
  if (!online) connText.textContent = 'Server offline';
  else if (stale) connText.textContent = 'Data may be stale';
  else connText.textContent = API.lastSync ? `Synced ${API.lastSync.toLocaleTimeString()}` : 'Online';
}

let loginMode = 'sign-in';
let loginMounted = false;
let authListenerBound = false;
let hubBooted = false;

function requiresLogin() {
  const cfg = FamilyHubAuth?.getConfig?.() || {};
  return Boolean(cfg.clerkEnabled) && !FamilyHubAuth.isSignedIn();
}

async function showLoginGate() {
  const gate = document.getElementById('loginGate');
  const shell = document.getElementById('hubShell');
  if (gate) gate.hidden = false;
  if (shell) shell.hidden = true;
  document.body.classList.add('login-only');
  await ensureLoginMount();
}

async function ensureLoginMount() {
  if (!window.FamilyHubAuth) return;
  if (FamilyHubAuth.isSignedIn()) return;
  const lead = document.getElementById('loginGateLead');
  const signInMount = document.getElementById('clerkSignInMount');
  const signUpMount = document.getElementById('clerkSignUpMount');
  const signUpRow = document.getElementById('loginGateSignUpRow');
  const backRow = document.getElementById('loginGateBackRow');
  if (!signInMount || !signUpMount) return;

  if (lead) {
    lead.textContent = loginMode === 'sign-up'
      ? 'Create an account to join this household hub.'
      : 'Sign in to continue to your household hub.';
  }

  // Only mount the active Clerk form — mounting both confuses the UI.
  if (loginMode === 'sign-up') {
    if (signInMount && !signInMount.hidden) await FamilyHubAuth.unmount(signInMount);
    signInMount.hidden = true;
    signUpMount.hidden = false;
    if (signUpRow) signUpRow.hidden = true;
    if (backRow) backRow.hidden = false;
    await FamilyHubAuth.mountSignUp(signUpMount);
  } else {
    if (signUpMount && !signUpMount.hidden) await FamilyHubAuth.unmount(signUpMount);
    signUpMount.hidden = true;
    signInMount.hidden = false;
    if (signUpRow) signUpRow.hidden = false;
    if (backRow) backRow.hidden = true;
    await FamilyHubAuth.mountSignIn(signInMount);
  }
  loginMounted = true;
}

function setLoginMode(mode) {
  loginMode = mode === 'sign-up' ? 'sign-up' : 'sign-in';
  ensureLoginMount().catch(() => {});
}

async function hideLoginGate() {
  const gate = document.getElementById('loginGate');
  const shell = document.getElementById('hubShell');
  if (gate) gate.hidden = true;
  if (shell) shell.hidden = false;
  document.body.classList.remove('login-only');
  if (loginMounted && window.FamilyHubAuth) {
    const signInMount = document.getElementById('clerkSignInMount');
    const signUpMount = document.getElementById('clerkSignUpMount');
    if (signInMount) await FamilyHubAuth.unmount(signInMount);
    if (signUpMount) await FamilyHubAuth.unmount(signUpMount);
    loginMounted = false;
  }
}

async function applyAuthGate() {
  if (!window.FamilyHubAuth) return false;
  // Wait out Clerk handshake / cookie sync so we don't mount SignIn while
  // already authenticated (that + force redirect was the reload loop).
  if (typeof FamilyHubAuth.settleAuth === 'function') {
    await FamilyHubAuth.settleAuth();
  } else {
    await FamilyHubAuth.init();
  }
  bindAuthListener();
  if (requiresLogin()) {
    await showLoginGate();
    updateAuthBar();
    return false;
  }
  await hideLoginGate();
  updateAuthBar();
  return true;
}

async function enterHubAfterAuth() {
  const allowed = await applyAuthGate();
  if (!allowed) return;
  if (FamilyHubAuth.isSignedIn()) {
    try {
      state.authMe = await FamilyHubAuth.refreshMe();
    } catch (_) {
      state.authMe = null;
    }
  }
  updateAuthBar();
  await refresh({ forceRender: true });
  if (!hubBooted) {
    hubBooted = true;
    if (!refreshTimer) refreshTimer = setInterval(() => refresh(), 30000);
  }
}

function bindAuthListener() {
  if (authListenerBound || !window.FamilyHubAuth) return;
  const clerk = FamilyHubAuth.getClerk?.();
  if (!clerk) return;
  authListenerBound = true;
  let lastSignedIn = FamilyHubAuth.isSignedIn();
  let entering = false;
  FamilyHubAuth.addListener(async () => {
    try {
      const signedIn = FamilyHubAuth.isSignedIn();
      if (signedIn === lastSignedIn && hubBooted) return;
      lastSignedIn = signedIn;
      if (signedIn) {
        // Stay on this page — hard reload to "/" remounts SignIn and loops.
        if (entering) return;
        entering = true;
        try {
          await enterHubAfterAuth();
        } finally {
          entering = false;
        }
      } else {
        hubBooted = false;
        if (refreshTimer) {
          clearInterval(refreshTimer);
          refreshTimer = null;
        }
        await applyAuthGate();
      }
    } catch (_) {
      /* ignore listener errors */
    }
  });
}

function updateAuthBar() {
  const bar = document.getElementById('authBar');
  const label = document.getElementById('authLabel');
  const signOut = document.getElementById('authSignOut');
  const userBtn = document.getElementById('authUserButton');
  if (!bar || !window.FamilyHubAuth) return;
  const cfg = FamilyHubAuth.getConfig();
  if (!cfg.clerkEnabled || requiresLogin()) {
    bar.hidden = true;
    return;
  }
  bar.hidden = false;
  const me = FamilyHubAuth.getMe();
  const signedIn = FamilyHubAuth.isSignedIn();
  if (signedIn) {
    label.textContent = me
      ? `${me.role}${me.member ? ` · ${me.member.name}` : ''}`
      : 'Signed in';
    signOut.hidden = false;
    if (userBtn) {
      userBtn.hidden = false;
      FamilyHubAuth.mountUserButton(userBtn);
    }
  } else {
    label.textContent = '';
    signOut.hidden = true;
    if (userBtn) {
      userBtn.hidden = true;
      userBtn.innerHTML = '';
    }
  }
}

async function syncAuthState() {
  if (!window.FamilyHubAuth) return;
  await FamilyHubAuth.init();
  const cfg = FamilyHubAuth.getConfig();
  state.clerkEnabled = Boolean(cfg.clerkEnabled);
  if (cfg.clerkEnabled && FamilyHubAuth.isSignedIn()) {
    try {
      state.authMe = await FamilyHubAuth.refreshMe();
    } catch (_) {
      state.authMe = null;
    }
  } else {
    state.authMe = null;
  }
  await applyAuthGate();
}

function addDaysIso(iso, days) {
  const d = new Date(`${iso}T12:00:00`);
  d.setDate(d.getDate() + days);
  return d.toISOString().slice(0, 10);
}

async function loadState() {
  try {
    await syncAuthState();
    health = await API.health();
    state = await API.dashboardState();
    state.clerkEnabled = Boolean(FamilyHubAuth?.getConfig()?.clerkEnabled);
    state.authMe = FamilyHubAuth?.getMe() || null;
    const dinnerEnd = addDaysIso(state.today, 27);
    const [groceryFull, choresFull, notesFull, membersFull, weekDinner, remindersFull] = await Promise.all([
      API.grocery.list(),
      API.chores.list(),
      API.notes.list(),
      API.members.list(),
      API.dinner.week(state.today, dinnerEnd),
      API.reminders.list({ status: 'pending' }),
    ]);
    state.groceryFull = groceryFull.items;
    state.groceryOtherTitle = groceryFull.otherTitle || 'Other';
    state.choresFull = choresFull.items;
    state.notesFull = notesFull.items;
    state.membersFull = membersFull.items;
    state.weekDinnerFull = weekDinner.items;
    state.reminders = remindersFull.items;
    state.editingMemberId = editingMemberId;
    state.editingNoteId = editingNoteId;
    state.editingReminderKey = editingReminderKey;
    state.dinnerWeekInQueue = dinnerWeekInQueue;
    state.dinnerQueueDates = [...dinnerQueueDates];
    initDinnerQueueIfNeeded();
    state.dinnerWeekInQueue = dinnerWeekInQueue;
    state.dinnerQueueDates = [...dinnerQueueDates];
    setConnection(true, false);
    updateAuthBar();
  } catch (err) {
    setConnection(false);
    if (!state.generatedAt) {
      appEl.innerHTML = `<section class="card"><h2>Cannot reach server</h2><p class="muted">Check that Family Hub is running on endeavor.</p></section>`;
    }
    throw err;
  }
}

function syncUiStateToRender() {
  state.editingMemberId = editingMemberId;
  state.editingNoteId = editingNoteId;
  state.editingReminderKey = editingReminderKey;
  state.dinnerWeekInQueue = dinnerWeekInQueue;
  state.dinnerQueueDates = [...dinnerQueueDates];
}

function captureNoteDraft() {
  if (currentView !== 'notes') return;
  const input = document.getElementById('noteInput');
  const pinned = document.getElementById('notePinned');
  if (input) {
    noteDraft = {
      text: input.value,
      pinned: pinned ? pinned.checked : false,
    };
  }
  if (editingNoteId) {
    const row = document.querySelector(`.note-edit-row[data-id="${editingNoteId}"]`);
    if (row) {
      const textEl = row.querySelector('[data-field="text"]');
      const pinnedEl = row.querySelector('[data-field="pinned"]');
      noteEditDraft = {
        id: editingNoteId,
        text: textEl ? textEl.value : '',
        pinned: pinnedEl ? pinnedEl.checked : false,
      };
    }
  } else {
    noteEditDraft = null;
  }
}

function restoreNoteDraft() {
  if (currentView !== 'notes') return;
  const input = document.getElementById('noteInput');
  const pinned = document.getElementById('notePinned');
  if (input) {
    input.value = noteDraft.text;
    if (pinned) pinned.checked = noteDraft.pinned;
  }
  if (noteEditDraft && editingNoteId === noteEditDraft.id) {
    const row = document.querySelector(`.note-edit-row[data-id="${noteEditDraft.id}"]`);
    if (row) {
      const textEl = row.querySelector('[data-field="text"]');
      const pinnedEl = row.querySelector('[data-field="pinned"]');
      if (textEl) textEl.value = noteEditDraft.text;
      if (pinnedEl) pinnedEl.checked = noteEditDraft.pinned;
    }
  }
}

function isNotesEditingActive() {
  if (currentView !== 'notes') return false;
  const active = document.activeElement;
  if (active && (active.id === 'noteInput' || active.closest('.note-edit-row'))) return true;
  if (editingNoteId) return true;
  if (editingReminderKey && editingReminderKey.startsWith('note:')) return true;
  if (noteDraft.text.trim()) return true;
  return false;
}

function isReminderEditingActive() {
  return Boolean(editingReminderKey);
}

function render() {
  captureNoteDraft();
  syncUiStateToRender();
  const html = Views[currentView](state, health);
  appEl.innerHTML = html;
  restoreNoteDraft();
  wireEmojiPickers(appEl);
}

async function refresh({ forceRender = false } = {}) {
  try {
    if (window.FamilyHubAuth) {
      await FamilyHubAuth.init();
      if (requiresLogin()) {
        await applyAuthGate();
        return;
      }
    }
    await loadState();
    if (!forceRender && (isNotesEditingActive() || isReminderEditingActive())) return;
    render();
  } catch (_) {
    showToast('Could not refresh the latest data. Showing the last loaded view.', 'error');
  }
}

async function withConfirm(action, successMsg) {
  try {
    hideToast();
    await action();
    await refresh({ forceRender: true });
    if (successMsg) showToast(successMsg, 'ok');
  } catch (err) {
    showToast(API.formatApiError(err), 'error');
    if (err && err.status === 401) {
      // Point operators at Setup without leaving the SPA.
      const setupTab = document.getElementById('tab-setup');
      if (setupTab) setupTab.classList.add('tab-attention');
    }
  }
}

function readDinnerRow(row) {
  const originalDate = row.dataset.date;
  const dateInput = row.querySelector('.dinner-date');
  const date = dateInput ? dateInput.value : originalDate;
  const main = row.querySelector('.dinner-main')?.value.trim() || '';
  const side = row.querySelector('.dinner-side')?.value.trim() || '';
  const side2 = row.querySelector('.dinner-side2')?.value.trim() || '';
  const cookId = row.querySelector('.dinner-cook').value || null;
  const notes = row.querySelector('.dinner-notes').value.trim();
  return { date, originalDate, main, side, side2, cookId, notes };
}

async function saveDinnerRow(row) {
  const { date, originalDate, main, side, side2, cookId, notes } = readDinnerRow(row);
  if (!date) throw new Error('Pick a date');
  await API.dinner.set(date, { main, side, side2, cookId, notes });
  if (originalDate && originalDate !== date) {
    await API.dinner.set(originalDate, {
      main: '',
      side: '',
      side2: '',
      cookId: null,
      notes: '',
    });
  }
  dinnerQueueDates.delete(originalDate);
}

function syncEmojiPickerSelection(block, emoji) {
  block.querySelectorAll('.emoji-pick').forEach((btn) => {
    const selected = btn.dataset.emoji === emoji;
    btn.classList.toggle('selected', selected);
    btn.setAttribute('aria-selected', selected ? 'true' : 'false');
  });
}

function wireEmojiPickers(root = document) {
  root.querySelectorAll('.emoji-picker-block').forEach((block) => {
    const targetId = block.dataset.emojiTarget;
    const input = targetId ? document.getElementById(targetId) : null;
    if (!input) return;
    syncEmojiPickerSelection(block, input.value.trim() || '👤');
  });
}

function setEmojiTargetValue(targetId, emoji) {
  const input = document.getElementById(targetId);
  if (!input) return;
  input.value = emoji;
  input.dispatchEvent(new Event('input', { bubbles: true }));
  const block = document.querySelector(`.emoji-picker-block[data-emoji-target="${targetId}"]`);
  if (block) syncEmojiPickerSelection(block, emoji);
}

appEl.addEventListener('click', async (e) => {
  const emojiBtn = e.target.closest('.emoji-pick');
  if (emojiBtn) {
    const block = emojiBtn.closest('.emoji-picker-block');
    const targetId = block?.dataset.emojiTarget;
    const emoji = emojiBtn.dataset.emoji;
    if (targetId && emoji) setEmojiTargetValue(targetId, emoji);
    return;
  }

  const btn = e.target.closest('[data-action]');
  if (!btn) return;
  const action = btn.dataset.action;
  const id = btn.dataset.id;

  if (action === 'add-grocery') {
    const listKey = btn.dataset.list || 'main';
    const input = document.getElementById(`groceryInput-${listKey}`);
    const text = input ? input.value.trim() : '';
    if (!text) return showToast('Enter an item', 'error');
    await withConfirm(() => API.grocery.create({ text, listKey }), 'Added');
    if (input) input.value = '';
  }

  if (action === 'toggle-grocery') {
    const item = state.groceryFull.find((g) => g.id === id);
    if (!item) return;
    await withConfirm(() => API.grocery.update(id, { checked: !item.checked }));
  }

  if (action === 'toggle-grocery-needed') {
    const item = state.groceryFull.find((g) => g.id === id);
    if (!item) return;
    await withConfirm(() => API.grocery.update(id, { needed: !item.needed }));
  }

  if (action === 'save-grocery-other-title') {
    const input = document.getElementById('groceryOtherTitle');
    const otherTitle = input ? input.value.trim() : '';
    if (!otherTitle) return showToast('Enter a list name', 'error');
    await withConfirm(async () => {
      const res = await API.grocery.setOtherTitle(otherTitle);
      state.groceryOtherTitle = res.otherTitle || otherTitle;
    }, 'List name saved');
  }

  if (action === 'delete-grocery') {
    await withConfirm(() => API.grocery.remove(id), 'Deleted');
  }

  if (action === 'add-chore') {
    const input = document.getElementById('choreInput');
    const assigneeId = document.getElementById('choreAssignee').value || null;
    const title = input.value.trim();
    if (!title) return showToast('Enter a chore', 'error');
    await withConfirm(() => API.chores.create({ title, assigneeId }), 'Chore added');
    input.value = '';
  }

  if (action === 'toggle-chore') {
    const item = state.choresFull.find((c) => c.id === id);
    if (!item) return;
    await withConfirm(() => API.chores.update(id, { completed: !item.completed }));
  }

  if (action === 'delete-chore') {
    await withConfirm(() => API.chores.remove(id), 'Deleted');
  }

  if (action === 'save-dinner-day') {
    const row = btn.closest('.dinner-queue-row');
    if (!row) return;
    await withConfirm(async () => {
      await saveDinnerRow(row);
      if (dinnerQueueDates.size === 0) dinnerWeekInQueue = false;
    }, 'Day saved');
  }

  if (action === 'save-dinner-week') {
    const rows = [...document.querySelectorAll('.dinner-queue-row')];
    await withConfirm(async () => {
      for (const row of rows) {
        await saveDinnerRow(row);
      }
      dinnerQueueDates.clear();
      dinnerWeekInQueue = false;
    }, 'Week saved — moved to your plan');
  }

  if (action === 'edit-dinner-day') {
    const date = btn.dataset.date;
    dinnerQueueDates.add(date);
    syncUiStateToRender();
    render();
    return;
  }

  if (action === 'plan-dinner-week') {
    const dates = weekDatesFromToday(state.today);
    dinnerWeekInQueue = true;
    dates.forEach((d) => dinnerQueueDates.add(d));
    syncUiStateToRender();
    render();
    return;
  }

  if (action === 'add-note') {
    const text = document.getElementById('noteInput').value.trim();
    const pinned = document.getElementById('notePinned').checked;
    if (!text) return showToast('Enter a note', 'error');
    noteDraft = { text: '', pinned: false };
    await withConfirm(() => API.notes.create({ text, pinned }), 'Note added');
  }

  if (action === 'delete-note') {
    await withConfirm(async () => {
      await API.notes.remove(id);
      if (editingNoteId === id) {
        editingNoteId = null;
        noteEditDraft = null;
      }
    }, 'Deleted');
  }

  if (action === 'edit-note') {
    editingNoteId = id;
    state.editingNoteId = id;
    render();
  }

  if (action === 'cancel-edit-note') {
    editingNoteId = null;
    noteEditDraft = null;
    state.editingNoteId = null;
    render();
  }

  if (action === 'save-note') {
    const row = btn.closest('.note-edit-row');
    if (!row) return;
    const text = row.querySelector('[data-field="text"]').value.trim();
    const pinned = row.querySelector('[data-field="pinned"]').checked;
    if (!text) return showToast('Note text is required', 'error');
    await withConfirm(async () => {
      await API.notes.update(id, { text, pinned });
      editingNoteId = null;
      noteEditDraft = null;
    }, 'Note updated');
  }

  if (action === 'save-write-token') {
    const input = document.getElementById('writeTokenInput');
    API.setWriteToken(input ? input.value.trim() : '');
    document.getElementById('tab-setup')?.classList.remove('tab-attention');
    showToast('Write token saved', 'ok');
    return;
  }

  if (action === 'link-clerk-member') {
    await withConfirm(() => API.auth.linkMember(id), 'Linked to member');
    await syncAuthState();
    return;
  }

  if (action === 'unlink-clerk-member') {
    await withConfirm(() => API.auth.unlinkMember(id), 'Unlinked');
    await syncAuthState();
    return;
  }

  if (action === 'add-member') {
    const name = document.getElementById('memberNameInput').value.trim();
    const avatarEmoji = document.getElementById('memberEmojiInput').value.trim() || '👤';
    const color = document.getElementById('memberColorInput').value;
    if (!name) return showToast('Enter a name', 'error');
    await withConfirm(
      () => API.members.create({ name, avatarEmoji, color }),
      'Member added'
    );
    document.getElementById('memberNameInput').value = '';
    document.getElementById('memberEmojiInput').value = '👤';
  }

  if (action === 'edit-member') {
    editingMemberId = id;
    state.editingMemberId = id;
    render();
  }

  if (action === 'cancel-edit-member') {
    editingMemberId = null;
    state.editingMemberId = null;
    render();
  }

  if (action === 'save-member') {
    const row = btn.closest('.member-edit-row');
    if (!row) return;
    const name = row.querySelector('[data-field="name"]').value.trim();
    const avatarEmoji = row.querySelector('[data-field="emoji"]').value.trim() || '👤';
    const color = row.querySelector('[data-field="color"]').value;
    if (!name) return showToast('Name is required', 'error');
    await withConfirm(async () => {
      await API.members.update(id, { name, avatarEmoji, color });
      editingMemberId = null;
    }, 'Member updated');
  }

  if (action === 'delete-member') {
    const member = (state.membersFull || state.members || []).find((m) => m.id === id);
    const label = member ? member.name : 'this member';
    if (!window.confirm(`Remove ${label}? Chores and dinner assignments will be unassigned.`)) {
      return;
    }
    await withConfirm(async () => {
      await API.members.remove(id);
      if (editingMemberId === id) editingMemberId = null;
    }, 'Member removed');
  }

  if (action === 'edit-reminder') {
    const entityType = btn.dataset.entityType;
    const entityId = btn.dataset.entityId;
    editingReminderKey = `${entityType}:${entityId}`;
    render();
    return;
  }

  if (action === 'cancel-reminder-edit') {
    editingReminderKey = null;
    render();
    return;
  }

  if (action === 'save-reminder') {
    const entityType = btn.dataset.entityType;
    const entityId = btn.dataset.entityId;
    const panel = btn.closest('.reminder-panel');
    if (!panel) return;
    const remindAtRaw = panel.querySelector('[data-field="remind-at"]')?.value;
    if (!remindAtRaw) return showToast('Pick date and time', 'error');
    const message = panel.querySelector('[data-field="message"]')?.value.trim() || '';
    const notifyMemberIds = [...panel.querySelectorAll('[data-field="member"]:checked')].map(
      (el) => el.value
    );
    await withConfirm(async () => {
      const existing = (state.reminders || []).find(
        (r) =>
          r.entityType === entityType && r.entityId === entityId && r.status === 'pending'
      );
      if (existing) await API.reminders.cancel(existing.id);
      await API.reminders.create({
        entityType,
        entityId,
        remindAt: new Date(remindAtRaw).toISOString(),
        message,
        notifyMemberIds,
      });
      editingReminderKey = null;
    }, 'Reminder saved');
  }

  if (action === 'remove-reminder') {
    const reminderId = btn.dataset.reminderId;
    if (!reminderId) return;
    await withConfirm(async () => {
      await API.reminders.cancel(reminderId);
      if (editingReminderKey) editingReminderKey = null;
    }, 'Reminder removed');
  }

  if (action === 'save-member-notify') {
    const row = btn.closest('.notification-row');
    if (!row) return;
    const phone = row.querySelector('.member-phone')?.value.trim() || '';
    const notifyEnabled = row.querySelector('.member-notify')?.checked ?? true;
    await withConfirm(
      () => API.members.update(id, { phone, notifyEnabled }),
      'Notification settings saved'
    );
  }
});

document.querySelectorAll('.tab').forEach((tab) => {
  tab.addEventListener('click', async () => {
    document.querySelectorAll('.tab').forEach((t) => {
      t.classList.remove('active');
      t.setAttribute('aria-selected', 'false');
      t.tabIndex = -1;
    });
    tab.classList.add('active');
    tab.setAttribute('aria-selected', 'true');
    tab.tabIndex = 0;
    currentView = tab.dataset.view;
    appEl.setAttribute('aria-labelledby', tab.id);
    if (currentView === 'setup') {
      tab.classList.remove('tab-attention');
    }
    if (currentView !== 'notes') {
      noteDraft = { text: '', pinned: false };
      noteEditDraft = null;
    }
    if (currentView !== 'grocery' && currentView !== 'chores' && currentView !== 'dinner' && currentView !== 'notes') {
      editingReminderKey = null;
    }
    await refresh({ forceRender: true });
  });
});

document.querySelector('.tabs').addEventListener('keydown', (event) => {
  if (!['ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key)) return;
  const tabs = [...document.querySelectorAll('.tab')];
  const current = tabs.indexOf(document.activeElement);
  if (current < 0) return;
  event.preventDefault();
  const next = event.key === 'Home' ? 0 : event.key === 'End' ? tabs.length - 1 :
    (current + (event.key === 'ArrowRight' ? 1 : -1) + tabs.length) % tabs.length;
  tabs[next].focus();
  tabs[next].click();
});

appEl.addEventListener('input', (e) => {
  if (e.target.classList.contains('emoji-input')) {
    const block = document.querySelector(`.emoji-picker-block[data-emoji-target="${e.target.id}"]`);
    if (block) syncEmojiPickerSelection(block, e.target.value.trim() || '👤');
  }
  if (e.target.id === 'noteInput' || e.target.closest('.note-edit-row')) {
    captureNoteDraft();
  }
});

appEl.addEventListener('change', (e) => {
  if (e.target.id === 'notePinned' || e.target.matches('.note-edit-row [data-field="pinned"]')) {
    captureNoteDraft();
  }
});

document.getElementById('loginGateSignUp')?.addEventListener('click', () => setLoginMode('sign-up'));
document.getElementById('loginGateSignIn')?.addEventListener('click', () => setLoginMode('sign-in'));

document.getElementById('authSignOut')?.addEventListener('click', async () => {
  try {
    await FamilyHubAuth.signOut();
    loginMode = 'sign-in';
    hubBooted = false;
    if (refreshTimer) {
      clearInterval(refreshTimer);
      refreshTimer = null;
    }
    await syncAuthState();
  } catch (err) {
    showToast(err.message || 'Sign out failed', 'error');
  }
});

(async function boot() {
  try {
    const allowed = await applyAuthGate();
    if (!allowed) return;
    hubBooted = true;
    await refresh({ forceRender: true });
    refreshTimer = setInterval(() => refresh(), 30000);
  } catch (_) {
    await refresh({ forceRender: true });
    refreshTimer = setInterval(() => refresh(), 30000);
  }
})();

window.addEventListener('focus', () => {
  syncAuthState().then(() => updateAuthBar()).catch(() => {});
});
