'use strict';

const { test } = require('node:test');
const assert = require('node:assert/strict');
const { createWriteRateLimit } = require('../middleware/rateLimit');

function mockReq(method = 'POST', ip = '127.0.0.1') {
  return {
    method,
    ip,
    originalUrl: '/api/grocery',
    socket: { remoteAddress: ip },
  };
}

function mockRes() {
  const res = {
    statusCode: 200,
    headers: {},
    body: null,
    setHeader(k, v) {
      this.headers[k] = v;
    },
    status(code) {
      this.statusCode = code;
      return this;
    },
    json(body) {
      this.body = body;
      return this;
    },
  };
  return res;
}

test('write rate limit allows GET always', () => {
  const limit = createWriteRateLimit({ windowMs: 60_000, max: 2 });
  let nextCount = 0;
  const next = () => {
    nextCount += 1;
  };
  for (let i = 0; i < 5; i += 1) {
    limit(mockReq('GET'), mockRes(), next);
  }
  assert.equal(nextCount, 5);
});

test('write rate limit returns 429 after max writes', () => {
  const limit = createWriteRateLimit({ windowMs: 60_000, max: 3 });
  let nextCount = 0;
  const next = () => {
    nextCount += 1;
  };
  for (let i = 0; i < 3; i += 1) {
    const res = mockRes();
    limit(mockReq('POST'), res, next);
    assert.equal(res.statusCode, 200);
  }
  assert.equal(nextCount, 3);
  const blocked = mockRes();
  limit(mockReq('POST'), blocked, next);
  assert.equal(blocked.statusCode, 429);
  assert.equal(blocked.body.error, 'rate-limited');
  assert.equal(nextCount, 3);
});
