'use strict';
process.env.NODE_ENV = process.env.NODE_ENV || 'test';

const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { DatabaseSync } = require('node:sqlite');
const {
  applyMigrations,
  migrateDb,
  MIGRATIONS_DIR,
} = require('../db/init');

const baselineSql = fs.readFileSync(path.join(__dirname, '..', 'db', 'schema.sql'), 'utf8');

function fixtureDb() {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'family-hub-migration-'));
  const file = path.join(dir, 'fixture.sqlite');
  const db = new DatabaseSync(file);
  db.exec('PRAGMA foreign_keys = ON; PRAGMA busy_timeout = 5000;');
  return {
    db,
    cleanup() {
      db.close();
      fs.rmSync(dir, { recursive: true, force: true });
    },
  };
}

function installBaseline(db) {
  db.exec(baselineSql);
  migrateDb(db);
}

function tableNames(db) {
  return db.prepare("SELECT name FROM sqlite_master WHERE type='table'").all().map((row) => row.name);
}

test('fresh database applies ordered migrations and reruns as a no-op', () => {
  const fixture = fixtureDb();
  try {
    installBaseline(fixture.db);
    assert.equal(applyMigrations(fixture.db), 8);
    const required = [
      'children', 'child_attendance_rules', 'task_definitions', 'task_assignments',
      'task_completion_requests', 'star_transactions', 'reward_definitions',
      'reward_child_access', 'reward_terms', 'reward_term_days', 'child_reward_goals',
      'reward_redemptions', 'behavior_rules', 'behavior_incidents', 'consequences',
      'media_assets', 'display_settings', 'audit_events', 'domain_idempotency_keys',
      'display_child_profiles', 'child_focus_sessions',
      'parent_credentials', 'parent_sessions', 'auth_rate_limits', 'auth_system_secrets',
      'behavior_rule_children', 'recovery_actions', 'routine_groups', 'routine_group_tasks',
      'households', 'child_households', 'behavior_definitions',
      'behavior_definition_children', 'behavior_observations',
      'behavior_observation_amendments', 'behavior_observation_contributions',
      'behavior_reward_links', 'behavior_observation_reward_contributions',
      'behavior_action_requests', 'child_onboarding_drafts',
    ];
    const names = new Set(tableNames(fixture.db));
    for (const name of required) assert.ok(names.has(name), `missing table ${name}`);
    const ledgerBefore = fixture.db.prepare('SELECT * FROM schema_migrations ORDER BY version').all();
    assert.deepEqual(ledgerBefore.map((row) => row.version), [1, 2, 3, 4, 5, 6, 7, 8]);
    assert.equal(applyMigrations(fixture.db), 0);
    assert.deepEqual(fixture.db.prepare('SELECT * FROM schema_migrations ORDER BY version').all(), ledgerBefore);
    assert.equal(fixture.db.prepare('PRAGMA integrity_check').get().integrity_check, 'ok');
    assert.deepEqual(fixture.db.prepare('PRAGMA foreign_key_check').all(), []);
  } finally {
    fixture.cleanup();
  }
});

test('existing baseline data survives additive migrations unchanged', () => {
  const fixture = fixtureDb();
  try {
    installBaseline(fixture.db);
    fixture.db.prepare(
      "INSERT INTO family_members (id, name, color, avatar_emoji) VALUES ('adult-1', 'Parent', '#111111', 'P')"
    ).run();
    fixture.db.prepare(
      "INSERT INTO chores (id, title, assignee_id) VALUES ('chore-1', 'Existing chore', 'adult-1')"
    ).run();
    fixture.db.prepare("INSERT INTO grocery_items (id, text) VALUES ('g-1', 'Existing item')").run();
    fixture.db.prepare("INSERT INTO notes (id, text) VALUES ('n-1', 'Existing note')").run();
    const before = {
      member: fixture.db.prepare("SELECT * FROM family_members WHERE id = 'adult-1'").get(),
      chore: fixture.db.prepare("SELECT * FROM chores WHERE id = 'chore-1'").get(),
      grocery: fixture.db.prepare("SELECT * FROM grocery_items WHERE id = 'g-1'").get(),
      note: fixture.db.prepare("SELECT * FROM notes WHERE id = 'n-1'").get(),
    };
    applyMigrations(fixture.db);
    assert.deepEqual(fixture.db.prepare("SELECT * FROM family_members WHERE id = 'adult-1'").get(), before.member);
    assert.deepEqual(fixture.db.prepare("SELECT * FROM chores WHERE id = 'chore-1'").get(), before.chore);
    assert.deepEqual(fixture.db.prepare("SELECT * FROM grocery_items WHERE id = 'g-1'").get(), before.grocery);
    assert.deepEqual(fixture.db.prepare("SELECT * FROM notes WHERE id = 'n-1'").get(), before.note);
  } finally {
    fixture.cleanup();
  }
});

