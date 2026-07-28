'use strict';

const express = require('express');
const dinner = require('../services/dinner');
const { optionalWriteAuth } = require('../middleware/auth');
const { logWrite } = require('../services/idempotency');
const { todayIso } = require('../utils/helpers');

const router = express.Router();

router.get('/', (req, res) => {
  const start = req.query.start || todayIso();
  const endDate = new Date(start);
  endDate.setDate(endDate.getDate() + 6);
  const end = req.query.end || endDate.toISOString().slice(0, 10);
  res.json({ items: dinner.listDinnerRange(start, end) });
});

router.get('/:date', (req, res) => {
  const plan = dinner.getDinner(req.params.date);
  if (!plan) return res.status(404).json({ error: 'not-found' });
  return res.json(plan);
});

router.put('/:date', optionalWriteAuth, (req, res) => {
  const plan = dinner.setDinner(req.params.date, req.body || {});
  logWrite('set', 'dinner', req.params.date, 'browser');
  return res.json(plan);
});

module.exports = router;
