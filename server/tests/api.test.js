'use strict';
process.env.NODE_ENV = process.env.NODE_ENV || 'test';

const { test, before, after } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('path');

const crypto = require('node:crypto');

process.chdir(path.join(__dirname, '..'));
const testDir = fs.mkdtempSync(path.join(os.tmpdir(), 'family-hub-api-'));
process.env.DB_PATH = path.join(testDir, 'api.sqlite');

let server;
let baseUrl;
let runId;

function uniqueId(prefix) {
  return `${prefix}-${runId}-${crypto.randomUUID().slice(0, 8)}`;
}

function request(method, urlPath, body, headers = {}) {
  return new Promise((resolve, reject) => {
    const url = new URL(urlPath, baseUrl);
    const req = http.request(
      url,
      {
        method,
        headers: { 'Content-Type': 'application/json', ...headers },
      },
      (res) => {
        let data = '';
        res.on('data', (c) => (data += c));
        res.on('end', () => {
          resolve({
            status: res.statusCode,
            body: data ? JSON.parse(data) : null,
          });
        });
      }
    );
    req.on('error', reject);
    if (body) req.write(JSON.stringify(body));
    req.end();
  });
}

function clearServerModules() {
  const serverRoot = `${path.sep}server${path.sep}`;
  for (const key of Object.keys(require.cache)) {
    if (key.includes(serverRoot)) delete require.cache[key];
  }
}

function bootServer(envOverrides = {}) {
  for (const [k, v] of Object.entries(envOverrides)) {
    process.env[k] = v;
  }
  process.env.PORT = process.env.PORT || '0';
  process.env.HOST = process.env.HOST || '127.0.0.1';

  const { initDb } = require('../db/init');
  const { seed } = require('../db/seed');
  initDb();
  seed();

  clearServerModules();
  const { createApp } = require('../app');
  const app = createApp();
  const host = process.env.HOST || '127.0.0.1';
  return app.listen(0, host);
}

before(async () => {
  runId = Date.now().toString(36);
  process.env.PORT = '0';
  process.env.HOST = '127.0.0.1';
  delete process.env.WRITE_TOKEN;
  server = bootServer();
  await new Promise((r) => {
    if (server.listening) return r();
    server.on('listening', r);
  });
  const addr = server.address();
  baseUrl = `http://127.0.0.1:${addr.port}`;
});

after(async () => {
  if (server && server.listening) {
    await new Promise((resolve) => server.close(resolve));
  }
  const { closeDb } = require('../db/connection');
  closeDb();
  fs.rmSync(testDir, { recursive: true, force: true });
});

test('health returns ok', async () => {
  const res = await request('GET', '/api/health');
  assert.equal(res.status, 200);
  assert.equal(res.body.ok, true);
});

test('dashboard-state has today, members, and honest connection', async () => {
  const res = await request('GET', '/api/dashboard-state');
  assert.equal(res.status, 200);
  assert.ok(res.body.today);
  assert.ok(Array.isArray(res.body.members));
  assert.equal(res.body.connection.sourceOfTruth, 'server');
  assert.equal(res.body.connection.status, undefined);
});

test('dashboard-state exposes schema_version and nested panel VMs', async () => {
  const res = await request('GET', '/api/dashboard-state');
  assert.equal(res.status, 200);
  assert.equal(res.body.schema_version, 1);
  assert.equal(typeof res.body.state_version, 'number');
  assert.ok(res.body.generated_at);
  assert.ok(res.body.server_version);
  assert.equal(typeof res.body.home, 'object');
  assert.ok(Array.isArray(res.body.home.pinned));
  assert.equal(typeof res.body.home.open_chores_count, 'number');
  assert.equal(typeof res.body.home.grocery_count, 'number');
  assert.ok(Array.isArray(res.body.grocery.items));
  assert.ok(Array.isArray(res.body.chores.items));
  assert.equal(typeof res.body.dinner, 'object');
  assert.ok(Array.isArray(res.body.dinner.week));
  assert.ok(Array.isArray(res.body.notes.pinned));
  assert.ok(Array.isArray(res.body.notes.recent));
});

