'use strict';

const grocery = require('./grocery');
const chores = require('./chores');
const dinner = require('./dinner');
const notes = require('./notes');
const { getCachedResponse, storeCachedResponse, logWrite } = require('./idempotency');
const { todayIso } = require('../utils/helpers');

const ALLOWED_TYPES = new Set([
  'grocery.add',
  'grocery.toggle',
  'chore.complete',
  'chore.uncomplete',
  'dinner.set',
  'note.add',
]);

function handleEvent(payload, source = 'panel') {
  const { eventId, type } = payload;
  if (!eventId || typeof eventId !== 'string' || eventId.length > 64) {
    throw new Error('invalid-event-id');
  }
  if (!ALLOWED_TYPES.has(type)) {
    throw new Error('invalid-event-type');
  }

  const cached = getCachedResponse(eventId);
  if (cached) {
    return { ...cached, deduplicated: true };
  }

  let result;
  switch (type) {
    case 'grocery.add': {
      const item = grocery.createGrocery({
        text: payload.text,
        category: payload.category,
        addedBy: payload.addedBy,
        listKey: payload.listKey || payload.list || 'main',
        needed: payload.needed,
      });
      logWrite('create', 'grocery', item.id, source);
      result = { ok: true, type, item };
      break;
    }
    case 'grocery.toggle': {
      const existing = grocery.getGrocery(payload.id);
      if (!existing) throw new Error('not-found');
      const item =
        existing.listKey === 'constant'
          ? grocery.updateGrocery(payload.id, { needed: !existing.needed })
          : grocery.updateGrocery(payload.id, { checked: !existing.checked });
      logWrite('toggle', 'grocery', item.id, source);
      result = { ok: true, type, item };
      break;
    }
    case 'chore.complete': {
      const item = chores.updateChore(payload.id, { completed: true });
      if (!item) throw new Error('not-found');
      logWrite('complete', 'chore', item.id, source);
      result = { ok: true, type, item };
      break;
    }
    case 'chore.uncomplete': {
      const item = chores.updateChore(payload.id, { completed: false });
      if (!item) throw new Error('not-found');
      logWrite('uncomplete', 'chore', item.id, source);
      result = { ok: true, type, item };
      break;
    }
    case 'dinner.set': {
      const date = payload.date || todayIso();
      const plan = dinner.setDinner(date, {
        cookId: payload.cookId,
        meal: payload.meal,
        main: payload.main,
        side: payload.side,
        side2: payload.side2,
        notes: payload.notes,
      });
      logWrite('set', 'dinner', date, source);
      result = { ok: true, type, plan };
      break;
    }
    case 'note.add': {
      const item = notes.createNote({ text: payload.text, pinned: payload.pinned });
      logWrite('create', 'note', item.id, source);
      result = { ok: true, type, item };
      break;
    }
    default:
      throw new Error('invalid-event-type');
  }

  storeCachedResponse(eventId, result);
  return result;
}

module.exports = { handleEvent, ALLOWED_TYPES };