test('child-specific attended and fixed-length terms are representable independently', () => {
  const fixture = fixtureDb();
  try {
    installBaseline(fixture.db);
    applyMigrations(fixture.db);
    const insertMember = fixture.db.prepare(
      'INSERT INTO family_members (id, name, color, avatar_emoji) VALUES (?, ?, ?, ?)'
    );
    const insertChild = fixture.db.prepare(
      'INSERT INTO children (id, display_name, age_mode) VALUES (?, ?, ?)'
    );
    insertMember.run('child-three', 'Three', '#123456', '3');
    insertMember.run('child-two', 'Two', '#654321', '2');
    insertChild.run('child-three', 'Three', 'preschool');
    insertChild.run('child-two', 'Two', 'toddler');
    fixture.db.prepare(`
      INSERT INTO reward_terms (
        id, child_id, name, term_type, term_config, required_successful_days, status
      ) VALUES (?, ?, ?, ?, ?, ?, 'active')
    `).run('term-attended', 'child-three', 'Visit', 'attended_day', '{"length":3}', 2);
    fixture.db.prepare(`
      INSERT INTO reward_terms (
        id, child_id, name, term_type, term_config, required_successful_days, status
      ) VALUES (?, ?, ?, ?, ?, ?, 'active')
    `).run('term-fixed', 'child-two', 'Short', 'fixed_length', '{"length":2}', 2);
    fixture.db.prepare(`
      INSERT INTO reward_term_days (
        id, term_id, calendar_date, attendance_status, day_status,
        progress_value, daily_progress_value
      ) VALUES ('day-1', 'term-attended', '2030-01-01', 'present', 'successful', 1, 4)
    `).run();
    const terms = fixture.db.prepare(
      'SELECT child_id, term_type, term_config, required_successful_days FROM reward_terms ORDER BY child_id'
    ).all();
    assert.deepEqual(terms.map((row) => row.term_type).sort(), ['attended_day', 'fixed_length']);
    const day = fixture.db.prepare("SELECT progress_value, daily_progress_value FROM reward_term_days WHERE id='day-1'").get();
    assert.equal(day.progress_value, 1);
    assert.equal(day.daily_progress_value, 4);
  } finally {
    fixture.cleanup();
  }
});

