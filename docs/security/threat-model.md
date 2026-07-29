# Family Hub — Threat Model (household product)

**Scope:** Live household product — grocery, chores, dinner, notes, Waveshare panel, optional Clerk + Cloudflare Tunnel.  
**Out of scope here:** Archived rewards/child-panel product ([reward-discipline threat model](../reward-discipline-threat-model.md) is historical only).  
**Audience:** Portfolio / design review. No home IPs, hostnames, or live credentials.

**Risk class:** Medium for a single-household LAN + optional tunnel install. Not a multi-tenant SaaS.

---

## 1. Assets

| Asset | Sensitivity | Notes |
| --- | --- | --- |
| Household lists & member names | Low–Med | Social / routine privacy |
| SQLite database + backups | Med | Canonical SoT on the home host |
| `PANEL_TOKEN` / `WRITE_TOKEN` | Med | Shared secrets; panel is not per-user |
| Clerk session / JWT material | Med–High | Browser humans; identity provider |
| Wi‑Fi credentials in panel NVS / `secrets.h` | Med | Physical + firmware flash access |
| Public tunnel hostname | Med | Internet-reachable control plane for humans |
| Panel physical presence | Low–Med | Wall unit can be tapped / stolen |

---

## 2. Trust boundaries

```text
[Internet] --HTTPS--> [Cloudflare Tunnel] --HTTP--> [Node/Express + SQLite]
                                                      ^
[Browser + Clerk] ------------------------------------+
                                                      |
[ESP32 panel] ---- LAN HTTP + x-family-hub-token -----+
```

| Boundary | Rule |
| --- | --- |
| Browser ↔ API | Clerk JWT (when enabled) or full `WRITE_TOKEN`; never treat UI hiding as auth |
| Panel ↔ API | LAN + scoped `PANEL_TOKEN` (or write token); mutations = chore complete + grocery toggle only |
| Tunnel ↔ Panel | Panel credentials must not work via Cloudflare headers (`REJECT_PANEL_VIA_TUNNEL`) |
| Firmware ↔ secrets | `secrets.h` / NVS stay out of git; no Clerk inside firmware |
| Family ops ↔ home automation | No locks/HVAC/cameras in this product ([safety-boundaries](../safety-boundaries.md)) |

---

## 3. Actors

| Actor | Intent | Capabilities assumed |
| --- | --- | --- |
| Household parent | Legitimate admin | Clerk parent role or LAN write token |
| Household kid (browser) | Limited use | Clerk kid role — no member CRUD / Setup abuse paths |
| Guest on LAN | Curious / mischievous | Can reach unbound HTTP if tokens empty |
| Remote internet attacker | Opportunistic | Sees only tunnel hostname + Clerk login |
| Physical visitor | Local access | Can touch panel; may unplug / reset device |
| Malicious firmware peer | Supply-chain / stolen flash | Reads NVS Wi‑Fi / tokens if present |

---

## 4. Attack scenarios (what “bad” looks like)

### A. Guest on the LAN mutates lists (LAN trust mode)

**Attack:** Another device on Wi‑Fi `POST`s grocery/chore changes with no token while `WRITE_TOKEN` / Clerk are off.  
**Impact:** Annoying data vandalism; low confidentiality impact.  
**Mitigations:** Optional `WRITE_TOKEN`; enable Clerk for human writes; treat empty-token LAN as an explicit household trust choice ([CISO gate notes](../v1-roadmap/07-ciso-security-gate.md)).

### B. Panel token replayed through the public tunnel

**Attack:** Attacker who learned `PANEL_TOKEN` calls `/api/chores/.../complete` or grocery toggle on the public hostname.  
**Impact:** Remote chore/grocery tampering without a Clerk login.  
**Mitigations:** `REJECT_PANEL_VIA_TUNNEL` rejects panel tokens when Cloudflare headers are present; panel firmware must use LAN IP only ([ADR-008](../adr/008-clerk-cloudflare-auth.md)).

