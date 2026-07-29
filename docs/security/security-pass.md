# Family Hub — Security pass (sanitized)

**Date:** 2026-07  
**Scope:** Public portfolio cut of the household product (server, web, panel auth paths, tunnel posture).  
**Method:** Architecture review + targeted code review of auth/logging/headers + regression tests.  
**Not included:** Live host names, IPs, secrets, production Clerk dashboards, or raw scanner dumps with environment fingerprints.

This note is a **sanitized** pass for portfolio readers. It describes what was checked, what was fixed, and what remains accepted.

---

## 1. Review goals

1. Confirm **human** and **panel** auth stay separated.  
2. Confirm the **public tunnel** cannot be used as a panel backdoor.  
3. Confirm secrets are not casually leaked via **URLs / logs / git**.  
4. Confirm the panel cannot perform **admin CRUD**.  
5. Confirm basic **browser hardening** headers exist when serving the SPA + API.

---

## 2. Findings summary

| ID | Severity | Finding | Status |
| --- | --- | --- | --- |
| SP-01 | Medium | Query-string `?token=` accepted alongside header; URLs logged on writes | **Fixed** — header-only; logger redacts token-like params |
| SP-02 | Medium | Panel credential usable on public ingress if operator pointed firmware at tunnel URL | **Mitigated** — reject panel token when Cloudflare headers present (default on) |
| SP-03 | Low | Portfolio docs exposed local absolute archive paths | **Fixed** — scrubbed; restore notes stay private |
| SP-04 | Low | Product-boundary docs mixed live household vs archived rewards/child design | **Fixed** — historical design moved under `archive/` |
| SP-05 | Info | LAN trust with empty tokens is powerful on a shared Wi‑Fi | **Accepted** — documented; Clerk / `WRITE_TOKEN` available |
| SP-06 | Info | Admin SPA CSP allows `'unsafe-inline'` (Clerk / simple static UI) | **Accepted** — monitored; tighten if SPA grows |
| SP-07 | Info | No TLS on LAN panel hops | **Accepted** — tunnel terminates TLS for browsers |

---

## 3. Controls verified (pass evidence)

| Control | Evidence (public tree) |
| --- | --- |
| Header-only device/write token | `server/middleware/auth.js` — `tokenFromRequest` reads `x-family-hub-token` only |
| Query token rejected | `server/tests/api.test.js` — write with `?token=` → 401; header succeeds |
| Log redaction helper | `server/middleware/requestLogger.js` + `server/tests/request-logger.test.js` |
| Panel vs browser write surfaces | Panel uses `panelCompleteAuth`; CRUD uses `optionalWriteAuth` (panel token insufficient) |
| Tunnel / panel split | `rejectPanelViaTunnel` + tests in `server/tests/clerk-auth.test.js` |
| Role separation | Kid blocked from member / Setup-sensitive paths when Clerk enabled |
| Security headers | `server/middleware/securityHeaders.js` + `server/tests/security-headers.test.js` |
| Secrets out of git | `.gitignore` for `.env`, `secrets.h`, SQLite under `server/data/` |
| Thin panel mutations | Contracts/ADRs limit panel to chore complete + grocery toggle |

---

## 4. Hardening already in the product story

- **Clerk** for browser humans on optional public HTTPS (Cloudflare Tunnel).  
- **No OAuth on the ESP32** — reduces embedded attack surface and cert/store complexity.  
- **Versioned panel view-models** — less “stringly” firmware parsing; invalid schema rejected.  
- **Safety boundaries** — explicit refusal to drive locks, HVAC, alarms, valves.  
- **Idempotent chore complete** — reduces duplicate-action abuse / retry storms from flaky Wi‑Fi.

---

## 5. Residual backlog (honest)

| Item | Notes |
| --- | --- |
| Rate limiting | Not first-class; rely on household scale + edge (Cloudflare) for public login |
| mTLS or per-device panel identity | Shared `PANEL_TOKEN` is enough for one wall unit; rotate on loss |
| Encrypted-at-rest SQLite | Operator / OS responsibility |
| Dependency / container scanning in CI | Nice-to-have for the public repo; local tests cover auth regressions today |
| CSP without `'unsafe-inline'` | Requires SPA packaging work |

---

## 6. How to re-run a minimal pass

```bash
cd server
npm test
```

Focus on: `api.test.js` (header-only token), `clerk-auth.test.js` (panel vs tunnel), `security-headers.test.js`, `request-logger.test.js`.

Manual checklist:

1. With Clerk on: unauthenticated write → 401.  
2. With `PANEL_TOKEN` set: LAN complete/toggle OK; same call with CF-Ray header → 403.  
3. Confirm `.env` / `secrets.h` / `*.sqlite` are untracked.  
4. Confirm panel firmware host is LAN, not the public app URL.

---

## 7. Related docs

- [Threat model](./threat-model.md)
- [Clerk + Cloudflare](../auth-clerk-cloudflare.md)
- [ADR-008](../adr/008-clerk-cloudflare-auth.md)
- [Safety boundaries](../safety-boundaries.md)
