'use strict';
process.env.NODE_ENV = 'test';

const { test, before, after } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('path');
const crypto = require('node:crypto');

process.chdir(path.join(__dirname, '..'));
const testDir = fs.mkdtempSync(path.join(os.tmpdir(), 'family-hub-clerk-'));
process.env.DB_PATH = path.join(testDir, 'clerk.sqlite');

let server;
let baseUrl;

function testBearer(role, userId = 'user_test_parent') {
  const payload = Buffer.from(
    JSON.stringify({ sub: userId, publicMetadata: { role } }),
    'utf8'
  ).toString('base64url');
  return `test.${payload}`;
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
          let parsed = null;
          if (data) {
            try {
              parsed = JSON.parse(data);
            } catch {
              parsed = data;
            }
          }
          resolve({ status: res.statusCode, body: parsed, headers: res.headers });
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

before(async () => {
  process.env.PORT = '0';
  process.env.HOST = '127.0.0.1';
  process.env.WRITE_TOKEN = '';
  process.env.PANEL_TOKEN = 'panel-secret';
  process.env.CLERK_SECRET_KEY = 'sk_test_fake';
  process.env.CLERK_PUBLISHABLE_KEY = 'pk_test_fake';
  process.env.CLERK_TEST_BYPASS = 'true';
  process.env.REJECT_PANEL_VIA_TUNNEL = 'true';
  process.env.TRUST_PROXY = 'true';

  clearServerModules();
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
  try {
    const { closeDb } = require('../db/connection');
    closeDb();
  } catch (_) {
    /* ignore */
  }
  fs.rmSync(testDir, { recursive: true, force: true });
});

test('auth config exposes clerk when enabled', async () => {
  const res = await request('GET', '/api/auth/config');
  assert.equal(res.status, 200);
  assert.equal(res.body.clerkEnabled, true);
  assert.equal(res.body.publishableKey, 'pk_test_fake');
});

test('dashboard-state stays public for the panel', async () => {
  const res = await request('GET', '/api/dashboard-state');
  assert.equal(res.status, 200);
  assert.ok(res.body.today);
});

test('writes without Clerk JWT are rejected when Clerk is on', async () => {
  const res = await request('POST', '/api/grocery', { text: 'No auth milk' });
  assert.equal(res.status, 401);
  assert.equal(res.body.error, 'unauthorized');
});

test('parent Clerk JWT can create grocery', async () => {
  const res = await request(
    'POST',
    '/api/grocery',
    { text: `Clerk milk ${crypto.randomUUID().slice(0, 6)}` },
    { Authorization: `Bearer ${testBearer('parent')}` }
  );
  assert.equal(res.status, 201);
  assert.ok(res.body.id);
});

test('kid Clerk JWT cannot rename other title', async () => {
  const res = await request(
    'PUT',
    '/api/grocery/meta/other-title',
    { otherTitle: 'Costco' },
    { Authorization: `Bearer ${testBearer('kid', 'user_kid')}` }
  );
  assert.equal(res.status, 403);
  assert.equal(res.body.error, 'forbidden');
});

test('parent can link Clerk user to a member', async () => {
  const members = await request('GET', '/api/members');
  assert.equal(members.status, 200);
  const memberId = members.body.items[0].id;
  const res = await request(
    'POST',
    '/api/auth/link-member',
    { memberId },
    { Authorization: `Bearer ${testBearer('parent', 'user_link_me')}` }
  );
  assert.equal(res.status, 200);
  assert.equal(res.body.member.id, memberId);
  assert.equal(res.body.member.clerkUserId, 'user_link_me');

  const me = await request('GET', '/api/auth/me', null, {
    Authorization: `Bearer ${testBearer('parent', 'user_link_me')}`,
  });
  assert.equal(me.status, 200);
  assert.equal(me.body.member.id, memberId);
});

test('panel token works for grocery toggle on LAN', async () => {
  const created = await request(
    'POST',
    '/api/grocery',
    { text: `Toggle ${crypto.randomUUID().slice(0, 6)}`, listKey: 'main' },
    { Authorization: `Bearer ${testBearer('parent')}` }
  );
  assert.equal(created.status, 201);
  const res = await request('POST', `/api/grocery/${created.body.id}/toggle`, {}, {
    'x-family-hub-token': 'panel-secret',
  });
  assert.equal(res.status, 200);
});

test('panel token is rejected when Cloudflare headers present', async () => {
  const created = await request(
    'POST',
    '/api/grocery',
    { text: `Tunnel ${crypto.randomUUID().slice(0, 6)}`, listKey: 'main' },
    { Authorization: `Bearer ${testBearer('parent')}` }
  );
  assert.equal(created.status, 201);
  const res = await request(
    'POST',
    `/api/grocery/${created.body.id}/toggle`,
    {},
    {
      'x-family-hub-token': 'panel-secret',
      'cf-ray': 'test-ray',
    }
  );
  assert.equal(res.status, 403);
  assert.equal(res.body.error, 'panel-lan-only');
});
