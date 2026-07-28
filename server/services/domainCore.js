'use strict';

// Thin stub kept for the Express error handler after Rewards & Behavior was
// archived. Full domain helpers live in the sibling archive's domainCore.js.

class DomainError extends Error {
  constructor(code, status = 409, details = undefined) {
    super(code);
    this.code = code;
    this.status = status;
    this.details = details;
  }
}

module.exports = { DomainError };
