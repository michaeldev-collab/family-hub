# Family Hub — Panel API Contracts (v1)

**Auth:** LAN trust (optional `WRITE_TOKEN` / `PANEL_TOKEN` later).  
**Primary read:** `GET /api/dashboard-state`  
**Content-Type:** `application/json`  
**Scope:** Household panel only. Rewards/child APIs stay archived.

Panel must not call web admin CRUD. Web owns editing. Panel mutations are restricted to chore completion and grocery-state toggles.

---

## Read — single versioned dashboard blob

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/api/dashboard-state` | Household panel view models + legacy flat keys (compat) |

Every successful body **must** include:

| Field | Type | Notes |
|-------|------|-------|
| `schema_version` | integer | `1` supported; panel rejects others |
| `state_version` | integer | Monotonic; also used as `ETag` (`"N"`). Conditional GET with `If-None-Match` → `304` |
| `generated_at` | string ISO-8601 | Also emitted as `generatedAt` for web compat |
| `server_version` | string | Also `serverVersion` |
| `today` | `YYYY-MM-DD` | |
| `home` | object | Home VM |
| `grocery` | object | Grocery VM |
| `chores` | object | Chores VM |
| `dinner` | object | Dinner VM |
| `notes` | object | Notes VM |
| `connection` | object | `{ sourceOfTruth: "server" }` |

**Legacy flat keys** (`dinner` meal object, `weekDinner`, top-level `chores`/`grocery`/`notes`/`pinnedNotes`/`members`) remain for the browser admin during the transition. Firmware validators require `schema_version` + nested VMs only.

Field tables: [schemas/dashboard-v1.md](schemas/dashboard-v1.md).  
Machine schema (Ajv): [schemas/dashboard-v1.schema.json](schemas/dashboard-v1.schema.json) — enforced by `server/tests/panel-contracts.test.js`.

---

## View models (v1)

### `home`

| Field | Type |
|-------|------|
| `dinner_today` | string \| null |
| `dinner_cook` | string \| null |
| `open_chores_count` | integer |
| `grocery_count` | integer |
| `pinned` | `[{ text }]` (max 5) |
| `badge` | `ONLINE` \| `STALE` \| `OFFLINE` (hint; panel may override) |

### `grocery`

| Field | Type |
|-------|------|
| `items` | `[{ id?, text, checked? }]` |

### `chores`

| Field | Type |
|-------|------|
| `items` | `[{ id, title, assignee_name? }]` |

### `dinner`

| Field | Type |
|-------|------|
| `today` | `{ date, meal, cook_name? }` \| null |
| `week` | `[{ date, meal }]` |

### `notes`

| Field | Type |
|-------|------|
| `pinned` | `[{ text }]` |
| `recent` | `[{ text }]` |

---

## Mutations (panel)

Panel mutations are restricted to chore completion and grocery-state toggles.

| Method | Path | Purpose |
|--------|------|---------|
| POST | `/api/chores/{id}/complete` | Mark chore complete |
| POST | `/api/grocery/{id}/toggle` | Toggle grocery checked state |

Optional JSON body:

| Field | Type | Notes |
|-------|------|-------|
| `idempotency_key` | string ≤128 | Retry-safe; duplicate posts return the cached 200 body |
| `expected_state_version` | integer | If set and not equal to current `state_version` → **409** `{ error: "version-conflict", state_version }` |

Already-complete without a key still returns **200** `{ ok, alreadyComplete: true, item }`.  
Do not show completed until API confirms. On failure: restore prior UI, allow retry (reuse the same `idempotency_key`). On 409: refresh dashboard, then retry with a new key.

---

## Polling

- Fetch dashboard on boot.
- Poll while active (~existing panel interval).
- Immediate refresh after successful chore complete.
- Invalid / unsupported `schema_version` → Diagnostics, not fake Home.

## Shell gestures (Waveshare)

| Action | Gesture |
|--------|---------|
| Sleep | Double-tap header chrome (`y < 70`) |
| Wake | Any single tap (screen is black) |
| Diagnostics | Hold header ≥2s |

---

## Out of scope for panel

Member CRUD, grocery add, dinner edit, notes compose, rewards, child focus, SSID provisioning (Settings host/token only until later).
