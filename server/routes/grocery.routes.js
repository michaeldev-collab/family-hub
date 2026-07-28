'use strict';

const express = require('express');
const grocery = require('../services/grocery');
const { optionalWriteAuth, panelCompleteAuth, requireParent } = require('../middleware/auth');
const { logWrite } = require('../services/idempotency');

const router = express.Router();

router.get('/', (req, res) => {
  const includeChecked = req.query.includeChecked !== 'false';
  const listKey = req.query.list || req.query.listKey || undefined;
  res.json({
    items: grocery.listGrocery({ includeChecked, listKey }),
    otherTitle: grocery.getOtherTitle(),
  });
});

router.get('/meta/other-title', (req, res) => {
  res.json({ otherTitle: grocery.getOtherTitle() });
});

router.put('/meta/other-title', optionalWriteAuth, requireParent, (req, res) => {
  const otherTitle = grocery.setOtherTitle((req.body || {}).otherTitle);
  logWrite('update', 'grocery-other-title', 'settings', 'browser');
  return res.json({ otherTitle });
});

router.get('/:id', (req, res) => {
  const item = grocery.getGrocery(req.params.id);
  if (!item) return res.status(404).json({ error: 'not-found' });
  return res.json(item);
});

router.post('/', optionalWriteAuth, (req, res) => {
  try {
    const item = grocery.createGrocery(req.body || {});
    logWrite('create', 'grocery', item.id, 'browser');
    return res.status(201).json(item);
  } catch (err) {
    if (err.message === 'text-required') {
      return res.status(400).json({ error: 'validation', message: 'text is required' });
    }
    throw err;
  }
});

router.patch('/:id', optionalWriteAuth, (req, res) => {
  const item = grocery.updateGrocery(req.params.id, req.body || {});
  if (!item) return res.status(404).json({ error: 'not-found' });
  logWrite('update', 'grocery', item.id, 'browser');
  return res.json(item);
});

// Panel (and web) tap toggle: constant → needed; main/other → checked.
router.post('/:id/toggle', panelCompleteAuth, (req, res) => {
  const item = grocery.toggleGroceryPanel(req.params.id);
  if (!item) return res.status(404).json({ error: 'not-found' });
  logWrite('toggle', 'grocery', item.id, 'panel');
  return res.json(item);
});

router.delete('/:id', optionalWriteAuth, (req, res) => {
  const ok = grocery.deleteGrocery(req.params.id);
  if (!ok) return res.status(404).json({ error: 'not-found' });
  logWrite('delete', 'grocery', req.params.id, 'browser');
  return res.status(204).end();
});

module.exports = router;