test('constraints reject orphans, invalid costs, duplicate term days, and duplicate reversals', () => {
  const fixture = fixtureDb();
  try {
    installBaseline(fixture.db);
    applyMigrations(fixture.db);
    assert.throws(() => fixture.db.prepare(
      "INSERT INTO children (id, display_name) VALUES ('missing-member', 'Nope')"
    ).run(), /FOREIGN KEY/);
    assert.throws(() => fixture.db.prepare(
      "INSERT INTO reward_definitions (id, name, reward_type, star_cost) VALUES ('bad', 'Bad', 'spendable', -1)"
    ).run(), /CHECK/);

    fixture.db.prepare("INSERT INTO family_members (id, name) VALUES ('child', 'Child')").run();
    fixture.db.prepare("INSERT INTO children (id, display_name) VALUES ('child', 'Child')").run();
    fixture.db.prepare(`
      INSERT INTO reward_terms (id, child_id, name, term_type, status)
      VALUES ('term', 'child', 'Term', 'manual', 'active')
    `).run();
    fixture.db.prepare(`
      INSERT INTO reward_term_days (id, term_id, calendar_date)
      VALUES ('d1', 'term', '2030-01-01')
    `).run();
    assert.throws(() => fixture.db.prepare(`
      INSERT INTO reward_term_days (id, term_id, calendar_date)
      VALUES ('d2', 'term', '2030-01-01')
    `).run(), /UNIQUE/);
    fixture.db.prepare(`
      INSERT INTO star_transactions (id, child_id, amount, transaction_type, reason)
      VALUES ('award', 'child', 1, 'award', 'Test')
    `).run();
    fixture.db.prepare(`
      INSERT INTO star_transactions (id, child_id, amount, transaction_type, reason, reversal_of)
      VALUES ('reverse-1', 'child', -1, 'reversal', 'Fix', 'award')
    `).run();
    assert.throws(() => fixture.db.prepare(`
      INSERT INTO star_transactions (id, child_id, amount, transaction_type, reason, reversal_of)
      VALUES ('reverse-2', 'child', -1, 'reversal', 'Again', 'award')
    `).run(), /UNIQUE/);
  } finally {
    fixture.cleanup();
  }
});

test('all approved term and day states persist without a seven-day assumption', () => {
  const fixture = fixtureDb();
  try {
    installBaseline(fixture.db);
    applyMigrations(fixture.db);
    fixture.db.prepare("INSERT INTO family_members (id, name) VALUES ('child-all', 'Child')").run();
    fixture.db.prepare("INSERT INTO children (id, display_name) VALUES ('child-all', 'Child')").run();
    const termTypes = [
      'fixed_length', 'scheduled_day', 'attended_day', 'calendar_week',
      'rolling', 'custom_date', 'manual',
    ];
    const insertTerm = fixture.db.prepare(`
      INSERT INTO reward_terms (
        id, child_id, name, term_type, term_config, required_successful_days, status
      ) VALUES (?, 'child-all', ?, ?, ?, ?, ?)
    `);
    termTypes.forEach((type, index) => {
      const status = index === 0 ? 'active' : 'draft';
      insertTerm.run(`term-${index}`, type, type, JSON.stringify({ length: index + 2 }), 1, status);
    });
    assert.equal(fixture.db.prepare('SELECT COUNT(*) AS count FROM reward_terms').get().count, 7);
    assert.equal(
      fixture.db.prepare("SELECT term_config FROM reward_terms WHERE term_type = 'fixed_length'").get().term_config,
      '{"length":2}'
    );
    fixture.db.prepare("UPDATE reward_terms SET status = 'paused' WHERE id = 'term-0'").run();
    fixture.db.prepare("UPDATE reward_terms SET status = 'completed' WHERE id = 'term-0'").run();
    fixture.db.prepare("UPDATE reward_terms SET status = 'reset' WHERE id = 'term-0'").run();
    assert.equal(fixture.db.prepare("SELECT status FROM reward_terms WHERE id='term-0'").get().status, 'reset');

    const dayStates = [
      'pending', 'successful', 'partially_successful', 'recovery_completed',
      'unsuccessful', 'excused', 'not_present', 'term_paused',
    ];
    const insertDay = fixture.db.prepare(`
      INSERT INTO reward_term_days (
        id, term_id, calendar_date, attendance_status, day_status
      ) VALUES (?, 'term-1', ?, ?, ?)
    `);
    dayStates.forEach((state, index) => {
      const attendance = state === 'not_present' ? 'not_present' : state === 'excused' ? 'excused' : 'present';
      insertDay.run(`state-${index}`, `2030-02-${String(index + 1).padStart(2, '0')}`, attendance, state);
    });
    assert.equal(fixture.db.prepare('SELECT COUNT(*) AS count FROM reward_term_days').get().count, 8);
  } finally {
    fixture.cleanup();
  }
});

