# ADR-008 — Clerk + Cloudflare Tunnel for browser auth

**Status:** Accepted  
**Date:** 2026-07-27  
**Supersedes (partially):** [ADR-007](007-stack-defaults.md) exposure + auth rows for **browser remote access**

## Context

Family Hub must stay self-hosted on the LAN (endeavor + SQLite + ESP32 panel) while allowing parents to sign in from phones away from home. ADR-007 locked LAN-only and optional `WRITE_TOKEN` / `PANEL_TOKEN`. Public exposure without real identity is unsafe.

## Decision

| Concern | Choice |
|---------|--------|
| Tenancy | **Single household** per install. No multi-family `tenant_id`. Roles = `parent` / `kid`. |
| Browser auth | **Clerk** session → Bearer JWT on API writes (and `/api/auth/*`) |
| Panel auth | Unchanged: LAN + `PANEL_TOKEN` / `WRITE_TOKEN`; **not** Clerk |
| Exposure | Optional **Cloudflare Tunnel** (`cloudflared`) to a public HTTPS hostname |
| Trust proxy | `TRUST_PROXY=true` when behind Tunnel |
| Panel via tunnel | Reject `PANEL_TOKEN` when Cloudflare headers present (`REJECT_PANEL_VIA_TUNNEL`) |

LAN-only installs keep working: leave `CLERK_SECRET_KEY` empty → legacy LAN trust / `WRITE_TOKEN` behavior.

## Consequences

- Operator guide: [auth-clerk-cloudflare.md](../auth-clerk-cloudflare.md)
- New env keys in `server/.env.example`
- Setup UI links Clerk `userId` → `family_members.clerk_user_id`
- Kid role cannot mutate members or rename the grocery Other list title

## Anti-goals

- Clerk Organizations / multi-household SaaS on one SQLite DB
- Clerk auth inside ESP32 firmware
