'use strict';

const express = require('express');
const config = require('../config/env');
const members = require('../services/members');
const { requireClerk, requireParent } = require('../middleware/auth');

const router = express.Router();

/** Public: tells the SPA whether Clerk is on and the publishable key. */
router.get('/config', (req, res) => {
  res.json({
    clerkEnabled: config.clerkEnabled,
    publishableKey: config.clerkEnabled ? config.clerkPublishableKey : '',
    publicAppUrl: config.publicAppUrl || '',
  });
});

router.get('/me', requireClerk, (req, res) => {
  const member = members.getMemberByClerkUserId(req.auth.userId);
  res.json({
    userId: req.auth.userId,
    role: req.auth.role,
    member: member
      ? {
          id: member.id,
          name: member.name,
          color: member.color,
          avatarEmoji: member.avatarEmoji,
        }
      : null,
  });
});

router.post('/link-member', requireClerk, requireParent, (req, res) => {
  const memberId = (req.body || {}).memberId;
  if (!memberId || !String(memberId).trim()) {
    return res.status(400).json({ error: 'validation', message: 'memberId is required' });
  }
  try {
    const member = members.linkClerkUser(String(memberId).trim(), req.auth.userId);
    if (!member) return res.status(404).json({ error: 'not-found' });
    return res.json({
      userId: req.auth.userId,
      role: req.auth.role,
      member: {
        id: member.id,
        name: member.name,
        color: member.color,
        avatarEmoji: member.avatarEmoji,
        clerkUserId: member.clerkUserId,
      },
    });
  } catch (err) {
    if (err.message === 'clerk-user-linked-elsewhere') {
      return res.status(409).json({
        error: 'conflict',
        message: 'That Clerk user is already linked to another member',
      });
    }
    if (err.message === 'member-already-linked') {
      return res.status(409).json({
        error: 'conflict',
        message: 'That member is already linked to a different Clerk user',
      });
    }
    throw err;
  }
});

router.post('/unlink-member', requireClerk, requireParent, (req, res) => {
  const memberId = (req.body || {}).memberId;
  if (!memberId || !String(memberId).trim()) {
    return res.status(400).json({ error: 'validation', message: 'memberId is required' });
  }
  const member = members.unlinkClerkUser(String(memberId).trim());
  if (!member) return res.status(404).json({ error: 'not-found' });
  return res.json({ ok: true, member });
});

module.exports = router;
