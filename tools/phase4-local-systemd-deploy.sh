#!/usr/bin/env bash
# Phase 4 local systemd deploy (dev-pc / endeavor-style).
# Run manually from a real terminal (needs sudo). Preserves live SQLite.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
EVID="$REPO/docs/verification-evidence"
mkdir -p "$EVID"

echo "== Free :3020 =="
if ss -ltnp | grep -q ':3020'; then
  # Prefer stopping systemd if already installed
  if systemctl is-active --quiet family-hub 2>/dev/null; then
    sudo systemctl stop family-hub
  fi
  PIDS=$(ss -ltnp | grep ':3020' | grep -oP 'pid=\K[0-9]+' | sort -u || true)
  for p in $PIDS; do kill "$p" 2>/dev/null || true; done
  sleep 1
fi

echo "== Checkpoint DB =="
sqlite3 "$REPO/server/data/family-hub.sqlite" "PRAGMA wal_checkpoint(TRUNCATE);" || true

echo "== Install trees =="
sudo mkdir -p /opt/family-hub/server /opt/family-hub/web /opt/family-hub/deploy /opt/family-hub/tools /var/lib/family-hub
sudo cp -a "$REPO/server/app.js" "$REPO/server/package.json" "$REPO/server/package-lock.json" /opt/family-hub/server/
sudo cp -a "$REPO/server/config" "$REPO/server/db" "$REPO/server/middleware" \
  "$REPO/server/routes" "$REPO/server/services" "$REPO/server/utils" /opt/family-hub/server/
sudo cp -a "$REPO/web/." /opt/family-hub/web/
sudo cp -a "$REPO/deploy/." /opt/family-hub/deploy/
sudo cp -a "$REPO/tools/." /opt/family-hub/tools/
sudo cp -a "$REPO/server/data/family-hub.sqlite" /var/lib/family-hub/family-hub.sqlite

echo "== npm install =="
cd /opt/family-hub/server
sudo npm install --omit=dev

echo "== systemd =="
sudo useradd -r -s /usr/sbin/nologin familyhub 2>/dev/null || true
sudo chown -R familyhub:familyhub /opt/family-hub /var/lib/family-hub
sudo cp /opt/family-hub/deploy/family-hub.service /etc/systemd/system/family-hub.service
sudo systemctl daemon-reload
sudo systemctl enable --now family-hub
sleep 2

systemctl is-active family-hub | tee "$EVID/E-systemctl-20260726.txt"
systemctl status family-hub --no-pager -l | tee "$EVID/E-systemctl-status-20260726.txt"

BASE_URL="${BASE_URL:-http://127.0.0.1:3020}"
bash /opt/family-hub/tools/smoke-test.sh | tee "$EVID/E-smoke-lan-20260726.txt"

echo "== S8 restart =="
python3 - <<'PY'
import json, urllib.request
before = json.load(urllib.request.urlopen("http://127.0.0.1:3020/api/members"))
items = before.get("items", before if isinstance(before, list) else [])
open("/tmp/fh-members-before.json", "w").write(
    json.dumps(sorted(i.get("id") for i in items if isinstance(i, dict)))
)
print("members_before", len(items))
PY
sudo systemctl restart family-hub
sleep 2
python3 - <<PY
import json, urllib.request
after = json.load(urllib.request.urlopen("http://127.0.0.1:3020/api/members"))
items = after.get("items", after if isinstance(after, list) else [])
ids = sorted(i.get("id") for i in items if isinstance(i, dict))
before = json.load(open("/tmp/fh-members-before.json"))
ok = ids == before
print("members_after", len(items))
print("S8", "PASS" if ok else "FAIL")
open("$EVID/E-s8-restart-20260726.txt", "w").write(
    f"members_before={len(before)}\nmembers_after={len(ids)}\nids_match={ok}\n"
)
PY
echo "S9=$(systemctl is-active family-hub)" | tee -a "$EVID/E-systemctl-20260726.txt"
echo "Done. Panel host remains 192.168.1.134:3020 (unit binds 0.0.0.0)."
