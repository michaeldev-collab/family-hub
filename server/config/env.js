'use strict';

const fs = require('fs');
const path = require('path');

function loadDotEnv() {
  const envPath = path.resolve(__dirname, '..', '.env');
  if (!fs.existsSync(envPath)) return;
  const lines = fs.readFileSync(envPath, 'utf8').split(/\r?\n/);
  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#')) continue;
    const eq = trimmed.indexOf('=');
    if (eq === -1) continue;
    const key = trimmed.slice(0, eq).trim();
    let val = trimmed.slice(eq + 1).trim();
    if (
      (val.startsWith('"') && val.endsWith('"')) ||
      (val.startsWith("'") && val.endsWith("'"))
    ) {
      val = val.slice(1, -1);
    }
    if (!(key in process.env)) process.env[key] = val;
  }
}

// Tests run hermetically: they must never inherit the developer's local .env
// (tokens, REMINDERS_ENABLED, webhook URLs), so skip it under NODE_ENV=test.
if (process.env.NODE_ENV !== 'test') loadDotEnv();

const NODE_ENV = process.env.NODE_ENV || 'development';
const APP_DATA_DIR = process.env.APP_DATA_DIR || '';
const DEFAULT_DB_PATH = APP_DATA_DIR
  ? path.join(APP_DATA_DIR, 'family-hub.sqlite')
  : path.join(__dirname, '..', 'data', 'family-hub.sqlite');

function parsePort(raw) {
  if (raw === undefined || raw === '') return 3020;
  const n = parseInt(raw, 10);
  return Number.isFinite(n) ? n : 3020;
}

function parseTrustProxy(raw) {
  if (raw === undefined || raw === '') return 'loopback';
  const v = String(raw).trim().toLowerCase();
  if (v === 'true' || v === '1' || v === 'yes') return true;
  if (v === 'false' || v === '0' || v === 'no') return false;
  if (/^\d+$/.test(v)) return Number.parseInt(v, 10);
  return raw;
}

function parseCsv(raw) {
  if (!raw) return [];
  return String(raw)
    .split(',')
    .map((s) => s.trim())
    .filter(Boolean);
}

const clerkSecretKey = process.env.CLERK_SECRET_KEY || '';
const clerkPublishableKey = process.env.CLERK_PUBLISHABLE_KEY || '';
const clerkEnabled = Boolean(clerkSecretKey);
const clerkTestBypass =
  process.env.CLERK_TEST_BYPASS === 'true' && NODE_ENV !== 'production';

module.exports = Object.freeze({
  nodeEnv: NODE_ENV,
  isProduction: NODE_ENV === 'production',
  port: parsePort(process.env.PORT),
  host: process.env.HOST || '0.0.0.0',
  dbPath: process.env.DB_PATH || DEFAULT_DB_PATH,
  writeToken: process.env.WRITE_TOKEN || '',
  // Scoped credential for household chore completion only.
  panelToken: process.env.PANEL_TOKEN || '',
  n8nReminderWebhookUrl: process.env.N8N_REMINDER_WEBHOOK_URL || '',
  remindersEnabled: process.env.REMINDERS_ENABLED === 'true',
  // Cloudflare Tunnel / reverse proxy: TRUST_PROXY=true (or hop count).
  trustProxy: parseTrustProxy(process.env.TRUST_PROXY),
  // Clerk (browser humans). Empty secret = Clerk disabled; LAN/WRITE_TOKEN rules apply.
  clerkEnabled,
  clerkSecretKey,
  clerkPublishableKey,
  clerkAuthorizedParties: parseCsv(process.env.CLERK_AUTHORIZED_PARTIES),
  clerkAllowedUserIds: process.env.CLERK_ALLOWED_USER_IDS || '',
  clerkTestBypass,
  // Reject PANEL_TOKEN when request carries Cloudflare headers (tunnel).
  rejectPanelViaTunnel: process.env.REJECT_PANEL_VIA_TUNNEL !== 'false',
  publicAppUrl: process.env.PUBLIC_APP_URL || '',
  version: '0.1.0',
  startedAt: new Date().toISOString(),
});
