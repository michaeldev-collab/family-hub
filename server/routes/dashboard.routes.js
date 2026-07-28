'use strict';

const express = require('express');
const { getDashboardState } = require('../services/dashboardState');

const router = express.Router();

function etagFor(stateVersion) {
  return `"${stateVersion}"`;
}

function maybeNotModified(req, res, payload) {
  const etag = etagFor(payload.state_version);
  res.setHeader('ETag', etag);
  const inm = req.headers['if-none-match'];
  if (inm && inm === etag) {
    res.status(304).end();
    return true;
  }
  return false;
}

router.get('/dashboard-state', (req, res) => {
  const payload = getDashboardState();
  if (maybeNotModified(req, res, payload)) return;
  res.json(payload);
});

module.exports = router;
