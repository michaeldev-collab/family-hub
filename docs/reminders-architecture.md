# Reminders Architecture (v1.1)

**Status:** UI + schema prep in v1.0.x; delivery (n8n + Google Voice) deferred to v1.1.

> **Alias note:** CEO direction referenced "NAN" — this feature uses **[n8n](https://n8n.io/)** (workflow automation). No separate "NAN" product.

## Source of truth

- **Server (SQLite on endeavor)** owns reminder schedules, member phone mappings, and delivery status.
- **n8n** is the **delivery layer only** — it receives outbound webhook payloads and forwards to Google Voice (or an SMS gateway).
- **Browser / ESP32** never schedule delivery directly; they CRUD reminders via REST.

This aligns with the [Architecture Book](architecture-book.md): server is canonical; panel and browser are clients.

## Outbound-only flow (v1.1)

```text
Family Hub server (cron/tick)
  → SELECT pending reminders WHERE remind_at <= now
  → POST N8N_REMINDER_WEBHOOK_URL (LAN-only)
  → n8n workflow
  → Google Voice / SMS gateway
  → family member phone(s)
```

No inbound SMS parsing in v1.1. Reminders are **informational notifications** only — see [safety-boundaries.md](safety-boundaries.md).

## Entity types

| entity_type | entity_id | Default message context |
|-------------|-----------|-------------------------|
| `grocery` | grocery_items.id | Item text |
| `chore` | chores.id | Chore title + assignee |
| `dinner` | dinner_plans.date (ISO date) | Meal + cook for that date |
| `note` | notes.id | Note text (truncated) |

One entity may have multiple reminders (e.g. grocery item + day-before nudge). UI v1 prep supports one active **pending** reminder per entity for simplicity; API allows multiples.

## Reminder fields (centralized `reminders` table)

| Field | Type | Notes |
|-------|------|-------|
| `id` | TEXT PK | UUID |
| `entity_type` | TEXT | `grocery` \| `chore` \| `dinner` \| `note` |
| `entity_id` | TEXT | FK by type (see above) |
| `remind_at` | TEXT | ISO 8601 datetime |
| `message` | TEXT | Optional override; server may default from entity |
| `notify_member_ids` | TEXT | JSON array of `family_members.id` |
| `channel` | TEXT | Default `google_voice` |
| `status` | TEXT | `pending` \| `sent` \| `cancelled` |
| `created_at` / `updated_at` | TEXT | SQLite datetime |

## Member notification setup

`family_members` extensions:

| Field | Type | Notes |
|-------|------|-------|
| `phone` | TEXT | E.164 preferred; freeform accepted in v1 prep |
| `notify_enabled` | INTEGER | 1 = include in delivery; 0 = skip |

Configured in browser **Setup → Notification numbers**. PATCH `/api/members/:id` with `phone`, `notifyEnabled`.

## Environment (server)

| Variable | Default | Purpose |
|----------|---------|---------|
| `N8N_REMINDER_WEBHOOK_URL` | (empty) | n8n webhook endpoint on LAN |
| `REMINDERS_ENABLED` | `false` | Gate for delivery tick (v1.1) |

## Safety

- **LAN-only webhook** — `N8N_REMINDER_WEBHOOK_URL` must resolve to a private/LAN address unless CISO explicitly approves public exposure.
- **No device control** — reminders cannot trigger locks, HVAC, or automation actuators.
- **No secrets in firmware** — phone numbers and webhook URLs live on the server only.
- Delivery worker must respect `notify_enabled` and skip members without `phone`.

## Deferred to v1.1

- n8n workflow JSON (lives in `docs/n8n/` or external repo when authored)
- Google Voice / SMS gateway integration inside n8n
- Server-side delivery scheduler (poll + webhook POST)
- Retry / dead-letter handling for failed sends
- Panel (ESP32) reminder UI — browser prep only in v1.0.x

## API (v1 prep)

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/api/reminders?entity_type=&entity_id=` | List (optional filter) |
| POST | `/api/reminders` | Create pending reminder |
| DELETE | `/api/reminders/:id` | Cancel (status → `cancelled`) |

## Related docs

- [architecture-book.md](architecture-book.md) — system roles
- [safety-boundaries.md](safety-boundaries.md) — informational-only scope
- [v1-roadmap/v1-decisions-register.md](v1-roadmap/v1-decisions-register.md) — D-39