test('grocery create and idempotent event', async () => {
  const created = await request('POST', '/api/grocery', { text: 'Test eggs' });
  assert.equal(created.status, 201);
  assert.equal(created.body.text, 'Test eggs');
  const eventId = uniqueId('grocery-dup');
  const e1 = await request('POST', '/api/events', {
    eventId,
    type: 'grocery.add',
    text: 'Dup test',
  });
  assert.equal(e1.status, 201);
  const e2 = await request('POST', '/api/events', {
    eventId,
    type: 'grocery.add',
    text: 'Dup test',
  });
  assert.equal(e2.status, 200);
  assert.equal(e2.body.deduplicated, true);
});

test('chore complete and uncomplete via events', async () => {
  const chore = await request('POST', '/api/chores', { title: 'Test chore' });
  assert.equal(chore.status, 201);
  const id = chore.body.id;

  const complete = await request('POST', '/api/events', {
    eventId: uniqueId('chore-complete'),
    type: 'chore.complete',
    id,
  });
  assert.equal(complete.status, 201);
  assert.equal(complete.body.item.completed, true);

  const uncomplete = await request('POST', '/api/events', {
    eventId: uniqueId('chore-uncomplete'),
    type: 'chore.uncomplete',
    id,
  });
  assert.equal(uncomplete.status, 201);
  assert.equal(uncomplete.body.item.completed, false);
});

test('dedicated panel chore-complete endpoint is narrow and idempotent', async () => {
  const chore = await request('POST', '/api/chores', { title: 'Panel chore', dueDate: '2026-07-11' });
  assert.equal(chore.status, 201);
  const id = chore.body.id;

  // Completes and returns the updated item
  const done = await request('POST', `/api/chores/${id}/complete`, {});
  assert.equal(done.status, 200);
  assert.equal(done.body.item.completed, true);
  // Only completion changed — title/dueDate are untouched by this route
  assert.equal(done.body.item.title, 'Panel chore');
  assert.equal(done.body.item.dueDate, '2026-07-11');

  // Idempotent: completing again is a 200 no-op, not an error
  const again = await request('POST', `/api/chores/${id}/complete`, {});
  assert.equal(again.status, 200);
  assert.equal(again.body.alreadyComplete, true);

  // Unknown chore is 404
  const missing = await request('POST', '/api/chores/does-not-exist/complete', {});
  assert.equal(missing.status, 404);
});

test('chore complete idempotency_key replays cached response without re-bump', async () => {
  const chore = await request('POST', '/api/chores', { title: 'Idem chore' });
  assert.equal(chore.status, 201);
  const id = chore.body.id;
  const before = await request('GET', '/api/dashboard-state');
  const versionBefore = before.body.state_version;

  const key = uniqueId('complete-idem');
  const first = await request('POST', `/api/chores/${id}/complete`, {
    idempotency_key: key,
  });
  assert.equal(first.status, 200);
  assert.equal(first.body.ok, true);
  assert.equal(first.body.item.completed, true);
  assert.equal(typeof first.body.state_version, 'number');
  assert.ok(first.body.state_version > versionBefore);

  const mid = await request('GET', '/api/dashboard-state');
  const versionMid = mid.body.state_version;

  const second = await request('POST', `/api/chores/${id}/complete`, {
    idempotency_key: key,
  });
  assert.equal(second.status, 200);
  assert.equal(second.body.item.id, first.body.item.id);
  assert.equal(second.body.state_version, first.body.state_version);

  const after = await request('GET', '/api/dashboard-state');
  assert.equal(after.body.state_version, versionMid);
});

test('chore complete expected_state_version mismatch returns clearer 409', async () => {
  const chore = await request('POST', '/api/chores', { title: 'Conflict chore' });
  assert.equal(chore.status, 201);
  const id = chore.body.id;
  const dash = await request('GET', '/api/dashboard-state');
  const current = dash.body.state_version;

  const conflict = await request('POST', `/api/chores/${id}/complete`, {
    idempotency_key: uniqueId('conflict'),
    expected_state_version: current - 1,
  });
  assert.equal(conflict.status, 409);
  assert.equal(conflict.body.error, 'version-conflict');
  assert.equal(conflict.body.state_version, current);

  const stillOpen = await request('GET', `/api/chores/${id}`);
  assert.equal(stillOpen.body.completed, false);
});