test('ledger and audit are append-only and discipline never changes star balance', () => {
  const fixture = fixtureDb();
  try {
    installBaseline(fixture.db);
    applyMigrations(fixture.db);
    fixture.db.prepare("INSERT INTO family_members (id, name) VALUES ('child-ledger', 'Child')").run();
    fixture.db.prepare("INSERT INTO children (id, display_name) VALUES ('child-ledger', 'Child')").run();
    fixture.db.prepare(`
      INSERT INTO star_transactions (id, child_id, amount, transaction_type, reason)
      VALUES ('immutable-award', 'child-ledger', 3, 'award', 'Task')
    `).run();
    fixture.db.prepare(`
      INSERT INTO behavior_incidents (id, child_id, severity, child_message)
      VALUES ('incident-1', 'child-ledger', 'low', 'Gentle hands')
    `).run();
    const balance = fixture.db.prepare(
      "SELECT COALESCE(SUM(amount), 0) AS balance FROM star_transactions WHERE child_id='child-ledger'"
    ).get().balance;
    assert.equal(balance, 3);
    assert.throws(() => fixture.db.prepare(
      "UPDATE star_transactions SET amount = 99 WHERE id = 'immutable-award'"
    ).run(), /append-only/);
    assert.throws(() => fixture.db.prepare(
      "DELETE FROM star_transactions WHERE id = 'immutable-award'"
    ).run(), /append-only/);
    fixture.db.prepare(`
      INSERT INTO audit_events (id, event_type, entity_type, entity_id, actor_type)
      VALUES ('audit-1', 'incident.created', 'behavior_incident', 'incident-1', 'parent')
    `).run();
    assert.throws(() => fixture.db.prepare("DELETE FROM audit_events WHERE id='audit-1'").run(), /append-only/);
    assert.throws(() => fixture.db.prepare("DELETE FROM behavior_incidents WHERE id='incident-1'").run(), /cannot-be-deleted/);
  } finally {
    fixture.cleanup();
  }
});

test('a failing migration rolls back its schema and ledger entry', () => {
  const fixture = fixtureDb();
  const migrations = fs.mkdtempSync(path.join(os.tmpdir(), 'family-hub-bad-migration-'));
  try {
    installBaseline(fixture.db);
    fs.copyFileSync(path.join(MIGRATIONS_DIR, '001_reward_discipline_domain.sql'), path.join(migrations, '001_domain.sql'));
    fs.writeFileSync(
      path.join(migrations, '002_broken.sql'),
      'CREATE TABLE should_rollback (id TEXT PRIMARY KEY); INSERT INTO no_such_table VALUES (1);'
    );
    assert.throws(() => applyMigrations(fixture.db, { migrationsDir: migrations }), /no such table/);
    assert.ok(!tableNames(fixture.db).includes('should_rollback'));
    assert.equal(
      fixture.db.prepare('SELECT COUNT(*) AS count FROM schema_migrations WHERE version = 2').get().count,
      0
    );
  } finally {
    fixture.cleanup();
    fs.rmSync(migrations, { recursive: true, force: true });
  }
});

test('phase 2 contract columns and recovery linkage are available', () => {
  const fixture = fixtureDb();
  try {
    installBaseline(fixture.db);
    applyMigrations(fixture.db);
    const columns = (table) => new Set(
      fixture.db.prepare(`PRAGMA table_info(${table})`).all().map((row) => row.name)
    );
    for (const name of ['effective_star_value', 'effective_daily_progress', 'rejected_by', 'request_fingerprint']) {
      assert.ok(columns('task_completion_requests').has(name));
    }
    for (const name of ['qualification_mode', 'release_timing', 'ready_at', 'reset_of', 'cycle_number']) {
      assert.ok(columns('reward_terms').has(name));
    }
    for (const name of ['goal_id', 'declined_by', 'redeemed_at', 'completed_by', 'cost_source']) {
      assert.ok(columns('reward_redemptions').has(name));
    }
    assert.ok(columns('domain_idempotency_keys').has('request_hash'));
    assert.ok(tableNames(fixture.db).includes('recovery_actions'));
  } finally {
    fixture.cleanup();
  }
});