### C. Token in query string leaks into logs

**Attack:** Client sends `?token=...`; request logger records `originalUrl`.  
**Impact:** Secret material in log files / crash dumps.  
**Mitigations:** Auth is **header-only** (`x-family-hub-token`); query tokens ignored; logger redacts token-like query params.

### D. Stolen Clerk session / weak allowlisting

**Attack:** Phishing or device theft yields a browser session; or any Clerk user in the app can join without allowlist.  
**Impact:** Full parent-capable household edits if role is parent.  
**Mitigations:** Clerk hosted auth; optional `CLERK_ALLOWED_USER_IDS`; parent vs kid role checks on sensitive routes; short session lifetime via Clerk defaults.

### E. Database file theft from the host

**Attack:** Physical or SSH access to the home server copies SQLite.  
**Impact:** History of household ops data.  
**Mitigations:** OS permissions; DB path outside git; no secrets in DB beyond app data; backups stay private. (Full disk encryption is an operator choice.)

### F. Firmware / NVS secret extraction

**Attack:** Attacker with the panel dumps flash or reads Diagnostics settings.  
**Impact:** Wi‑Fi password and panel token exposure → LAN pivot.  
**Mitigations:** `secrets.h` gitignored; prefer NVS overrides; physical control of the wall unit; rotate tokens if panel is lost.

### G. Confused deputy / over-broad panel API

**Attack:** Compromised panel firmware calls admin CRUD as if it were the browser.  
**Impact:** Full data rewrite from a thin client.  
**Mitigations:** Panel token cannot pass `optionalWriteAuth` for general writes; only `panelCompleteAuth` routes for complete/toggle; web owns editing ([ADR-006](../adr/006-web-owns-editing.md)).

### H. Clickjacking / XSS on the admin SPA

**Attack:** Embed or inject script into the static admin UI.  
**Impact:** Session abuse in the browser.  
**Mitigations:** Security headers (`X-Frame-Options`, CSP tuned for Clerk, `nosniff`, Permissions-Policy); same-origin static hosting; keep dependencies lean.

### I. Scope creep into physical safety systems

**Attack:** Future feature wires door locks or HVAC into the same API.  
**Impact:** Safety / property damage.  
**Mitigations:** Explicit deny-list in [safety-boundaries](../safety-boundaries.md); separate domain from MQTT/HA.

---

## 5. STRIDE snapshot

| Category | Examples in this system | Primary controls |
| --- | --- | --- |
| Spoofing | Fake panel token; forged Bearer | Shared secrets + Clerk verify; tunnel reject for panel |
| Tampering | LAN guest writes; SQLite edit | Tokens / Clerk; host access control |
| Repudiation | Who completed a chore | Server logs writes; Clerk user id when present |
| Info disclosure | Tokens in URLs/logs; DB copy | Header-only auth; redact logs; gitignore secrets/DB |
| DoS | Flood panel poll / API | Single-household scale; operator network controls |
| Elevation | Kid → parent actions; panel → admin CRUD | Role guards; separate panel auth surface |

---

## 6. Accepted risks (documented)

| Risk | Why accepted |
| --- | --- |
| LAN trust with empty tokens | Valid for a locked-down home LAN; harden with Clerk/`WRITE_TOKEN` when guests share Wi‑Fi |
| HTTP on LAN (no TLS to panel) | Local segment assumption; tunnel provides HTTPS for browsers |
| Shared `PANEL_TOKEN` | One wall device per household; not a multi-user IdP on-device |
| Clerk as IdP dependency | Trades self-hosted IdP ops for stronger browser auth than a homemade PIN |

---

## 7. Related docs

- [Security pass (sanitized)](./security-pass.md)
- [Clerk + Cloudflare auth](../auth-clerk-cloudflare.md)
- [ADR-008](../adr/008-clerk-cloudflare-auth.md)
- [Safety boundaries](../safety-boundaries.md)
