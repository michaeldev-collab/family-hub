'use strict';

/**
 * Thin facade — panel/dashboard routes keep importing getDashboardState.
 * Builders live under services/viewModels/.
 */
const {
  buildDashboard,
  PANEL_SCHEMA_VERSION,
} = require('./viewModels/dashboard');

function getDashboardState() {
  return buildDashboard();
}

module.exports = { getDashboardState, PANEL_SCHEMA_VERSION };
