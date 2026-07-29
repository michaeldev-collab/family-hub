'use strict';

const { test } = require('node:test');
const assert = require('node:assert/strict');
const { redactUrl } = require('../middleware/requestLogger');

test('redactUrl strips token query params', () => {
  assert.equal(
    redactUrl('/api/chores?token=super-secret&x=1'),
    '/api/chores?token=[redacted]&x=1'
  );
  assert.equal(
    redactUrl('/api/chores?foo=1&access_token=abc'),
    '/api/chores?foo=1&access_token=[redacted]'
  );
  assert.equal(redactUrl('/api/health'), '/api/health');
});
