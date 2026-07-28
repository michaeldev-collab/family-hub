'use strict';

const express = require('express');
const notes = require('../services/notes');
const { optionalWriteAuth } = require('../middleware/auth');
const { logWrite } = require('../services/idempotency');

const router = express.Router();

router.get('/', (req, res) => {
  res.json({ items: notes.listNotes() });
});

router.get('/:id', (req, res) => {
  const item = notes.getNote(req.params.id);
  if (!item) return res.status(404).json({ error: 'not-found' });
  return res.json(item);
});

router.post('/', optionalWriteAuth, (req, res) => {
  try {
    const item = notes.createNote(req.body || {});
    logWrite('create', 'note', item.id, 'browser');
    return res.status(201).json(item);
  } catch (err) {
    if (err.message === 'text-required') {
      return res.status(400).json({ error: 'validation', message: 'text is required' });
    }
    throw err;
  }
});

router.patch('/:id', optionalWriteAuth, (req, res) => {
  const item = notes.updateNote(req.params.id, req.body || {});
  if (!item) return res.status(404).json({ error: 'not-found' });
  logWrite('update', 'note', item.id, 'browser');
  return res.json(item);
});

router.delete('/:id', optionalWriteAuth, (req, res) => {
  const ok = notes.deleteNote(req.params.id);
  if (!ok) return res.status(404).json({ error: 'not-found' });
  logWrite('delete', 'note', req.params.id, 'browser');
  return res.status(204).end();
});

module.exports = router;
