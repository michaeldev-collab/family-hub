# CISO Security Gate — LAN Trust & Deploy Safety

**Gate ID:** CISO-01  
**Status:** GREEN (with documented YELLOW items)  
**Date:** 2026-06-18

---

## Security Posture (V1)

| Control | Setting |
|---------|---------|
| Network exposure | **LAN only** — `HOST=0.0.0.0` binds all interfaces but must not be port-forwarded |
| Transport | HTTP (no TLS in V1) |
| Write auth | **Optional** `WRITE_TOKEN` — default **empty** (LAN trust mode) |
| Admin PIN | **Removed** |
| Firmware secrets | `secrets.h` gitignored; WiFi creds local only |
| API surface | REST CRUD + idempotent events — no shell/exec |
| High-risk control | **None** in v1 |

---

## WRITE_TOKEN Policy (Final)

| Server `WRITE_TOKEN` | Client behavior | V1 default |
|---------------------|-----------------|------------|
| Empty | No header required | **Yes — use on endeavor** |
| Set | `x-family-hub-token` on all writes | Optional hardening post-v1 |

**Agent rule:** Do not enable WRITE_TOKEN on endeavor during vacation unless CISO YELLOW escalates to documented incident (e.g., compromised LAN). If enabled, must verify browser + panel + smoke with token in same session.

**Client wiring (already implemented):**

- Browser: `localStorage.familyHubWriteToken` / Settings UI
- Panel: NVS `write_tok` or `FAMILY_HUB_WRITE_TOKEN` build flag
- Smoke: `FAMILY_HUB_TOKEN=... bash tools/smoke-test.sh`

---

## Threat Model Summary

| Threat | Likelihood | Impact | Mitigation V1 |
|--------|------------|--------|---------------|
| LAN guest mutates grocery/chores | Low–Med | Low | Accept in trust mode; optional WRITE_TOKEN later |
| Port 3020 exposed to internet | Low if no NAT rule | Med | Document: no port forward; stop if detected |
| SQLite theft from endeavor | Low (physical/LAN) | Med | OS access controls; backups gitignored |
| WiFi creds in firmware flash | Med | Med | secrets.h gitignored; device physical access |
| Fake write success on panel | Low | Low | Server confirm required — implemented |
| MQTT/HA pivot attack surface | N/A | — | Out of scope |

---

## Deploy Safety Checklist (Phase E — run before `systemctl enable`)

Execute on endeavor; save output to `docs/verification-evidence/E-security-YYYYMMDD.txt`.

- [ ] `.env` not world-readable: `chmod 600 /opt/family-hub/server/.env`
- [ ] Service runs as non-root: `User=familyhub` in unit file
- [ ] DB path outside web root: `/var/lib/family-hub/`
- [ ] No secrets in `/opt/family-hub` tree committed from git
- [ ] `WRITE_TOKEN` documented in `.env` (empty OK)
- [ ] Firewall: confirm no WAN rule to 3020 (manual inspect — document result)
- [ ] Backup taken before first production write

---

## Firmware Security

- [ ] `secrets.h` not in git (`git check-ignore firmware/include/secrets.h` → ignored)
- [ ] No production WRITE_TOKEN baked into committed `platformio.ini`
- [ ] Panel remains read-mostly; writes are low-risk family ops only

---

## Stop Conditions (CISO)

**RED — halt and escalate to CEO brief escalation list:**

- Plan exposes API to public internet
- Hardcoded production secrets committed
- High-risk device control added

**YELLOW — continue with waiver log:**

- WRITE_TOKEN disabled on open LAN (accepted for V1)
- HTTP on LAN (accepted for V1)

---

## Exit Criteria (CISO Gate)

- [x] Threat model documented
- [x] WRITE_TOKEN policy matches code
- [x] Deploy checklist runnable

**Gate:** GREEN (YELLOW: LAN trust mode accepted for V1)
