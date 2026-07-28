'use strict';

const express = require('express');
const members = require('../services/members');
const { optionalWriteAuth, requireParent } = require('../middleware/auth');
const { logWrite } = require('../services/idempotency');

const router = express.Router();

router.get('/', (req, res) => {
  res.json({ items: members.listMembers() });
});

router.get('/:id', (req, res) => {
  const item = members.getMember(req.params.id);
  if (!item) return res.status(404).json({ error: 'not-found' });
  return res.json(item);
});

router.post('/', optionalWriteAuth, requireParent, (req, res) => {
  const { name } = req.body || {};
  if (!name || !String(name).trim()) {
    return res.status(400).json({ error: 'validation', message: 'name is required' });
  }
  const item = members.createMember(req.body);
  logWrite('create', 'member', item.id, 'browser');
  return res.status(201).json(item);
});

router.patch('/:id', optionalWriteAuth, requireParent, (req, res) => {
  const item = members.updateMember(req.params.id, req.body || {});
  if (!item) return res.status(404).json({ error: 'not-found' });
  logWrite('update', 'member', item.id, 'browser');
  return res.json(item);
});

router.delete('/:id', optionalWriteAuth, requireParent, (req, res) => {
  const ok = members.deleteMember(req.params.id);
  if (!ok) return res.status(404).json({ error: 'not-found' });
  logWrite('delete', 'member', req.params.id, 'browser');
  return res.status(204).end();
});

module.exports = router;