test('phase 3 administrative contract adds routines, richer settings, and media lineage', () => {
  const fixture = fixtureDb();
  try {
    installBaseline(fixture.db);
    applyMigrations(fixture.db);
    const columns = (table) => new Set(fixture.db.prepare(`PRAGMA table_info(${table})`).all().map((row) => row.name));
    for (const name of ['exceptions_config']) assert.ok(columns('task_assignments').has(name));
    for (const name of ['resolution_type', 'parent_note']) assert.ok(columns('task_completion_requests').has(name));
    for (const name of ['usage_limit', 'availability_config', 'display_priority', 'child_audio_ref']) assert.ok(columns('reward_definitions').has(name));
    for (const name of ['enabled', 'reset_behavior', 'automatic_restart', 'approval_required']) assert.ok(columns('reward_terms').has(name));
    for (const name of ['show_child_profiles', 'visual_only_mode', 'show_consequence_indicator', 'profile_lock_timeout_seconds', 'cache_refresh_seconds', 'reward_ordering']) assert.ok(columns('display_settings').has(name));
    for (const name of ['logical_asset_id', 'replaces_id']) assert.ok(columns('media_assets').has(name));
    assert.equal(fixture.db.prepare('PRAGMA integrity_check').get().integrity_check, 'ok');
    assert.deepEqual(fixture.db.prepare('PRAGMA foreign_key_check').all(), []);
  } finally { fixture.cleanup(); }
});

test('phase 5 corrective contract adds complete Child Focus configuration and history', () => {
  const fixture = fixtureDb();
  try {
    const db = fixture.db;
    installBaseline(db);
    applyMigrations(db);
    const columns = (table) => new Set(db.prepare(`PRAGMA table_info(${table})`).all().map((row) => row.name));
    for (const name of [
      'child_focus_enabled', 'show_child_mode_toggle', 'default_child_id',
      'skip_child_selection_single', 'require_pin_exit', 'require_pin_change_child',
      'restart_mode', 'auto_return_enabled', 'child_dashboard_timeout_seconds',
      'show_task_grid', 'sync_interval_seconds',
    ]) assert.ok(columns('display_settings').has(name), name);
    assert.ok(columns('display_child_profiles').has('child_id'));
    assert.ok(columns('child_focus_sessions').has('end_condition'));
    assert.throws(() => db.prepare(`INSERT INTO display_settings (id,device_id,restart_mode) VALUES ('bad','bad','invalid')`).run());
  } finally { fixture.cleanup(); }
});

