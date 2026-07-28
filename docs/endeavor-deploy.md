# Deploy Family Hub on endeavor (Arch)

## Prerequisites

- Node.js 22+
- systemd
- LAN access for the ESP32 panel; optional public HTTPS via Cloudflare Tunnel (see [auth-clerk-cloudflare.md](auth-clerk-cloudflare.md))

## Install

```bash
sudo mkdir -p /opt/family-hub /var/lib/family-hub
sudo cp -r server web deploy /opt/family-hub/
cd /opt/family-hub/server
sudo npm install --omit=dev
sudo cp .env.example .env
```

Edit `/opt/family-hub/server/.env`:

```ini
NODE_ENV=production
HOST=0.0.0.0
PORT=3020
DB_PATH=/var/lib/family-hub/family-hub.sqlite
# Optional — leave empty for LAN trust mode when Clerk is off
WRITE_TOKEN=
PANEL_TOKEN=
# Behind Cloudflare Tunnel:
# TRUST_PROXY=true
# PUBLIC_APP_URL=https://family.example.com
# CLERK_PUBLISHABLE_KEY=pk_...
# CLERK_SECRET_KEY=sk_...
# CLERK_AUTHORIZED_PARTIES=https://family.example.com
```

Clerk + Tunnel operator steps: [auth-clerk-cloudflare.md](auth-clerk-cloudflare.md). Example unit: [`deploy/cloudflared-family-hub.service`](../deploy/cloudflared-family-hub.service).

```bash
sudo npm run init-db
# First boot on empty DB — seed demo family data (idempotent; skips if members exist):
sudo npm run seed
```

**Seed policy:** `npm run seed` inserts demo members, chores, grocery, and dinner only when the database has no members. Re-running is safe. For a truly empty production start, run `init-db` only and add members via the browser.

## systemd

The unit loads `/opt/family-hub/server/.env` via `EnvironmentFile=-` (optional file; inline `Environment=` lines remain as defaults).

```bash
sudo useradd -r -s /usr/sbin/nologin familyhub || true
sudo chown -R familyhub:familyhub /opt/family-hub /var/lib/family-hub
sudo cp /opt/family-hub/deploy/family-hub.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now family-hub
```

After changing `.env` or the unit file:

```bash
sudo systemctl daemon-reload
sudo systemctl restart family-hub
```

## Verify

```bash
curl http://endeavor:3020/api/health
BASE_URL=http://endeavor:3020 bash /opt/family-hub/tools/smoke-test.sh
```

With write token enabled:

```bash
# In .env: WRITE_TOKEN=your-secret
BASE_URL=http://endeavor:3020 FAMILY_HUB_TOKEN=your-secret bash /opt/family-hub/tools/smoke-test.sh
```

## Panel write token

When `WRITE_TOKEN` is set on the server, clients must send `x-family-hub-token`:

| Client | How to set token |
|--------|------------------|
| Browser | `API.setWriteToken('...')` in devtools, or `localStorage.setItem('familyHubWriteToken', '...')` |
| ESP32 panel | NVS key `write_tok` (serial `w` saves test token on devkit), or build flag `-DFAMILY_HUB_WRITE_TOKEN=\"...\"` in `platformio.ini` |

Empty `WRITE_TOKEN` = LAN trust mode (no header required).

## Deploy on dev-pc (same machine as 192.168.1.132)

When this workstation **is** dev-pc (LAN IP `192.168.1.132`), Phase E does **not** require SSH to `endeavor` or `192.168.1.132`. Self-SSH often fails with `publickey` (W-Y04) even though install and systemd work with local `sudo`. Use the steps below instead of the `ssh endeavor '…'` commands in [v1-phase-plan.md](v1-roadmap/v1-phase-plan.md) Phase E.

### 1. Free port 3020

Stop any dev server before enabling systemd (both bind `0.0.0.0:3020`):

```bash
# In the terminal running repo dev server — Ctrl+C
# Or find and stop whatever holds 3020:
ss -tlnp | grep ':3020'
```

Do not leave `npm start` in `server/` running alongside `family-hub.service`.

### 2. Install / update (local — no SSH)

From the repo root:

```bash
REPO=/home/stitch/Desktop/Operating/pi-iot/family-hub
sudo mkdir -p /opt/family-hub /var/lib/family-hub
sudo cp -r "$REPO/server" "$REPO/web" "$REPO/deploy" "$REPO/tools" /opt/family-hub/
cd /opt/family-hub/server
sudo npm install --omit=dev
sudo npm run init-db
sudo npm run seed
```

Configure production settings under `/opt/family-hub/server/` per the [Install](#install) section (copy from `.env.example` if this is a fresh install).

### 3. Enable systemd

Run every command in [systemd](#systemd) on this machine with local `sudo` (not over SSH): service user, ownership on `/opt/family-hub` and `/var/lib/family-hub`, install `family-hub.service`, then `daemon-reload` and `enable --now family-hub`.

### 4. Verify (S8 / S9)

Checklist rows from [v1-verification-checklist.md](v1-roadmap/v1-verification-checklist.md):

```bash
# S9 — production unit running
systemctl is-active family-hub    # expect: active

curl -s http://127.0.0.1:3020/api/health
BASE_URL=http://192.168.1.132:3020 bash /opt/family-hub/tools/smoke-test.sh

# S8 — data survives service restart
curl -s http://127.0.0.1:3020/api/dashboard-state > /tmp/fh-before.json
sudo systemctl restart family-hub
sleep 2
curl -s http://127.0.0.1:3020/api/dashboard-state > /tmp/fh-after.json
diff /tmp/fh-before.json /tmp/fh-after.json   # expect no diff (members/grocery unchanged)
```

Save evidence as `E-systemctl-*.txt`, `E-smoke-*.txt` per Phase E exit criteria.


## Backup

Daily SQLite backup (cron example):

```bash
0 3 * * * sqlite3 /var/lib/family-hub/family-hub.sqlite ".backup '/var/lib/family-hub/backups/family-hub-$(date +\%F).sqlite'"
```

## Recovery

- **Server down:** ESP32 panel shows cached data + offline badge; phones show error until server returns.
- **Corrupt DB:** restore from latest backup; restart `family-hub.service`.
- **ESP32 reset:** reflash firmware; set server host in NVS via Settings screen or re-provision `DEFAULT_SERVER_HOST`.

## Security

- Do not expose port 3020 to the public internet without TLS review.
- Optional: set `WRITE_TOKEN` in `.env` and configure clients as above.
- Keep `.env`, `secrets.h`, and SQLite backups out of git.
