# Calendar Hub → Family Hub architecture borrow plan

**Status:** Phase **C6 COMPLETE** (2026-07-26) — web flash/api helpers · CEO ★ · borrow track C0–C4+C6 done; C5 remains gated  
**Reference product:** Bedroom Calendar Hub (private sibling project — patterns only; not in this repo)  
**Constraint:** Patterns only — **do not merge repos**. Household-only. Anti-list in [panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md) §2 still applies.

---

## 1. Intent

Reuse Calendar’s **API plane discipline** and **web thin-admin patterns** so Family Hub’s household stack stays as clean as the panel contracts from Phase 3 — without importing calendar domain (events, RRULE, month grid) or Cloudflare-as-default.

---

## 2. Facts vs assumptions

| Facts | Assumptions | Unknowns |
|-------|-------------|----------|
| Calendar has ADR 001–007, `/api/v1` admin vs panel planes, JSON Schema + Ajv tests, `state_version`/ETag | FH LAN trust + optional tokens remains preferred over calendar `ADMIN_TOKEN` public model | Whether device-hashed panel credentials are worth it on a single home LAN |
| FH already has `schema_version` VMs, screen specs, web-owns-editing, chore-complete only | SPA web can stay; no need to copy multi-page login unless remote admin is desired | Effort to extract `createApp()` without breaking tests |
| Addendum already lists steal/anti | Next highest ROI is ETag + schema tests, not web rewrite | |

---

## 3. Steal list (API + web)

| Pattern | Calendar reference | FH target |
|---------|-------------------|-----------|
| Formal ADR pack | `docs/adr/001`–`007` | FH `docs/adr/` mirrors for SoT, thin client, contracts, polling, web-owns-editing, stack defaults |
| JSON Schema + Ajv | `docs/api/schemas/*.schema.json`, `server/tests/panel-contracts.test.js` | Schema files for dashboard v1 + contract tests |
| `state_version` + ETag/304 | `services/stateVersion.js`, panel routes | Bump on household writes; conditional GET on dashboard-state |
| View-model modules | `services/viewModels/*` | Split builders out of `dashboardState.js` |
| Optional `/api/v1/panel/dashboard` alias | panel routes | Alias without breaking firmware path |
| `createApp()` factory | `server/app.js` | Testable app without listen-on-import |
| Security headers (+ light rate limit) | middleware | Add headers; rate limit only if writes leave pure LAN trust |
| Mutation rigor | occurrence complete + idempotency + 409 | Stronger expected-version / idempotency on chore complete (household-shaped) |
| Shared web `api.js` helpers | `web/js/api.js` flash/auth helpers | Optional polish inside SPA — not multi-page rewrite |

---

## 4. Anti-list (do not borrow)

- Calendar domain (RRULE, occurrences, month/day grids)
- Cloudflare tunnel / public ghost host as FH default
- Replacing LAN + optional `WRITE_TOKEN` with calendar public `ADMIN_TOKEN` as the only model
- Merging `calender-display` and `family-hub` repos
- Remounting rewards/child archives
- SoftAP / heavy device-provisioning SoftAP flows as Phase-0 defaults

---

## 5. Recommended phase order (future)

### Phase C0 — Docs ADRs — **COMPLETE 2026-07-26**

Promote addendum steal/anti into FH ADRs 001–003, 005–006 + stack defaults (007).  
**Done =** [docs/adr/](adr/README.md) linked from [architecture-book.md](architecture-book.md); ADR-004 omitted (occurrence domain). No code.

### Phase C1 — Contract tests — **COMPLETE 2026-07-26**

JSON Schema for dashboard v1; Ajv tests against live `GET /api/dashboard-state`.  
**Done =** [docs/api/schemas/dashboard-v1.schema.json](api/schemas/dashboard-v1.schema.json) + [server/tests/panel-contracts.test.js](../server/tests/panel-contracts.test.js); `npm test` fails if VM shape regresses (29/29).

### Phase C2 — `state_version` + ETag — **COMPLETE 2026-07-26**

Monotonic version on grocery/chores/dinner/notes/members writes; `ETag` / `If-None-Match` → 304; panel keeps poll but skips rebuild on 304.  
**Done =** `server/services/stateVersion.js` + dashboard route ETag; `server/tests/etag.test.js`; firmware `If-None-Match` / 304 keep-last (`household_api_client.cpp`, `main.cpp`). Serial: `[api] 304 not modified` when idle (after flash).

### Phase C3 — Server structure — **COMPLETE 2026-07-26**

`services/viewModels/*`, optional `createApp()`, security headers.  
**Done =** [viewModels/dashboard.js](../server/services/viewModels/dashboard.js); [createApp](../server/app.js); [securityHeaders](../server/middleware/securityHeaders.js); same external API; `npm test` passes. No write rate-limit (LAN trust).

### Phase C4 — Mutation rigor — **COMPLETE 2026-07-26**

Idempotency key + clearer 409 on `POST /api/chores/:id/complete`.  
**Done =** `idempotency_key` replay + `expected_state_version` → 409; firmware sends key and handles 409 with resync; tests cover retry-safe duplicate posts.

### Phase C5 — Auth plane upgrade (optional, gated)

Hashed panel credential + revoke UI — **only** if LAN trust is no longer enough. Keep web token story separate.

### Phase C6 — Web polish — **COMPLETE 2026-07-26**

Shared helpers / flash UX inside existing SPA. Do **not** force multi-page login.  
**Done =** `web/js/api.js` — `showFlash` / `hideFlash` / `esc` / `formatApiError`; clear 401 messaging + Setup tab attention; SPA topology unchanged.

---

## 6. Web app arch note

| | Calendar | Family Hub (keep) |
|--|----------|-------------------|
| Shape | Multi-page static HTML | SPA `index.html` + `app.js` / `views.js` |
| Auth UX | Bearer admin token + login page | LAN trust + optional write token in `localStorage` |
| Editing | Web CRUD only | Same product rule |
| Borrow | `api.js` patterns, flash, clear 401 handling | Not the page topology |

---

## 7. CEO ★ gate

| Phase | Status |
|-------|--------|
| C0 ADRs | **COMPLETE** — CEO ★ 2026-07-26 |
| C1 Schema + Ajv | **COMPLETE** — CEO ★ 2026-07-26 |
| C2 state_version/ETag | **COMPLETE** — CEO ★ 2026-07-26 |
| C3 Server structure | **COMPLETE** — CEO ★ 2026-07-26 |
| C4 Mutation rigor | **COMPLETE** — CEO ★ 2026-07-26 |
| C6 Web polish | **COMPLETE** — CEO ★ 2026-07-26 |
| C5 Auth plane | **GATED** — only if LAN trust is no longer enough |

### Borrow track status

C0–C4 + C6 done. Remaining optional: C5 (auth) if exposure requirements change.

### Paste-ready next Execute (C5 — only if gated need opens)

```text
/3dl-matrix-cto Execute Family Hub calendar-arch borrow Phase C5
(hashed panel credential + revoke UI — only if LAN trust insufficient)
per docs/calendar-arch-borrow-plan.md.
Authority: CEO ★.
```