test('migrations 005 through 008 preserve settings and backfill split task progress', () => {
  const fixture = fixtureDb();
  const prior = fs.mkdtempSync(path.join(os.tmpdir(), 'family-hub-prior-migrations-'));
  try {
    installBaseline(fixture.db);
    for (const name of fs.readdirSync(MIGRATIONS_DIR).filter((name) => Number.parseInt(name.slice(0, 3), 10) < 5)) {
      fs.copyFileSync(path.join(MIGRATIONS_DIR, name), path.join(prior, name));
    }
    assert.equal(applyMigrations(fixture.db, { migrationsDir: prior }), 4);
    fixture.db.prepare(`INSERT INTO display_settings (id,device_id,reward_module_enabled,visible_reward_limit)
      VALUES ('existing-settings','existing-panel',1,7)`).run();
    fixture.db.exec(`
      INSERT INTO family_members (id,name) VALUES ('split-child','Split');
      INSERT INTO children (id,display_name) VALUES ('split-child','Split');
      INSERT INTO task_definitions (id,name,default_daily_progress)
        VALUES ('split-task','Split task',2);
      INSERT INTO task_assignments
        (id,task_id,child_id,schedule_type,daily_progress_override)
        VALUES ('split-assignment','split-task','split-child','daily',1.5);
      INSERT INTO task_completion_requests
        (id,assignment_id,child_id,request_key,requested_from,status,
         occurrence_date,effective_daily_progress,request_fingerprint)
        VALUES ('split-request','split-assignment','split-child','split-request-key',
          'panel','approved','2030-01-01',1.5,'split-fingerprint');
      INSERT INTO reward_terms (id,child_id,name,term_type,status)
        VALUES ('split-term','split-child','Split term','manual','active');
      INSERT INTO reward_term_days
        (id,term_id,calendar_date,daily_progress_value)
        VALUES ('split-day','split-term','2030-01-01',3);
    `);
    assert.equal(applyMigrations(fixture.db), 4);
    const row = fixture.db.prepare("SELECT * FROM display_settings WHERE id='existing-settings'").get();
    assert.equal(row.reward_module_enabled, 1);
    assert.equal(row.visible_reward_limit, 7);
    assert.equal(row.child_focus_enabled, 0);
    assert.equal(row.restart_mode, 'restore');
    assert.equal(row.behavior_goals_enabled, 1);
    assert.equal(row.max_behavior_goals, 3);
    assert.equal(row.profile_switching_enabled, 1);
    assert.equal(fixture.db.prepare("SELECT default_larger_progress FROM task_definitions WHERE id='split-task'").get().default_larger_progress, 2);
    assert.equal(fixture.db.prepare("SELECT larger_progress_override FROM task_assignments WHERE id='split-assignment'").get().larger_progress_override, 1.5);
    assert.equal(fixture.db.prepare("SELECT effective_larger_progress FROM task_completion_requests WHERE id='split-request'").get().effective_larger_progress, 1.5);
    assert.equal(fixture.db.prepare("SELECT task_larger_progress_value FROM reward_term_days WHERE id='split-day'").get().task_larger_progress_value, 3);
    assert.equal(fixture.db.prepare('PRAGMA integrity_check').get().integrity_check, 'ok');
  } finally {
    fixture.cleanup();
    fs.rmSync(prior, { recursive: true, force: true });
  }
});

