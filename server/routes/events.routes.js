'use strict';

const express = require('express');
const { handleEvent } = require('../services/events');
const { optionalWriteAuth } = require('../middleware/auth');

const router = express.Router();

router.post('/', optionalWriteAuth, (req, res) => {
  try {
    const source = req.get('x-family-hub-source') || 'panel';
    const result = handleEvent(req.body || {}, source);
    return res.status(result.deduplicated ? 200 : 201).json(result);
  } catch (err) {
    const map = {
      'invalid-event-id': 400,
      'invalid-event-type': 400,
      'not-found': 404,
      'text-required': 400,
      'title-required': 400,
    };
    const status = map[err.message] || 500;
    return res.status(status).json({ error: err.message });
  }
});

module.exports = router;
