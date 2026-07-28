'use strict';

const express = require('express');
const reminders = require('../services/reminders');
const { optionalWriteAuth } = require('../middleware/auth');
const { logWrite } = require('../services/idempotency');

const router = express.Router();

router.get('/', (req, res) => {
  const { entity_type: entityType, entity_id: entityId, status } = req.query;
  res.json({
    items: reminders.listReminders({ entityType, entityId, status }),
  });
});

router.get('/:id', (req, res) => {
  const item = reminders.getReminder(req.params.id);
  if (!item) return res.status(404).json({ error: 'not-found' });
  return res.json(item);
});

router.post('/', optionalWriteAuth, (req, res) => {
  try {
    const item = reminders.createReminder(req.body || {});
    logWrite('create', 'reminder', item.id, 'browser');
    return res.status(201).json(item);
  } catch (err) {
    if (err.message === 'invalid-entity-type') {
      return res.status(400).json({ error: 'validation', message: 'invalid entityType' });
    }
    if (err.message === 'entity-id-required') {
      return res.status(400).json({ error: 'validation', message: 'entityId is required' });
    }
    if (err.message === 'remind-at-required') {
      return res.status(400).json({ error: 'validation', message: 'remindAt is required' });
    }
    throw err;
  }
});

router.delete('/:id', optionalWriteAuth, (req, res) => {
  const item = reminders.cancelReminder(req.params.id);
  if (!item) return res.status(404).json({ error: 'not-found' });
  logWrite('cancel', 'reminder', req.params.id, 'browser');
  return res.json(item);
});

module.exports = router;