test('panel token completes chores but cannot touch the general write surface', async () => {
  if (server) server.close();
  const scoped = bootServer({ PANEL_TOKEN: 'panel-only', WRITE_TOKEN: 'full-write' });
  await new Promise((r) => (scoped.listening ? r() : scoped.on('listening', r)));
  const scopedBase = `http://127.0.0.1:${scoped.address().port}`;

  const call = (method, urlPath, body, token) =>
    new Promise((resolve, reject) => {
      const url = new URL(urlPath, scopedBase);
      const headers = { 'Content-Type': 'application/json' };
      if (token) headers['x-family-hub-token'] = token;
      const req = http.request(url, { method, headers }, (res) => {
        let data = '';
        res.on('data', (c) => (data += c));
        res.on('end', () => resolve({ status: res.statusCode, body: data ? JSON.parse(data) : null }));
      });
      req.on('error', reject);
      if (body !== undefined) req.write(JSON.stringify(body));
      req.end();
    });

  // Seed a chore with the full write token
  const chore = await call('POST', '/api/chores', { title: 'Scoped chore' }, 'full-write');
  assert.equal(chore.status, 201);
  const id = chore.body.id;

  // Panel token CAN complete a chore
  const complete = await call('POST', `/api/chores/${id}/complete`, {}, 'panel-only');
  assert.equal(complete.status, 200);
  assert.equal(complete.body.item.completed, true);

  // Panel token CANNOT create a chore (general write surface) -> 401
  const create = await call('POST', '/api/chores', { title: 'nope' }, 'panel-only');
  assert.equal(create.status, 401);

  // Panel token CANNOT add grocery -> 401
  const grocery = await call('POST', '/api/grocery', { text: 'nope' }, 'panel-only');
  assert.equal(grocery.status, 401);

  scoped.close();
  await new Promise((r) => scoped.once('close', r));
  delete process.env.PANEL_TOKEN;
  delete process.env.WRITE_TOKEN;
  server = bootServer();
  await new Promise((r) => (server.listening ? r() : server.on('listening', r)));
  baseUrl = `http://127.0.0.1:${server.address().port}`;
});

test('write auth accepts header token only — query string token is ignored', async () => {
  if (server) server.close();
  const scoped = bootServer({ WRITE_TOKEN: 'header-only-secret' });
  await new Promise((r) => (scoped.listening ? r() : scoped.on('listening', r)));
  const scopedBase = `http://127.0.0.1:${scoped.address().port}`;

  const call = (method, urlPath, body, headers = {}) =>
    new Promise((resolve, reject) => {
      const url = new URL(urlPath, scopedBase);
      const req = http.request(
        url,
        { method, headers: { 'Content-Type': 'application/json', ...headers } },
        (res) => {
          let data = '';
          res.on('data', (c) => (data += c));
          res.on('end', () => resolve({ status: res.statusCode, body: data ? JSON.parse(data) : null }));
        }
      );
      req.on('error', reject);
      if (body !== undefined) req.write(JSON.stringify(body));
      req.end();
    });

  const viaQuery = await call('POST', '/api/chores?token=header-only-secret', { title: 'via-query' });
  assert.equal(viaQuery.status, 401);

  const viaHeader = await call('POST', '/api/chores', { title: 'via-header' }, {
    'x-family-hub-token': 'header-only-secret',
  });
  assert.equal(viaHeader.status, 201);

  scoped.close();
  await new Promise((r) => scoped.once('close', r));
  delete process.env.WRITE_TOKEN;
  server = bootServer();
  await new Promise((r) => (server.listening ? r() : server.on('listening', r)));
  baseUrl = `http://127.0.0.1:${server.address().port}`;
});

test('dinner set via event', async () => {
  const res = await request('POST', '/api/events', {
    eventId: uniqueId('dinner-set'),
    type: 'dinner.set',
    meal: 'Pasta night',
  });
  assert.equal(res.status, 201);
  assert.equal(res.body.plan.meal, 'Pasta night');
});

