'use strict';
process.env.NODE_ENV = process.env.NODE_ENV || 'test';

const { test, before, after } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const http = require('node:http');
const os = require('node:os');
const path = require('node:path');

process.chdir(path.join(__dirname, '..'));
const testDir = fs.mkdtempSync(path.join(os.tmpdir(), 'family-hub-headers-'));
process.env.DB_PATH = path.join(testDir, 'headers.sqlite');
process.env.PORT = '0';
process.env.HOST = '127.0.0.1';
delete process.env.WRITE_TOKEN;
delete process.env.PANEL_TOKEN;

let server;
let baseUrl;

function request(method, urlPath) {
  return new Promise((resolve, reject) => {
    const url = new URL(urlPath, baseUrl);
    const req = http.request(url, { method }, (res) => {
      let data = '';
      res.on('data', (c) => (data += c));
      res.on('end', () => {
        resolve({
          status: res.statusCode,
          headers: res.headers,
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
  initDb();
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

test('API responses include security headers', async () => {
  const res = await request('GET', '/api/health');
  assert.equal(res.status, 200);
  assert.equal(res.headers['x-content-type-options'], 'nosniff');
  assert.equal(res.headers['x-frame-options'], 'DENY');
  assert.equal(res.headers['referrer-policy'], 'no-referrer');
  const csp = String(res.headers['content-security-policy'] || '');
  assert.ok(csp.includes("default-src 'self'"));
  assert.ok(csp.includes('fonts.googleapis.com'));
});

test('createApp and start are exported', () => {
  const appModule = require('../app');
  assert.equal(typeof appModule.createApp, 'function');
  assert.equal(typeof appModule.start, 'function');
  assert.equal(typeof appModule.createApp().listen, 'function');
});
