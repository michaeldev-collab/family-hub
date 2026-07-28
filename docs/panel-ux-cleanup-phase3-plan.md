# Family Hub — Panel UX Phase 3 Plan (Contracts)

**Status:** Phase 3 Execute **COMPLETE** (2026-07-26) — CEO ★ authorized · builds PASS · serial C1/C4 PASS  
**Date:** 2026-07-26  
**Product lock:** [panel-ux-cleanup-addendum.md](panel-ux-cleanup-addendum.md) §§1,3,7  
**Handoff:** [panel-ux-cleanup-cto-handoff-phase3.md](panel-ux-cleanup-cto-handoff-phase3.md)  
**Reference:** Calendar `docs/api/panel-contracts.md` + `docs/adr/003-fixed-screen-contracts.md` (patterns only; no repo merge)  
**Out of scope:** systemd/`/opt`, WRITE_TOKEN force-on, rewards/child remount, full firmware tree restructure

---

## 1. Goal

Panel binds **typed view models** with `schema_version`, not ad-hoc JSON field probing. Invalid / unsupported schema → Diagnostics (fail-open), never a fake healthy Home.

**Done =**

1. Server emits versioned panel VMs (or versioned fields on a single dashboard payload)
2. Firmware validators reject bad/missing `schema_version`
3. Home / Grocery / Chores / Dinner / Notes UI bind VMs only
4. Chore complete mutation still works end-to-end
5. `docs/screens/` specs exist for household screens (1024×600 Waveshare; note 800×480 Elecrow later)
6. Builds: `waveshare7b`, `waveshare7b-household-launcher`, `devkit` SUCCESS
7. Aikido on modified first-party sources

---

## 2. Current state (Known)

| Layer | Today |
|-------|--------|
| API | `GET /api/dashboard-state` — flat bag: `dinner`, `weekDinner`, `chores`, `grocery`, `notes`, `pinnedNotes`, `members`, … |
| Validity | `dashboardStateValid()` = “any of those arrays/objects present” — **no `schema_version`** |
| UI | Household screens read fields directly from one `JsonDocument` |
| Diag | Overlay already shows `schema_version: n/a until Phase 3` |

Steal from calendar: fixed contracts + `schema_version` + thin panel mutations; keep FH LAN auth model.

---

## 3. Contract shape (proposed)

### Prefer single poll (household panel already polls one blob)

Keep one primary read for the panel:

`GET /api/dashboard-state` **or** alias `GET /api/v1/panel/dashboard`

Every successful body **must** include:

| Field | Type | Notes |
|-------|------|-------|
| `schema_version` | integer | Start at `1`; panel rejects ≠ supported set |
| `generated_at` | string ISO-8601 | Rename/alias today’s `generatedAt` for contract clarity (accept both in v1 validator if needed) |
| `server_version` | string | today’s `serverVersion` |
| `today` | `YYYY-MM-DD` | |
| `home` | object | Home VM |
| `grocery` | object | Grocery VM |
| `chores` | object | Chores VM |
| `dinner` | object | Dinner VM |
| `notes` | object | Notes VM |
| `connection` | object | sourceOfTruth + optional lastError |

**Compat path:** During Execute, server may emit **both** legacy top-level arrays **and** nested VMs for one release; firmware v1 validator requires `schema_version` + VMs. Drop legacy in a later polish pass (not Phase 3 Done=).

### Home VM (`home`)

| Field | Type |
|-------|------|
| `dinner_today` | string \| null (meal label) |
| `dinner_cook` | string \| null |
| `open_chores_count` | integer |
| `grocery_count` | integer |
| `pinned` | array of `{ text }` (max 5) |
| `badge` | `ONLINE` \| `STALE` \| `OFFLINE` (server hint; panel may override from RSSI/HTTP) |

### Grocery VM

`items`: `[{ id?, text, checked? }]` — panel shows open list; no add.

### Chores VM

`items`: `[{ id, title, assignee_name? }]` — complete via existing write path.

### Dinner VM

`today`: `{ date, meal, cook_name? } \| null`  
`week`: `[{ date, meal }]` short range

### Notes VM

`pinned`: `[{ text }]`  
`recent`: `[{ text }]` (optional for Notes screen)

### Mutations (unchanged policy)

Panel: chore complete only (existing endpoint). Web owns edits.

---

## 4. Work packages

