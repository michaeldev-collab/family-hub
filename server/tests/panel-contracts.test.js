'use strict';
process.env.NODE_ENV = process.env.NODE_ENV || 'test';

const { test, before, after } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');
const Ajv2020 = require('ajv/dist/2020');
const addFormats = require('ajv-formats');

process.chdir(path.join(__dirname, '..'));
const testDir = fs.mkdtempSync(path.join(os.tmpdir(), 'family-hub-contracts-'));
process.env.DB_PATH = path.join(testDir, 'contracts.sqlite');
process.env.PORT = '0';
process.env.HOST = '127.0.0.1';
delete process.env.WRITE_TOKEN;
delete process.env.PANEL_TOKEN;

let server;
let baseUrl;

const schemaPath = path.join(
  __dirname,
  '..',
  '..',
  'docs',
  'api',
  'schemas',
  'dashboard-v1.schema.json'
);
const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf8'));
const ajv = new Ajv2020({ allErrors: true, strict: false });
addFormats(ajv);
const validate = ajv.compile(schema);

function request(method, urlPath) {
  return new Promise((resolve, reject) => {
    const url = new URL(urlPath, baseUrl);
    const req = http.request(url, { method }, (res) => {
      let data = '';
      res.on('data', (c) => (data += c));
      res.on('end', () => {
        resolve({
          status: res.statusCode,
          body: data ? JSON.parse(data) : null,
        });
      });
    });
    req.on('error', reject);
    req.end();
  });
}

function clearServerModules() {
  const serverRoot = `${path.sep}server${path.sep}`;
  for (const key of Object.keys(require.cache)) {
    if (key.includes(serverRoot)) delete require.cache[key];
  }
}

before(async () => {
  const { initDb } = require('../db/init');
  const { seed } = require('../db/seed');
  initDb();
  seed();
  clearServerModules();
  const { createApp } = require('../app');
  const app = createApp();
  server = app.listen(0, '127.0.0.1');
  await new Promise((r) => {
    if (server.listening) return r();
    server.on('listening', r);
  });
  baseUrl = `http://127.0.0.1:${server.address().port}`;
});

after(async () => {
  if (server && server.listening) {
    await new Promise((resolve) => server.close(resolve));
  }
  const { closeDb } = require('../db/connection');
  closeDb();
  fs.rmSync(testDir, { recursive: true, force: true });
});

test('GET /api/dashboard-state matches dashboard-v1 JSON Schema', async () => {
  const res = await request('GET', '/api/dashboard-state');
  assert.equal(res.status, 200);
  const ok = validate(res.body);
  assert.equal(
    ok,
    true,
    ok ? '' : ajv.errorsText(validate.errors, { separator: '\n' })
  );
  assert.equal(res.body.schema_version, 1);
  assert.equal(typeof res.body.state_version, 'number');
  assert.ok(Array.isArray(res.body.home.pinned));
  assert.ok(Array.isArray(res.body.grocery.items));
  assert.ok(Array.isArray(res.body.chores.items));
  assert.ok(Array.isArray(res.body.dinner.week));
  assert.ok(Array.isArray(res.body.notes.pinned));
  assert.ok(Array.isArray(res.body.notes.recent));
});

test('schema rejects missing schema_version', () => {
  const bad = {
    state_version: 0,
    generated_at: new Date().toISOString(),
    server_version: '0.1.0',
    today: '2026-07-26',
    home: {
      dinner_today: null,
      dinner_cook: null,
      open_chores_count: 0,
      grocery_count: 0,
      pinned: [],
      badge: 'ONLINE',
    },
    grocery: { items: [] },
    chores: { items: [] },
    dinner: { today: null, week: [] },
    notes: { pinned: [], recent: [] },
    connection: { sourceOfTruth: 'server' },
  };
  assert.equal(validate(bad), false);
});

test('schema rejects unsupported schema_version', () => {
  const bad = {
    schema_version: 99,
    state_version: 0,
    generated_at: new Date().toISOString(),
    server_version: '0.1.0',
    today: '2026-07-26',
    home: {
      dinner_today: null,
      dinner_cook: null,
      open_chores_count: 0,
      grocery_count: 0,
      pinned: [],
      badge: 'ONLINE',
    },
    grocery: { items: [] },
    chores: { items: [] },
    dinner: { today: null, week: [] },
    notes: { pinned: [], recent: [] },
    connection: { sourceOfTruth: 'server' },
  };
  assert.equal(validate(bad), false);
});

test('schema rejects chores item without id', () => {
  const bad = {
    schema_version: 1,
    state_version: 0,
    generated_at: new Date().toISOString(),
    server_version: '0.1.0',
    today: '2026-07-26',
    home: {
      dinner_today: null,
      dinner_cook: null,
      open_chores_count: 1,
      grocery_count: 0,
      pinned: [],
      badge: 'ONLINE',
    },
    grocery: { items: [] },
    chores: { items: [{ title: 'No id' }] },
    dinner: { today: null, week: [] },
    notes: { pinned: [], recent: [] },
    connection: { sourceOfTruth: 'server' },
  };
  assert.equal(validate(bad), false);
});