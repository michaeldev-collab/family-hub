# Auth: Clerk + Cloudflare Tunnel

Single-household Family Hub: self-hosted on endeavor, optional public HTTPS via Cloudflare Tunnel, Clerk for browser humans, panel tokens on LAN only.

See [ADR-008](adr/008-clerk-cloudflare-auth.md).

## Architecture

| Client | How it reaches the server | Auth |
|--------|---------------------------|------|
| Phone / laptop browser | `https://family.example.com` (Tunnel) or LAN IP | Clerk Bearer JWT |
| ESP32 panel | `http://192.168.x.x:3020` only | `x-family-hub-token: PANEL_TOKEN` |
| Health check | either | Public |

## Cloudflare Tunnel

1. In Cloudflare Zero Trust → Networks → Tunnels, create a tunnel.
2. Route a public hostname to `http://127.0.0.1:3020` on endeavor.
3. Install `cloudflared` and use the unit in [`deploy/cloudflared-family-hub.service`](../deploy/cloudflared-family-hub.service) (adjust credentials path).
4. On the Family Hub server `.env`:

```ini
TRUST_PROXY=true
PUBLIC_APP_URL=https://family.example.com
REJECT_PANEL_VIA_TUNNEL=true
```

5. Restart `family-hub` after env changes.

**Do not** point the panel firmware at the public hostname.

## Clerk setup

1. Create a Clerk application.
2. Allowed origins / redirect URLs: your tunnel hostname and `http://127.0.0.1:3020` for local dev.
3. For each household user, set **public metadata**:

```json
{ "role": "parent" }
```

or `"kid"`. Missing role defaults to **parent** so the first admin is not locked out.

4. Server `.env`:

```ini
CLERK_PUBLISHABLE_KEY=pk_...
CLERK_SECRET_KEY=sk_...
CLERK_AUTHORIZED_PARTIES=https://family.example.com,http://127.0.0.1:3020
# Optional hard allowlist:
# CLERK_ALLOWED_USER_IDS=user_abc,user_def
```

5. Restart the server. The web header shows **Sign in**. Setup → **Clerk sign-in** links the signed-in user to a member.

When `CLERK_SECRET_KEY` is empty, Clerk is off and LAN / `WRITE_TOKEN` rules from v0.1 still apply.

## Roles

| Role | Can |
|------|-----|
| parent | Full grocery / chores / dinner / notes / Setup / member CRUD / Other list rename / Clerk link |
| kid | Sign in and use lists; **cannot** member CRUD, Other title rename, or Clerk link |

## Panel

Provision `PANEL_TOKEN` on the server and the same value in panel NVS / Settings. Use LAN IP only. Grocery toggle and chore complete accept panel token, write token, or Clerk JWT.

## Local test bypass

For automated tests only (`NODE_ENV=test` / non-production):

```ini
CLERK_SECRET_KEY=test-secret
CLERK_TEST_BYPASS=true
```

Send `Authorization: Bearer test.<base64url({"sub":"user_x","publicMetadata":{"role":"parent"}})>`.
