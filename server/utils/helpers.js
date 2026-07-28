'use strict';

const crypto = require('crypto');

function newId() {
  return crypto.randomUUID();
}

function clampText(value, max = 500) {
  if (typeof value !== 'string') return '';
  return value.trim().slice(0, max);
}

function parseBool(value) {
  return value === true || value === 1 || value === '1' || value === 'true';
}

function rowToBool(row, field) {
  return row[field] === 1;
}

function todayIso() {
  return new Date().toISOString().slice(0, 10);
}

module.exports = {
  newId,
  clampText,
  parseBool,
  rowToBool,
  todayIso,
};
