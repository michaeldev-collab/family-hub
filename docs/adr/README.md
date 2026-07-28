# Family Hub ADRs

Architecture Decision Records for the **household** Family Hub product.

Borrowed as **patterns** from Bedroom Calendar Hub (`calender-display/docs/adr/`). Repos stay separate. See [calendar-arch-borrow-plan.md](../calendar-arch-borrow-plan.md).

| ADR | Title | Status |
|-----|-------|--------|
| [001](001-api-source-of-truth.md) | API is the source of truth | Accepted |
| [002](002-panel-thin-client.md) | Panel is a thin client | Accepted |
| [003](003-fixed-screen-contracts.md) | Fixed screen contracts | Accepted |
| [005](005-http-polling-first.md) | HTTP polling first | Accepted |
| [006](006-web-owns-editing.md) | Web application owns editing | Accepted |
| [007](007-stack-defaults.md) | Locked stack defaults | Accepted |
| [008](008-clerk-cloudflare-auth.md) | Clerk + Cloudflare Tunnel (browser) | Accepted |

**Intentionally omitted:** Calendar ADR-004 (occurrence-level completion) — household chores complete by entity id, not calendar occurrences.

**Anti-list** (do not reverse via ADR): rewards/child remount, merge with `calender-display`, replace LAN+optional tokens with public `ADMIN_TOKEN` as the only model. Cloudflare Tunnel + Clerk for **browser** remote access is allowed per [ADR-008](008-clerk-cloudflare-auth.md); the ESP32 panel remains LAN + `PANEL_TOKEN`. See [panel-ux-cleanup-addendum.md](../panel-ux-cleanup-addendum.md) §2 and [auth-clerk-cloudflare.md](../auth-clerk-cloudflare.md).
