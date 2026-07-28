# V1 Risks and Waivers — Vacation Mode

**Usage:** Reference waiver ID when marking checklist WAIVED. Pre-approved mitigations may proceed without CEO contact.

---

## Pre-Approved YELLOW Waivers

| Waiver ID | Risk | Likelihood | Impact | Pre-approved mitigation | Agent action |
|-----------|------|------------|--------|-------------------------|--------------|
| W-HW01 | Waveshare ESP32-S3-Touch-LCD-7 not received | Certain | High | **V1.0 ships without panel**; Phase F–G DEFERRED-HARDWARE | Mark P1–P16 WAIVED; prep [waveshare-esp32-s3-touch-lcd-7-reference.md](../waveshare-esp32-s3-touch-lcd-7-reference.md); resume Phase F when hardware arrives |
| W-Y01 | Unauthenticated LAN writes | Med | Low | Accept LAN trust mode V1 | Keep WRITE_TOKEN empty; document in deploy |
| W-Y02 | HTTP cleartext on LAN | Med | Low | No port forwarding | Confirm no NAT rule; log in E-security evidence |
| W-Y03 | LVGL/Waveshare integration fails | Med | High | Stop Phase G; devkit serial fallback | V1-full = **BLOCKED** until fixed; V1.0 unaffected |
| W-Y04 | SSH to endeavor unavailable | Med | High | Local server for F/G; defer E | Document blocker; complete all non-E work |
| W-Y05 | WiFi credentials wrong | Med | Med | Human must fix secrets.h once | STOP F until serial shows connected |
| W-Y06 | Panel hardware unavailable | **Certain** (2026-06-18) | High | **W-HW01** — V1.0 without panel | V1.0 shippable; V1-full blocked until Phase G |
| W-Y07 | npm/pio not installed on agent machine | Low | Med | Install deps once | `npm install`, PlatformIO core |
| W-Y08 | endeavor hostname not resolving | Med | Low | Use LAN IP in URLs/smoke | `BASE_URL=http://<IP>:3020` |

---

## NOT Waivable (RED — Stop)

| ID | Risk | Agent action |
|----|------|--------------|
| W-R01 | Public internet exposure | STOP — escalate CEO |
| W-R02 | Secrets committed to git | STOP — revert before continue |
| W-R03 | High-risk device control added | STOP — revert |
| W-R04 | Production DB wiped without backup | STOP — restore backup |
| W-R05 | Spend money | STOP — escalate CEO |

---

## Known Technical Risks

| Risk | Phase | Mitigation |
|------|-------|------------|
| Waveshare Arduino package version drift | G | Pin versions in platformio.ini; follow waveshare-setup |
| SQLite lock on restart | E/H | Single systemd instance; `Restart=on-failure` |
| Idempotency table growth | B (done) | TTL prune implemented |
| PORT collision in dev | A (done) | PORT=0 for tests |
| Partial v0.1 checklist false positives | I | Use v1-verification-checklist, re-run manual |

---

## Vacation-Specific Risks

| Risk | Mitigation |
|------|------------|
| No CEO for scope decisions | `v1-decisions-register.md` |
| Agent scope creep | CPO gate + code-reviewer |
| Silent checklist skip | Require evidence file per phase |
| Parallel ui_manager.cpp edits | CHRO single-writer rule |

---

## Waiver Request Template (Agent)

If an unlisted waiver seems needed:

```markdown
## Waiver Request — WX-99 (UNLISTED)

- Checklist item:
- Risk:
- Why pre-approved register insufficient:
- Proposed mitigation:
- Recommendation: BLOCKED until CEO return
```

**Default for unlisted:** BLOCKED — do not invent waivers.

---

## Risk Acceptance at V1 Ship

Shipping **V1.0** (no panel) accepts W-Y01, W-Y02, and **W-HW01** by default.  
W-Y03 prevents **V1-full** SHIPPED status — does not block V1.0 server release.  
W-Y04 may defer full systemd deploy; manual steps in vacation evidence.

---

## Rollback Triggers

| Symptom | Rollback |
|---------|----------|
| Smoke fail post-deploy | COO gate rollback E |
| Panel boot loop after G | Reflash devkit firmware / prior binary |
| DB corruption | Restore `~/family-hub.sqlite.bak` |

---

*Pre-approved by board pack 2026-06-18 for vacation execution.*
