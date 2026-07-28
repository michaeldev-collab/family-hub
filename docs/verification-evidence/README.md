# Hardware verification evidence (gitignored)

Add panel photos, serial logs, endeavor deploy output, and checklist sign-offs here.

## Index

| File | Phase | Date | Result |
|------|-------|------|--------|
| A1-npm-install-20260618.txt | Baseline | 2026-06-18 | PASS |
| A1-npm-test-20260618.txt | Baseline | 2026-06-18 | PASS (9/9) |
| A2-server-start-20260618.txt | Baseline | 2026-06-18 | PASS |
| A2-smoke-local-20260618.txt | Baseline | 2026-06-18 | PASS |
| A3-devkit-20260618.txt | Baseline | 2026-06-18 | WAIVED (W-Y07) |
| A4-waveshare7-20260618.txt | Baseline | 2026-06-18 | WAIVED (W-Y07) |
| E-ssh-probe-20260618.txt | Phase E | 2026-06-18 | FAIL — hostname unresolved |
| E-health-probe-20260618.txt | Phase E | 2026-06-18 | FAIL — hostname unresolved |
| E-waiver-W-Y04-20260618.txt | Phase E | 2026-06-18 | WAIVED (W-Y04) |
| F-prep-secrets-20260618.txt | Phase F/G | 2026-06-18 | PREP — HITL WiFi + pio |
| vacation-run-20260618.md | Execution log | 2026-06-18 | Day 1 complete |
| vacation-run-final.md | Vacation closure | 2026-06-18 | Hardware-deferred V1 summary |
| vacation-npm-test-20260618.txt | Baseline | 2026-06-18 | PASS (9/9) |
| vacation-smoke-lan-20260618.txt | Phase E | 2026-06-18 | FAIL — connection refused (re-run when endeavor up) |
| vacation-health-lan-20260618.txt | Phase E | 2026-06-18 | FAIL — no response |
| vacation-smoke-local-20260618.txt | Baseline | 2026-06-18 | FAIL — server not running locally |
| E-ssh-probe-vacation-final.txt | Phase E | 2026-06-18 | FAIL — publickey (stitch/michael/endeavor/root) |

See [v1-roadmap/v1-agent-runbook.md](../v1-roadmap/v1-agent-runbook.md) for naming conventions.

## V1 Run — 2026-06-18

**Agent:** vacation mode autonomous run  
**Workspace:** `/home/stitch/Desktop/Operating/pi-iot/family-hub`  
**Summary:** V1-without-panel closure — npm test green; panel deferred W-HW01; SSH deploy blocked; manual endeavor steps in `vacation-run-final.md`.

## V1 Run — 2026-06-19

| File | Phase | Result |
|------|-------|--------|
| A1-npm-test-20260619.txt | Baseline | PASS (9/9) |
| A2-smoke-local-20260619.txt | Baseline | PASS |
| A3-devkit-20260619.txt | Baseline | PASS |
| A4-waveshare7-20260619.txt | Baseline | PASS |
| A5-elecrow7-20260619.txt | Baseline | PASS |
| E-health-probe-20260619.txt | Phase E LAN | PASS |
| E-smoke-lan-20260619.txt | Phase E LAN | PASS |
| E-ssh-probe-20260619.txt | Phase E SSH | FAIL — W-Y04 publickey |

**Summary:** Full baseline green; LAN smoke via dev `npm start` on dev-pc @ 192.168.1.132; Phase E systemd deploy blocked (SSH + inactive unit).
