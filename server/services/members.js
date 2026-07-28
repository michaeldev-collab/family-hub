'use strict';

const { getDb } = require('../db/connection');
const { bumpStateVersion } = require('./stateVersion');

function mapMemberRow(row) {
  return {
    id: row.id,
    name: row.name,
    color: row.color,
    avatarEmoji: row.avatar_emoji,
    sortOrder: row.sort_order,
    phone: row.phone || '',
    notifyEnabled: row.notify_enabled !== 0,
    clerkUserId: row.clerk_user_id || null,
  };
}

function listMembers() {
  const db = getDb();
  return db
    .prepare(
      `SELECT id, name, color, avatar_emoji, sort_order, phone, notify_enabled, clerk_user_id
       FROM family_members ORDER BY sort_order ASC, name ASC`
    )
    .all()
    .map(mapMemberRow);
}

function getMember(id) {
  const db = getDb();
  const row = db
    .prepare(
      `SELECT id, name, color, avatar_emoji, sort_order, phone, notify_enabled, clerk_user_id
       FROM family_members WHERE id = ?`
    )
    .get(id);
  return row ? mapMemberRow(row) : null;
}

function getMemberByClerkUserId(clerkUserId) {
  if (!clerkUserId) return null;
  const db = getDb();
  const row = db
    .prepare(
      `SELECT id, name, color, avatar_emoji, sort_order, phone, notify_enabled, clerk_user_id
       FROM family_members WHERE clerk_user_id = ?`
    )
    .get(clerkUserId);
  return row ? mapMemberRow(row) : null;
}

function linkClerkUser(memberId, clerkUserId) {
  const existing = getMember(memberId);
  if (!existing) return null;
  if (existing.clerkUserId && existing.clerkUserId !== clerkUserId) {
    throw new Error('member-already-linked');
  }
  const other = getMemberByClerkUserId(clerkUserId);
  if (other && other.id !== memberId) {
    throw new Error('clerk-user-linked-elsewhere');
  }
  const db = getDb();
  db.prepare(
    `UPDATE family_members SET clerk_user_id = ? WHERE id = ?`
  ).run(clerkUserId, memberId);
  bumpStateVersion(db);
  return getMember(memberId);
}

function unlinkClerkUser(memberId) {
  const existing = getMember(memberId);
  if (!existing) return null;
  const db = getDb();
  db.prepare(
    `UPDATE family_members SET clerk_user_id = NULL WHERE id = ?`
  ).run(memberId);
  bumpStateVersion(db);
  return getMember(memberId);
}

function createMember({ name, color, avatarEmoji, sortOrder, phone, notifyEnabled }) {
  const { newId } = require('../utils/helpers');
  const id = newId();
  const db = getDb();
  db.prepare(
    `INSERT INTO family_members (id, name, color, avatar_emoji, sort_order, phone, notify_enabled)
     VALUES (?, ?, ?, ?, ?, ?, ?)`
  ).run(
    id,
    name,
    color || '#4A90D9',
    avatarEmoji || '👤',
    sortOrder ?? 0,
    phone || '',
    notifyEnabled === false ? 0 : 1
  );
  bumpStateVersion(db);
  return getMember(id);
}

function updateMember(id, patch) {
  const existing = getMember(id);
  if (!existing) return null;
  const db = getDb();
  const notifyEnabled =
    patch.notifyEnabled !== undefined
      ? patch.notifyEnabled
        ? 1
        : 0
      : existing.notifyEnabled
        ? 1
        : 0;
  db.prepare(
    `UPDATE family_members
     SET name = ?, color = ?, avatar_emoji = ?, sort_order = ?, phone = ?, notify_enabled = ?
     WHERE id = ?`
  ).run(
    patch.name ?? existing.name,
    patch.color ?? existing.color,
    patch.avatarEmoji ?? existing.avatarEmoji,
    patch.sortOrder ?? existing.sortOrder,
    patch.phone !== undefined ? patch.phone : existing.phone,
    notifyEnabled,
    id
  );
  bumpStateVersion(db);
  return getMember(id);
}

// Archived rewards/behavior domain still FK-restricts children → members, and
// many of those tables are append-only via BEFORE DELETE triggers. For an
// explicit household member delete we temporarily drop those delete guards,
// purge the child subtree, then restore the triggers from sqlite_master SQL.
const CHILD_HISTORY_DELETE_TRIGGERS = [
  'behavior_action_requests_no_delete',
  'behavior_observation_amendments_no_delete',
  'behavior_observation_contributions_no_delete',
  'behavior_observation_reward_contributions_no_delete',
  'behavior_observations_no_delete',
  'behavior_incidents_no_delete',
  'consequences_no_delete',
  'recovery_actions_no_delete',
  'star_transactions_no_delete',
  'reward_redemptions_no_delete',
  'reward_term_days_no_delete',
  'reward_terms_no_delete',
  'task_completion_requests_no_delete',
  'child_focus_sessions_no_delete',
  'child_onboarding_drafts_no_delete',
];

