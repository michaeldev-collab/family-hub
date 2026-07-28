'use strict';
process.env.NODE_ENV = process.env.NODE_ENV || 'test';

const { test, before, after } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');

process.chdir(path.join(__dirname, '..'));
const testDir = fs.mkdtempSync(path.join(os.tmpdir(), 'family-hub-etag-'));
process.env.DB_PATH = path.join(testDir, 'etag.sqlite');
process.env.PORT = '0';
process.env.HOST = '127.0.0.1';
delete process.env.WRITE_TOKEN;
delete process.env.PANEL_TOKEN;

let server;
let baseUrl;

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
          resolve({
            status: res.statusCode,
            headers: res.headers,
            body: parsed,
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

test('GET /api/dashboard-state returns ETag matching state_version', async () => {
  const res = await request('GET', '/api/dashboard-state');
  assert.equal(res.status, 200);
  assert.equal(typeof res.body.state_version, 'number');
  assert.equal(res.headers.etag, `"${res.body.state_version}"`);
});

test('If-None-Match returns 304 when unchanged', async () => {
  const first = await request('GET', '/api/dashboard-state');
  assert.equal(first.status, 200);
  const etag = first.headers.etag;
  assert.ok(etag);

  const second = await request('GET', '/api/dashboard-state', null, {
    'If-None-Match': etag,
  });
  assert.equal(second.status, 304);
  assert.equal(second.body, null);
});

test('mutation bumps state_version and breaks prior ETag', async () => {
  const before = await request('GET', '/api/dashboard-state');
  const etag = before.headers.etag;
  const versionBefore = before.body.state_version;

  const created = await request('POST', '/api/grocery', {
    text: `etag-bump-${Date.now()}`,
  });
  assert.equal(created.status, 201);

  const stale = await request('GET', '/api/dashboard-state', null, {
    'If-None-Match': etag,
  });
  assert.equal(stale.status, 200);
  assert.ok(stale.body.state_version > versionBefore);
  assert.notEqual(stale.headers.etag, etag);
});
