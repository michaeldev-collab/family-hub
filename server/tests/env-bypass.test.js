'use strict';

const { test } = require('node:test');
const assert = require('node:assert/strict');
const path = require('path');
const { spawnSync } = require('node:child_process');

test('CLERK_TEST_BYPASS is ignored outside NODE_ENV=test', () => {
  const script = `
    process.env.NODE_ENV = 'development';
    process.env.CLERK_TEST_BYPASS = 'true';
    process.env.CLERK_SECRET_KEY = '';
    const config = require(${JSON.stringify(path.join(__dirname, '../config/env.js'))});
    if (config.clerkTestBypass !== false) process.exit(2);
  `;
  const result = spawnSync(process.execPath, ['-e', script], {
    encoding: 'utf8',
    env: { ...process.env, NODE_ENV: 'development', CLERK_TEST_BYPASS: 'true' },
  });
  assert.equal(result.status, 0, result.stderr || result.stdout);
});

test('CLERK_TEST_BYPASS throws in production', () => {
  const script = `
    process.env.NODE_ENV = 'production';
    process.env.CLERK_TEST_BYPASS = 'true';
    require(${JSON.stringify(path.join(__dirname, '../config/env.js'))});
  `;
  const result = spawnSync(process.execPath, ['-e', script], {
    encoding: 'utf8',
    env: { ...process.env, NODE_ENV: 'production', CLERK_TEST_BYPASS: 'true' },
  });
  assert.notEqual(result.status, 0);
  assert.match(result.stderr + result.stdout, /CLERK_TEST_BYPASS/);
});
