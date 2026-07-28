'use strict';

const config = require('../../config/env');
const members = require('../members');
const grocery = require('../grocery');
const chores = require('../chores');
const dinner = require('../dinner');
const notes = require('../notes');
const { getStateVersion } = require('../stateVersion');
const { todayIso } = require('../../utils/helpers');

/** Panel contract schema — bump only with firmware support. */
const PANEL_SCHEMA_VERSION = 1;

function buildHome({ dinnerWithCook, cookName, openChores, openGrocery, pinnedNotes }) {
  return {
    dinner_today: dinnerWithCook ? dinnerWithCook.meal || null : null,
    dinner_cook: cookName,
    open_chores_count: openChores.length,
    grocery_count: openGrocery.length,
    pinned: pinnedNotes.map((n) => ({ text: n.text })),
    badge: 'ONLINE',
  };
}

function buildGroceryVm() {
  const otherTitle = grocery.getOtherTitle();
  const constant = grocery.listGrocery({ listKey: 'constant' }).map((g) => ({
    id: g.id,
    text: g.text,
    needed: !!g.needed,
  }));
  const main = grocery.listGrocery({ listKey: 'main' }).map((g) => ({
    id: g.id,
    text: g.text,
    checked: !!g.checked,
  }));
  const other = grocery.listGrocery({ listKey: 'other' }).map((g) => ({
    id: g.id,
    text: g.text,
    checked: !!g.checked,
  }));
  const toBuy = grocery.shoppingItems({ limit: 24 }).map((g) => ({
    id: g.id,
    text: g.text,
    checked: g.listKey === 'constant' ? false : !!g.checked,
    needed: g.listKey === 'constant' ? !!g.needed : false,
    list_key: g.listKey,
  }));
  return {
    other_title: otherTitle,
    constant,
    main,
    other,
    // Legacy flat "to buy" list for Home / older firmware.
    items: toBuy.map((g) => ({
      id: g.id,
      text: g.text,
      checked: !!g.checked,
    })),
  };
}

function buildChoresVm(openChores) {
  return {
    items: openChores.map((c) => ({
      id: c.id,
      title: c.title,
      assignee_name: c.assignee && c.assignee.name ? c.assignee.name : null,
    })),
  };
}

function buildDinnerVm({ dinnerWithCook, cookName, today, weekDinner }) {
  return {
    today: dinnerWithCook
      ? {
          date: dinnerWithCook.date || today,
          main: dinnerWithCook.main || '',
          side: dinnerWithCook.side || '',
          side2: dinnerWithCook.side2 || '',
          meal: dinnerWithCook.meal,
          cook_name: cookName,
          notes: dinnerWithCook.notes || '',
        }
      : null,
    week: weekDinner.map((d) => ({
      date: d.date,
      main: d.main || '',
      // Keep meal as the main for older panel firmware filters.
      meal: d.main || d.meal || '',
    })),
  };
}

function buildNotesVm({ pinnedNotes, recentNotes }) {
  return {
    pinned: pinnedNotes.map((n) => ({ text: n.text })),
    recent: recentNotes.map((n) => ({ text: n.text })),
  };
}

function buildDashboard() {
  const today = todayIso();
  const weekEnd = new Date();
  weekEnd.setDate(weekEnd.getDate() + 6);
  const weekEndIso = weekEnd.toISOString().slice(0, 10);

  const allMembers = members.listMembers();
  const memberMap = Object.fromEntries(allMembers.map((m) => [m.id, m]));

  const todayDinner = dinner.getDinner(today);
  const dinnerWithCook = todayDinner
    ? {
        ...todayDinner,
        cook: todayDinner.cookId ? memberMap[todayDinner.cookId] || null : null,
      }
    : null;

  const openChores = chores
    .listChores({ includeCompleted: false })
    .slice(0, 8)
    .map((c) => ({
      ...c,
      assignee: c.assigneeId ? memberMap[c.assigneeId] || null : null,
    }));

  const openGrocery = grocery.shoppingItems({ limit: 24 });
  const pinnedNotes = notes.listNotes().filter((n) => n.pinned).slice(0, 5);
  const recentNotes = notes.listNotes().slice(0, 8);
  const weekDinner = dinner.listDinnerRange(today, weekEndIso);

  const generatedAt = new Date().toISOString();
  const cookName =
    dinnerWithCook && dinnerWithCook.cook && dinnerWithCook.cook.name
      ? dinnerWithCook.cook.name
      : null;

  const ctx = { dinnerWithCook, cookName, openChores, openGrocery, pinnedNotes };

  return {
    schema_version: PANEL_SCHEMA_VERSION,
    state_version: getStateVersion(),
    generated_at: generatedAt,
    server_version: config.version,
    // Legacy camelCase aliases for browser (transition).
    generatedAt,
    serverVersion: config.version,
    today,
    members: allMembers,
    // Nested panel VMs — firmware binds these (own the dinner/chores/grocery/notes keys).
    home: buildHome(ctx),
    grocery: buildGroceryVm(),
    chores: buildChoresVm(openChores),
    dinner: buildDinnerVm({ dinnerWithCook, cookName, today, weekDinner }),
    notes: buildNotesVm({ pinnedNotes, recentNotes }),
    // Flat list aliases still useful for web until it fully uses *Full endpoints.
    weekDinner,
    pinnedNotes,
    connection: {
      sourceOfTruth: 'server',
    },
    // Public browser URL for panel App tab + QR (falls back to LAN on device).
    public_app_url: config.publicAppUrl || '',
  };
}

module.exports = {
  PANEL_SCHEMA_VERSION,
  buildDashboard,
  buildHome,
  buildGroceryVm,
  buildChoresVm,
  buildDinnerVm,
  buildNotesVm,
};
