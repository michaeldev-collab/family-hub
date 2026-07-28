#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:3020}"
AUTH_HEADER=()
if [[ -n "${FAMILY_HUB_TOKEN:-}" ]]; then
  AUTH_HEADER=(-H "x-family-hub-token: ${FAMILY_HUB_TOKEN}")
fi

echo "== Family Hub smoke test =="
echo "Target: $BASE_URL"
if [[ ${#AUTH_HEADER[@]} -gt 0 ]]; then
  echo "Auth: x-family-hub-token set"
fi

curl -fsS "$BASE_URL/api/health" | grep -q '"ok":true'
echo "OK  /api/health"

curl -fsS "$BASE_URL/api/dashboard-state" | grep -q '"today"'
curl -fsS "$BASE_URL/api/dashboard-state" | grep -q '"schema_version"'
curl -fsS "$BASE_URL/api/dashboard-state" | grep -q '"home"'
echo "OK  /api/dashboard-state (today + schema_version + home VM)"

ITEM=$(curl -fsS -X POST "$BASE_URL/api/grocery" \
  -H 'Content-Type: application/json' \
  "${AUTH_HEADER[@]}" \
  -d '{"text":"Smoke test milk"}')
echo "$ITEM" | grep -q 'Smoke test milk'
ID=$(echo "$ITEM" | sed -n 's/.*"id":"\([^"]*\)".*/\1/p')
echo "OK  POST /api/grocery ($ID)"

curl -fsS -X POST "$BASE_URL/api/events" \
  -H 'Content-Type: application/json' \
  -H 'x-family-hub-source: smoke' \
  "${AUTH_HEADER[@]}" \
  -d "{\"eventId\":\"smoke-$(date +%s)\",\"type\":\"grocery.toggle\",\"id\":\"$ID\"}" \
  | grep -q '"ok":true'
echo "OK  POST /api/events (toggle)"

CHORE=$(curl -fsS -X POST "$BASE_URL/api/chores" \
  -H 'Content-Type: application/json' \
  "${AUTH_HEADER[@]}" \
  -d '{"title":"Smoke chore"}')
CHORE_ID=$(echo "$CHORE" | sed -n 's/.*"id":"\([^"]*\)".*/\1/p')
curl -fsS -X POST "$BASE_URL/api/events" \
  -H 'Content-Type: application/json' \
  "${AUTH_HEADER[@]}" \
  -d "{\"eventId\":\"smoke-chore-$(date +%s)\",\"type\":\"chore.complete\",\"id\":\"$CHORE_ID\"}" \
  | grep -q '"ok":true'
echo "OK  POST /api/events (chore.complete)"

TODAY=$(date +%Y-%m-%d)
curl -fsS -X POST "$BASE_URL/api/events" \
  -H 'Content-Type: application/json' \
  "${AUTH_HEADER[@]}" \
  -d "{\"eventId\":\"smoke-dinner-$(date +%s)\",\"type\":\"dinner.set\",\"date\":\"$TODAY\",\"meal\":\"Smoke tacos\"}" \
  | grep -q '"ok":true'
echo "OK  POST /api/events (dinner.set)"

DUP_ID="smoke-dup-test"
curl -fsS -X POST "$BASE_URL/api/events" \
  -H 'Content-Type: application/json' \
  "${AUTH_HEADER[@]}" \
  -d "{\"eventId\":\"$DUP_ID\",\"type\":\"grocery.add\",\"text\":\"Dup item\"}" > /dev/null
curl -fsS -X POST "$BASE_URL/api/events" \
  -H 'Content-Type: application/json' \
  "${AUTH_HEADER[@]}" \
  -d "{\"eventId\":\"$DUP_ID\",\"type\":\"grocery.add\",\"text\":\"Dup item\"}" \
  | grep -q '"deduplicated":true'
echo "OK  idempotent event dedupe"

curl -fsS -o /dev/null -w "%{http_code}" "$BASE_URL/" | grep -q 200
echo "OK  web index"

echo "All smoke checks passed."