function deleteChildDomainRows(db, childId) {
  const childExists = db.prepare('SELECT 1 AS ok FROM children WHERE id = ?').get(childId);
  if (!childExists) return;

  const placeholders = CHILD_HISTORY_DELETE_TRIGGERS.map(() => '?').join(',');
  const triggerRows = db
    .prepare(
      `SELECT name, sql FROM sqlite_master
       WHERE type = 'trigger' AND name IN (${placeholders})`
    )
    .all(...CHILD_HISTORY_DELETE_TRIGGERS);

  for (const row of triggerRows) {
    db.exec(`DROP TRIGGER IF EXISTS ${row.name}`);
  }

  try {
    db.prepare(
      `DELETE FROM behavior_observation_contributions
       WHERE observation_id IN (SELECT id FROM behavior_observations WHERE child_id = ?)`
    ).run(childId);
    db.prepare(
      `DELETE FROM behavior_observation_amendments
       WHERE observation_id IN (SELECT id FROM behavior_observations WHERE child_id = ?)`
    ).run(childId);
    db.prepare(
      `DELETE FROM behavior_observation_reward_contributions
       WHERE observation_id IN (SELECT id FROM behavior_observations WHERE child_id = ?)`
    ).run(childId);
    db.prepare('DELETE FROM behavior_action_requests WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM behavior_observations WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM behavior_reward_links WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM behavior_definition_children WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM recovery_actions WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM consequences WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM behavior_incidents WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM behavior_rule_children WHERE child_id = ?').run(childId);

    // Stars reference redemptions/completions — remove stars first.
    db.prepare('DELETE FROM star_transactions WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM reward_redemptions WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM child_reward_goals WHERE child_id = ?').run(childId);
    db.prepare(
      `DELETE FROM reward_term_days
       WHERE term_id IN (SELECT id FROM reward_terms WHERE child_id = ?)`
    ).run(childId);
    db.prepare(
      'DELETE FROM reward_terms WHERE child_id = ? AND reset_of IS NOT NULL'
    ).run(childId);
    db.prepare('DELETE FROM reward_terms WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM reward_child_access WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM child_focus_sessions WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM task_completion_requests WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM task_assignments WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM child_attendance_rules WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM display_child_profiles WHERE child_id = ?').run(childId);
    db.prepare(
      'UPDATE child_onboarding_drafts SET activated_child_id = NULL WHERE activated_child_id = ?'
    ).run(childId);
    db.prepare('DELETE FROM child_households WHERE child_id = ?').run(childId);
    db.prepare('DELETE FROM children WHERE id = ?').run(childId);
  } finally {
    for (const row of triggerRows) {
      if (row.sql) db.exec(row.sql);
    }
  }
}

function scrubMemberIdFromJsonArrays(db, memberId) {
  const targets = [
    { table: 'reminders', col: 'notify_member_ids' },
    { table: 'events', col: 'member_ids' },
  ];
  for (const { table, col } of targets) {
    const exists = db
      .prepare("SELECT 1 AS ok FROM sqlite_master WHERE type='table' AND name=?")
      .get(table);
    if (!exists) continue;
    const rows = db.prepare(`SELECT id, ${col} AS raw FROM ${table}`).all();
    for (const row of rows) {
      let arr;
      try {
        arr = JSON.parse(row.raw || '[]');
      } catch {
        continue;
      }
      if (!Array.isArray(arr) || !arr.includes(memberId)) continue;
      const next = arr.filter((x) => x !== memberId);
      db.prepare(`UPDATE ${table} SET ${col} = ? WHERE id = ?`).run(
        JSON.stringify(next),
        row.id
      );
    }
  }
}

function deleteMember(id) {
  const db = getDb();
  db.exec('BEGIN IMMEDIATE');
  try {
    if (!getMember(id)) {
      db.exec('ROLLBACK');
      return false;
    }
    deleteChildDomainRows(db, id);
    scrubMemberIdFromJsonArrays(db, id);
    const result = db.prepare('DELETE FROM family_members WHERE id = ?').run(id);
    if (result.changes > 0) bumpStateVersion(db);
    db.exec('COMMIT');
    return result.changes > 0;
  } catch (err) {
    try {
      db.exec('ROLLBACK');
    } catch {
      /* ignore */
    }
    throw err;
  }
}

module.exports = {
  listMembers,
  getMember,
  getMemberByClerkUserId,
  linkClerkUser,
  unlinkClerkUser,
  createMember,
  updateMember,
  deleteMember,
};