test('notes CRUD', async () => {
  const created = await request('POST', '/api/notes', { text: 'Test note', pinned: true });
  assert.equal(created.status, 201);
  const id = created.body.id;

  const updated = await request('PATCH', `/api/notes/${id}`, { text: 'Updated note' });
  assert.equal(updated.status, 200);
  assert.equal(updated.body.text, 'Updated note');

  const list = await request('GET', '/api/notes');
  assert.ok(list.body.items.some((n) => n.id === id));

  const removed = await request('DELETE', `/api/notes/${id}`);
  assert.equal(removed.status, 204);
});

test('members create and list', async () => {
  const created = await request('POST', '/api/members', {
    name: 'Test Member',
    avatarEmoji: '🧪',
    color: '#123456',
    phone: '+15551234567',
    notifyEnabled: true,
  });
  assert.equal(created.status, 201);
  assert.equal(created.body.phone, '+15551234567');
  assert.equal(created.body.notifyEnabled, true);
  const list = await request('GET', '/api/members');
  assert.ok(list.body.items.some((m) => m.name === 'Test Member'));
});

test('reminders create list and cancel', async () => {
  const grocery = await request('POST', '/api/grocery', { text: 'Reminder milk' });
  assert.equal(grocery.status, 201);
  const gid = grocery.body.id;

  const created = await request('POST', '/api/reminders', {
    entityType: 'grocery',
    entityId: gid,
    remindAt: '2030-01-15T18:00:00.000Z',
    message: 'Buy milk',
    notifyMemberIds: [],
  });
  assert.equal(created.status, 201);
  assert.equal(created.body.status, 'pending');
  assert.equal(created.body.entityType, 'grocery');

  const list = await request('GET', `/api/reminders?entity_type=grocery&entity_id=${gid}`);
  assert.equal(list.status, 200);
  assert.ok(list.body.items.some((r) => r.id === created.body.id));

  const cancelled = await request('DELETE', `/api/reminders/${created.body.id}`);
  assert.equal(cancelled.status, 200);
  assert.equal(cancelled.body.status, 'cancelled');
});

test('idempotent chore.complete event', async () => {
  const chore = await request('POST', '/api/chores', { title: 'Idem chore' });
  const id = chore.body.id;
  const eventId = uniqueId('chore-idem');
  const first = await request('POST', '/api/events', {
    eventId,
    type: 'chore.complete',
    id,
  });
  assert.equal(first.status, 201);
  const second = await request('POST', '/api/events', {
    eventId,
    type: 'chore.complete',
    id,
  });
  assert.equal(second.status, 200);
  assert.equal(second.body.deduplicated, true);
});

test('write token rejects unauthorized writes when configured', async () => {
  if (server) server.close();
  const authServer = bootServer({ WRITE_TOKEN: 'test-secret-token' });
  await new Promise((r) => {
    if (authServer.listening) return r();
    authServer.on('listening', r);
  });
  const addr = authServer.address();
  const authBase = `http://127.0.0.1:${addr.port}`;

  const unauthorized = await new Promise((resolve, reject) => {
    const url = new URL('/api/grocery', authBase);
    const req = http.request(
      url,
      { method: 'POST', headers: { 'Content-Type': 'application/json' } },
      (res) => {
        let data = '';
        res.on('data', (c) => (data += c));
        res.on('end', () => resolve({ status: res.statusCode, body: data ? JSON.parse(data) : null }));
      }
    );
    req.on('error', reject);
    req.write(JSON.stringify({ text: 'no token' }));
    req.end();
  });
  assert.equal(unauthorized.status, 401);

  const authorized = await new Promise((resolve, reject) => {
    const url = new URL('/api/grocery', authBase);
    const req = http.request(
      url,
      {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'x-family-hub-token': 'test-secret-token',
        },
      },
      (res) => {
        let data = '';
        res.on('data', (c) => (data += c));
        res.on('end', () => resolve({ status: res.statusCode, body: data ? JSON.parse(data) : null }));
      }
    );
    req.on('error', reject);
    req.write(JSON.stringify({ text: 'with token' }));
    req.end();
  });
  assert.equal(authorized.status, 201);

  authServer.close();
  await new Promise((r) => authServer.once('close', r));
  delete process.env.WRITE_TOKEN;
  server = bootServer();
  await new Promise((r) => {
    if (server.listening) return r();
    server.on('listening', r);
  });
  baseUrl = `http://127.0.0.1:${server.address().port}`;
});
