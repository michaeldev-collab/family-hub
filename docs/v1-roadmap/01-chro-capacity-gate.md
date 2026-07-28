# CHRO Capacity Gate — Ownership & Agent Boundaries

**Gate ID:** CHRO-01  
**Status:** GREEN — approved for vacation execution  
**Date:** 2026-06-18

---

## Ownership Matrix

| Domain | Owner (human) | Execution (vacation) | Notes |
|--------|---------------|----------------------|-------|
| Product scope | Michael (CEO) | Pre-decided in `02-cpo-product-gate.md` | No scope changes |
| Architecture | Michael + agents | Frozen per Architecture Book | Changes require board RED |
| Server code | Agent (`generalPurpose`) | Autonomous | `server/`, `web/`, `tools/` |
| Firmware | Agent (`generalPurpose`) | Autonomous except flash | `firmware/` |
| endeavor deploy | Agent + HITL | SSH required | See HITL table |
| Verification | Agent (`validation-review`) | Autonomous | Checklists + evidence |
| Security sign-off | Agent (`security-review`, read-only) | Autonomous review | No prod auth changes without register |

---

## Agent Roles (3DL OS Mapping)

| Role | Subagent / skill | Responsibilities |
|------|------------------|------------------|
| **Build executor** | `generalPurpose` | Code, deploy scripts, firmware LVGL |
| **Security reviewer** | `security-review` (readonly) | Auth, secrets, LAN exposure — end of Phase E |
| **Validation reviewer** | `validation-review` | Tests, smoke, checklist mapping — end Phase E, F, I |
| **Compatibility** | `compatibility-scan-review` | Contract scan after firmware changes |
| **Code review** | `code-reviewer` (optional) | Scope creep check at phase boundaries |
| **Explore** | `explore` (readonly) | Pre-flight inventory only |

**Single-writer rule:** One agent owns `firmware/src/ui_manager.cpp` during Phase G (LVGL). No parallel edits.

---

## Autonomous vs HITL-Only

### Runs autonomously (no human required)

- `cd server && npm install && npm test`
- `bash tools/smoke-test.sh` (local or `BASE_URL=...`)
- `cd firmware && pio run -e devkit && pio run -e waveshare7`
- Git commits per CEO brief recommendation
- Doc updates in `docs/v1-roadmap/` and evidence README
- Creating `firmware/include/secrets.h` from example (local, gitignored)
- Browser manual tests from a machine on LAN
- Serial monitor / devkit testing without display hardware

### HITL-only (human in the loop)

| Action | Why | Fallback if unavailable |
|--------|-----|-------------------------|
| **SSH to endeavor** | Production deploy, systemd, DB on `/var/lib` | STOP Phase E; complete Phases F prep + G build locally; log blocker |
| **USB flash Waveshare panel** | Physical device programming | STOP Phase F/G hardware rows; devkit serial evidence only + WAIVED LVGL with waiver |
| **WiFi AP credentials** | Must match home network | Agent creates `secrets.h` template; human fills SSID/password once |
| **Purchasing hardware** | CFO zero-spend policy | STOP; document missing hardware |
| **Router/firewall changes** | Out of agent authority | Document LAN requirement; no changes |

---

## Capacity Assumptions

- **1–2 agents** may run parallel workstreams where `v1-master-plan.md` allows (server deploy vs firmware build).
- **No 24/7 on-call** — async execution; stop conditions are hard stops, not retries forever.
- **Evidence over speed** — partial progress with logs beats silent failure.

---

## Vacation Schedule (CHRO)

| Window | Focus | Parallel OK? |
|--------|-------|--------------|
| Days 1–2 | Phase E endeavor deploy + smoke | Server agent only on endeavor |
| Days 2–4 | Phase F hardware verification (devkit → panel) | Firmware agent |
| Days 4–7 | Phase G LVGL implementation + flash | Single firmware owner |
| Days 7–8 | Phase H gap fill (browser manual, restart test) | Any |
| Day 8+ | Phase I sign-off + board log update | Validation agent |

Adjust dates freely; order is **E → F → G → H → I** (G may start build work in parallel with E if no endeavor dependency for compile-only).

---

## Exit Criteria (CHRO Gate)

- [x] Every workstream has an agent role
- [x] HITL boundaries documented
- [x] Parallelization rules in `v1-master-plan.md`
- [x] Stop conditions do not require CEO except escalation list in CEO brief

**Gate:** GREEN