test('behavior and onboarding migrations enforce scope, history, and retry invariants', () => {
  const fixture = fixtureDb();
  try {
    const db = fixture.db;
    installBaseline(db);
    applyMigrations(db);
    db.exec(`
      INSERT INTO family_members (id,name) VALUES ('scope-a','A'),('scope-b','B');
      INSERT INTO children (id,display_name) VALUES ('scope-a','A'),('scope-b','B');
      INSERT INTO households (id,name) VALUES ('other','Other');
      UPDATE child_households SET household_id='other' WHERE child_id='scope-b';
      INSERT INTO behavior_definitions (
        id,household_id,name,category,classification_default,reward_eligible,
        default_star_value,child_visible,parent_only,goal_eligible
      ) VALUES ('kind','default','Kind hands','positive','positive_independent',1,1,1,0,1);
      INSERT INTO behavior_definition_children (
        household_id,behavior_definition_id,child_id,goal_enabled,goal_target
      ) VALUES ('default','kind','scope-a',1,3);
    `);

    assert.equal(db.prepare("SELECT household_id FROM child_households WHERE child_id='scope-a'").get().household_id, 'default');
    assert.throws(() => db.prepare(`INSERT INTO behavior_definition_children
      (household_id,behavior_definition_id,child_id) VALUES ('default','kind','scope-b')`).run(), /FOREIGN KEY/);

    db.prepare(`INSERT INTO behavior_incidents (id,child_id,severity)
      VALUES ('other-incident','scope-b','low')`).run();
    assert.throws(() => db.prepare(`INSERT INTO behavior_observations
      (id,household_id,child_id,behavior_definition_id,classification,occurred_at,incident_id,source)
      VALUES ('bad-link','default','scope-a','kind','challenging','2030-01-01T00:00:00.000Z','other-incident','web')`).run(), /observation-incident-scope-mismatch/);

    db.prepare(`INSERT INTO behavior_observations
      (id,household_id,child_id,behavior_definition_id,classification,occurred_at,source,idempotency_key,star_value_awarded)
      VALUES ('observation','default','scope-a','kind','positive_independent','2030-01-01T00:00:00.000Z','web','retry-key',1)`).run();
    assert.throws(() => db.prepare("UPDATE behavior_observations SET context='changed' WHERE id='observation'").run(), /append-only/);
    assert.throws(() => db.prepare("DELETE FROM behavior_observations WHERE id='observation'").run(), /append-only/);
    assert.throws(() => db.prepare(`INSERT INTO behavior_observations
      (id,household_id,child_id,behavior_definition_id,classification,occurred_at,source,idempotency_key)
      VALUES ('duplicate','default','scope-a','kind','positive_independent','2030-01-01T00:01:00.000Z','web','retry-key')`).run(), /UNIQUE/);

    db.prepare(`INSERT INTO behavior_observation_contributions
      (id,household_id,observation_id,star_value) VALUES ('contribution','default','observation',1)`).run();
    assert.throws(() => db.prepare(`INSERT INTO behavior_observation_contributions
      (id,household_id,observation_id,star_value) VALUES ('duplicate-contribution','default','observation',1)`).run(), /UNIQUE/);
    db.prepare(`INSERT INTO behavior_observations
      (id,household_id,child_id,behavior_definition_id,classification,occurred_at,source)
      VALUES ('accounting-observation','default','scope-a','kind','positive_independent',
        '2030-01-01T00:02:00.000Z','web')`).run();
    db.prepare(`INSERT INTO star_transactions
      (id,child_id,amount,transaction_type,reason)
      VALUES ('wrong-child-stars','scope-b',1,'award','Wrong child')`).run();
    assert.throws(() => db.prepare(`INSERT INTO behavior_observation_contributions
      (id,household_id,observation_id,star_transaction_id,star_value)
      VALUES ('wrong-child-contribution','default','accounting-observation','wrong-child-stars',1)`)
      .run(), /observation-star-transaction-scope-mismatch/);
    db.prepare(`INSERT INTO reward_terms
      (id,child_id,name,term_type,term_config,status)
      VALUES ('wrong-child-term','scope-b','Wrong child term','manual','{}','draft')`).run();
    assert.throws(() => db.prepare(`INSERT INTO behavior_observation_reward_contributions
      (id,household_id,observation_id,reward_scope,reward_term_id,contribution_value)
      VALUES ('wrong-child-reward','default','accounting-observation','daily','wrong-child-term',1)`)
      .run(), /observation-reward-term-scope-mismatch/);
    assert.throws(() => db.prepare(`INSERT INTO behavior_observation_amendments
      (id,household_id,observation_id,sequence_no,amendment_type,reason,recorded_by)
      VALUES ('cross-amendment','other','observation',1,'annotation','Wrong household','parent')`).run(), /FOREIGN KEY/);
    assert.throws(() => db.prepare(`INSERT INTO behavior_observation_reward_contributions
      (id,household_id,observation_id,reward_scope,contribution_value)
      VALUES ('cross-reward','other','observation','daily',1)`).run(), /FOREIGN KEY/);

    db.prepare(`INSERT INTO child_onboarding_drafts
      (id,household_id,client_key,client_hash,draft_json,created_by)
      VALUES ('draft','default','client-draft','client-hash','{}','parent-session:test')`).run();
    assert.equal(db.prepare("SELECT status FROM child_onboarding_drafts WHERE id='draft'").get().status, 'draft');
    assert.throws(() => db.prepare(`INSERT INTO child_onboarding_drafts
      (id,household_id,client_key,client_hash,draft_json,activated_child_id,created_by)
      VALUES ('cross-draft','other','cross-client','cross-hash','{}','scope-a','parent-session:test')`).run(), /FOREIGN KEY/);
    assert.throws(() => db.prepare("DELETE FROM child_onboarding_drafts WHERE id='draft'").run(), /cannot-be-deleted/);
    assert.equal(db.prepare('PRAGMA integrity_check').get().integrity_check, 'ok');
    assert.deepEqual(db.prepare('PRAGMA foreign_key_check').all(), []);
  } finally { fixture.cleanup(); }
});
