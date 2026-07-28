'use strict';

const express = require('express');
const config = require('../config/env');
const { getDb } = require('../db/connection');

const router = express.Router();

router.get('/health', (req, res) => {
  let dbOk = false;
  try {
    const db = getDb();
    db.prepare('SELECT 1 AS ok').get();
    dbOk = true;
  } catch (err) {
    console.error('[health] DB check failed:', err.message);
  }

  res.json({
    ok: dbOk,
    version: config.version,
    startedAt: config.startedAt,
    uptimeSeconds: Math.floor(process.uptime()),
    dbOk,
    timestamp: new Date().toISOString(),
  });
});

module.exports = router;
