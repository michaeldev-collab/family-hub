# V1 Agent Runbook — Vacation Execution

**Audience:** Autonomous Cursor agents  
**Authority:** `09-board-decision-log.md`  
**Do not ask Michael** except RED escalations in `00-ceo-handoff-brief.md`

---

## Before You Start

1. Read `00-ceo-handoff-brief.md` (5 min)
2. Read `v1-decisions-register.md` (defaults — no questions)
3. Confirm repo path: `/home/stitch/Desktop/Operating/pi-iot/family-hub`
4. Create evidence dir if missing: `mkdir -p docs/verification-evidence`

---

## Day 1 — Baseline + Phase E

### Step 1: Automated baseline (always first)

PlatformIO: from repo root, `export PATH="$PWD/.venv-pio/bin:$PATH"` (or install with `pacman -S platformio-core`).

```bash
cd /home/stitch/Desktop/Operating/pi-iot/family-hub/server
npm install
npm test 2>&1 | tee ../docs/verification-evidence/A1-npm-test-$(date +%Y%m%d).txt

cd /home/stitch/Desktop/Operating/pi-iot/family-hub
bash tools/smoke-test.sh 2>&1 | tee docs/verification-evidence/A2-smoke-local-$(date +%Y%m%d).txt

cd firmware
pio run -e devkit 2>&1 | tee ../docs/verification-evidence/A3-devkit-$(date +%Y%m%d).txt
pio run -e waveshare7 2>&1 | tee ../docs/verification-evidence/A4-waveshare7-$(date +%Y%m%d).txt
```

**Pass:** All exit 0 / SUCCESS  
**Fail:** Fix regressions before Phase E (scope: v0.1 repair only — no new features)

### Step 2: Attempt Phase E (endeavor deploy)

```bash
ssh endeavor 'echo ok' 2>&1 | tee docs/verification-evidence/E-ssh-probe-$(date +%Y%m%d).txt
```

| SSH result | Action |
|------------|--------|
| Success | Execute `v1-phase-plan.md` Phase E steps E.1–E.10 |
| Failure | **STOP Phase E.** Log blocker. Proceed to Day 1 Step 3. |

Deploy commands: see Phase E in `v1-phase-plan.md` and `docs/endeavor-deploy.md`.

### Step 3: Parallel prep if SSH blocked or after E completes

```bash
cp firmware/include/secrets.example.h firmware/include/secrets.h
# Edit WIFI_SSID, WIFI_PASSWORD, DEFAULT_SERVER_HOST — use endeavor LAN IP or dev machine IP running npm start
```

**Stop Day 1 when:** Baseline green AND (E smoke pass OR E blocker documented).

---

## Day 2–3 — Phase F Hardware

1. Flash devkit: `cd firmware && pio run -e devkit -t upload`
2. Monitor: `pio device monitor -b 115200`
3. Verify serial checklist: WiFi, fetch, write (`a` key), WiFi drop
4. Save full serial log → `docs/verification-evidence/F-boot-YYYYMMDD.txt`
5. Flash waveshare7 if hardware available

**Stop if:** WiFi never connects after secrets verified — document SSID issue (HITL: human must confirm password)

**Do not:** Start LVGL until F.5 server fetch passes

---

## Day 4–6 — Phase G LVGL

**Single agent owns** `firmware/src/ui_manager.cpp`.

1. Follow `06-cto-build-gate.md` LVGL spec
2. Implement `#ifdef WAVESHARE_7` branches — status badge first, then home, then writes
3. Build/upload loop:
   ```bash
   cd firmware && pio run -e waveshare7 -t upload && pio device monitor
   ```
4. Capture photo → `docs/verification-evidence/G-panel-home-YYYYMMDD.jpg`
5. Run compatibility gate if CLI available

**Stop if:** LVGL libs cannot install — log Y-03 waiver; mark V1 BLOCKED

---

## Day 7–8 — Phase H Gap Fill

Requires endeavor running for S8, B*, P* against production.

```bash
# Restart persistence
ssh endeavor 'sudo systemctl restart family-hub'
curl -s http://endeavor:3020/api/dashboard-state | jq '.members | length'

# Browser tests — manual; record in docs/verification-evidence/H-browser-YYYYMMDD.md
```

Fix only bugs blocking checklist — no feature adds.

---

## Day 9+ — Phase I Sign-Off

1. Re-run all automated commands (§ Day 1 Step 1)
2. Mark every row in `v1-verification-checklist.md`
3. Fill `09-board-decision-log.md` Phase I template
4. Update evidence README index
5. Optional commit + tag per CEO brief

---

## When to Stop (Hard Stops)

| Condition | Action |
|-----------|--------|
| RED item in board log | Stop all; document for CEO |
| Smoke fails 2 fix attempts | Stop Phase E/H; preserve logs |
| Scope creep requested | Reject; defer v1.1 |
| Michael question needed | Check `v1-decisions-register.md` first — if not listed, pick default aligned with CEO brief |

---

## When to Continue Despite Blocker

| Blocker | Continue with |
|---------|---------------|
| No SSH | F, G compile work; local `npm start` as server |
| No panel hardware | Devkit serial only — **cannot SHIP V1** |
| LVGL lib pain | Try waveshare-setup steps once; then STOP G |

---

## Evidence Index Template

Append to `docs/verification-evidence/README.md`:

```markdown
## V1 Run — YYYY-MM-DD

| File | Phase | Result |
|------|-------|--------|
| A1-npm-test-....txt | Baseline | PASS |
| ... | ... | ... |
```

---

## File Map for Agents

| Need | File |
|------|------|
| Scope | `02-cpo-product-gate.md` |
| Commands | `v1-phase-plan.md` |
| Defaults | `v1-decisions-register.md` |
| Waivers | `v1-risks-and-waivers.md` |
| Deploy | `docs/endeavor-deploy.md` |
| Panel setup | `docs/waveshare-setup.md` |
| Safety | `docs/safety-boundaries.md` |

---

## Subagent Invocation

| After phase | Subagent | readonly |
|-------------|----------|----------|
| E | `security-review` | yes |
| E, I | `validation-review` | yes |
| G | `compatibility-scan-review` | yes |
| Each phase end | `code-reviewer` | optional |

Prompt template: "Review Family Hub diff against docs/v1-roadmap/v1-phase-plan.md Phase X exit criteria. Return PASS/FAIL list."

---

*Follow the plan exactly. No silent plan changes. No `.cursor/plans/` edits.*

---

## Execution Log

| Date | Log | Summary |
|------|-----|---------|
| 2026-06-18 | [vacation-run-20260618.md](../verification-evidence/vacation-run-20260618.md) | Day 1: server baseline PASS; endeavor blocked (W-Y04); pio missing (W-Y07) |
| 2026-06-19 | [vacation-run-final.md](../verification-evidence/vacation-run-final.md) § 2026-06-19 | Re-run: baseline+LAN smoke PASS; firmware 3/3; SSH/systemd deploy blocked |