### WP-A — Docs + schemas

1. `docs/api/panel-contracts.md` (FH) — table above
2. `docs/screens/home-1024x600.md` (+ grocery/chores/dinner/notes stubs or one-pagers)
3. JSON Schema or markdown field tables under `docs/api/schemas/` (keep light)

**Done =** contracts + at least Home screen spec committed.

### WP-B — Server VMs

1. Extend `server/services/dashboardState.js` to build nested VMs + `schema_version: 1`
2. Keep legacy keys during transition **or** update firmware in same PR (prefer same ship)
3. Tests: assert `schema_version` and VM shapes in `server/tests/`

**Done =** `GET /api/dashboard-state` returns `schema_version` + VMs; smoke 200.

### WP-C — Firmware validators

1. Replace/extend `dashboard_state.cpp` validity to require `schema_version` ∈ supported + required VM keys
2. Filter document for deserialize to VM fields only
3. On reject: log, do not commit fake Home; open/keep Diagnostics (wire already in `household/main.cpp`)

**Done =** bad schema / missing `home` → Diagnostics path; good payload → IDLE.

### WP-D — UI bind VMs

1. Household UI render paths read `home` / `grocery` / … VMs (not legacy-only)
2. Settings + connection badge unchanged except badge from VM/status
3. Chore complete still uses chore `id` from Chores VM

**Done =** panel screens render from VMs on hardware or serial status dump.

### WP-E — Verify + Aikido

1. `pio run` waveshare7b / launcher / devkit
2. Serial: boot with good schema; optional reject test (fixture)
3. Aikido scan modified sources

---

## 5. Hardware / serial checks (Phase 3)

| # | Check |
|---|-------|
| C1 | Good `schema_version` → Home renders; no forced Diagnostics |
| C2 | Tampered/missing schema (dev fixture or proxy) → Diagnostics |
| C3 | Chore complete still confirms on server |
| C4 | Sleep/gestures from Phase 2 still work (smoke) |

H7 leftover from Phase 2 (API-down boot) can be closed here if CEO authorizes a brief API stop during C2/C1 offline testing.

---

## 6. Explicit non-goals

- Per-screen HTTP endpoints (optional later; single blob is enough for FH)
- Elecrow 800×480 layout polish (document sizes; Waveshare primary)
- Joystick adapter beyond stub
- systemd Phase E deploy
- Auth / WRITE_TOKEN policy changes

---

## 7. CEO ★ gate

Phase 3 **planning** = this document.  
Phase 3 **Execute** = WP-A→E after explicit authorization.

### Paste-ready Execute prompt

```text
/3dl-matrix-cto Execute Family Hub Panel UX Phase 3 contracts per docs/panel-ux-cleanup-phase3-plan.md.

Authority: CEO ★ Phase 3 authorized.
Follow WP-A → WP-E. Cite Done= from that plan and docs/panel-ux-cleanup-addendum.md §7 Phase 3.

Constraints:
- Household-only; do NOT remount rewards/child
- Do NOT deploy systemd / /opt
- Prefer single versioned dashboard blob + nested VMs
- Keep chore-complete mutation working
- Aikido scan modified first-party sources before complete
```


---

## 8. Execute record (2026-07-26)

| Check | Result |
|-------|--------|
| WP-A contracts + screens | Done — `docs/api/panel-contracts.md`, `docs/api/schemas/dashboard-v1.md`, `docs/screens/*-1024x600.md` |
| WP-B server VMs | Done — `schema_version: 1` + nested `home`/`grocery`/`chores`/`dinner`/`notes`; web home adapted |
| WP-C firmware validators | Done — require schema + VM shapes; filter VM fields only |
| WP-D UI bind VMs | Done — Serial + LVGL + serial `c` use `*.items` / home VM |
| `npm test` | 25/25 PASS |
| `pio run` waveshare7b / launcher / devkit | SUCCESS |
| Flash + serial | Home from VMs; no forced Diagnostics; `z`/`d` smoke PASS (`docs/verification-evidence/phase3-boot-schema-20260726.txt`) |
| C2 reject fixture | Code-path (validator); live tamper deferred |
| C3 chore complete | Endpoint tests PASS; panel path uses `chores.items[].id` (0 open chores on panel) |
| Aikido | `issues: []` on modified core sources (opengrep notes empty/non-git tree; 0 findings) |
