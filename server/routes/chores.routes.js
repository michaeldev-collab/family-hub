'use strict';

const express = require('express');
const chores = require('../services/chores');
const { optionalWriteAuth, panelCompleteAuth } = require('../middleware/auth');
const { logWrite } = require('../services/idempotency');

const router = express.Router();

router.get('/', (req, res) => {
  const includeCompleted = req.query.includeCompleted !== 'false';
  res.json({ items: chores.listChores({ includeCompleted }) });
});

router.get('/:id', (req, res) => {
  const item = chores.getChore(req.params.id);
  if (!item) return res.status(404).json({ error: 'not-found' });
  return res.json(item);
});

router.post('/', optionalWriteAuth, (req, res) => {
  try {
    const item = chores.createChore(req.body || {});
    logWrite('create', 'chore', item.id, 'browser');
    return res.status(201).json(item);
  } catch (err) {
    if (err.message === 'title-required') {
      return res.status(400).json({ error: 'validation', message: 'title is required' });
    }
    throw err;
  }
});

router.patch('/:id', optionalWriteAuth, (req, res) => {
  const item = chores.updateChore(req.params.id, req.body || {});
  if (!item) return res.status(404).json({ error: 'not-found' });
  logWrite('update', 'chore', item.id, 'browser');
  return res.json(item);
});

router.delete('/:id', optionalWriteAuth, (req, res) => {
  const ok = chores.deleteChore(req.params.id);
  if (!ok) return res.status(404).json({ error: 'not-found' });
  logWrite('delete', 'chore', req.params.id, 'browser');
  return res.status(204).end();
});

// Dedicated, panel-scoped completion. This is the ONLY mutation the ESP32
// panel may perform. It cannot create, edit, reassign, reprioritize, reopen,
// or delete — completeChore() touches only the completion columns.
//
// Idempotent without a key: already-complete → 200.
// With idempotency_key: duplicate posts replay the cached JSON body.
// expected_state_version mismatch → 409 version-conflict.
router.post('/:id/complete', panelCompleteAuth, (req, res) => {
  const body = req.body || {};
  const result = chores.completeChore(req.params.id, body);

  if (result.status === 'invalid-idempotency-key') {
    return res.status(400).json({
      error: 'validation',
      message: 'idempotency_key must be a non-empty string ≤ 128 chars',
    });
  }
  if (result.status === 'not-found') {
    return res.status(404).json({ error: 'not-found' });
  }
  if (result.status === 'version-conflict') {
    return res.status(409).json({
      error: 'version-conflict',
      message: 'State version mismatch',
      state_version: result.state_version,
    });
  }
  if (result.status === 'cached') {
    return res.status(200).json(result.response);
  }
  if (result.status === 'already-complete') {
    return res.status(200).json(result.response);
  }

  const source = req.get('x-family-hub-source') || 'panel';
  logWrite('complete', 'chore', result.item.id, source);
  return res.json(result.response);
});

module.exports = router;
