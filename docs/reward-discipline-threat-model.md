# Reward and Discipline Add-On — Threat Model

**Risk class:** R2 Medium for LAN-only household use.  
**Security owner:** CISO; implementation owner: CTO.  
**Re-review triggers:** public exposure, cloud/third-party child data, auth design
change, new device-control capability, or unresolved critical scanner finding.

## Sensitive assets

- Child identity, photos, attendance schedules, progress, and reward history
- Incidents, consequences, recovery, and parent notes
- Star ledger, approvals, audit records, and issuing-parent identity
- Original uploaded media and database/backups
- Parent credentials/session material and scoped panel credentials

## Actors and trust boundaries

| Actor | Allowed | Forbidden |
|---|---|---|
| Parent/admin | Authenticated configuration, approvals, history | Bypassing audit/invariants |
| Child/panel | Allowlisted display reads and request actions | Admin reads, approvals, notes |
| Other LAN client | Health/static login only as designed | Sensitive reads or mutations |
| Server | Canonical rules and persistence | Trusting client-calculated effects |
| Media processor | Validate and derive images | Executing or publicly serving originals |

## Required controls

### Parent authorization

- Server-validated parent session on all admin reads and mutations.
- If PIN-based: salted slow hash, rate limiting/backoff, short-lived session,
  explicit logout/expiry, and no reusable PIN in panel firmware.
- If cookies: `HttpOnly`, `SameSite=Strict`, secure transport handling, origin
  checking, and CSRF tokens on mutations.
- Authorization is enforced in middleware and domain services where ownership
  matters; hiding UI controls is not a control.

### Panel authorization and isolation

- Separate scoped credential/capability from parent authorization.
- Only enumerated task-completion, reward-selection/request, and allowed focus
  actions; no generic event-to-admin bridge.
- Server validates child, assignment, lifecycle, setting, and idempotency key.
- Child-specific endpoints cannot enumerate another child's private correction
  or administrative history.

### Display minimization

DTO allowlists contain only identity presentation, bounded progress/rewards/tasks,
one child-safe correction, allowed-action booleans, asset derivative references,
and sync metadata. Negative contract tests reject parent notes, audit fields,
original paths, contacts, internal IDs not required by actions, and sibling data.

### Upload and media controls

- Parent auth, request/body quotas, file-size and dimension caps.
- MIME plus magic-byte validation, full image decode/re-encode, EXIF removal.
- Random server filenames outside web root; originals require authorization.
- Only generated, non-executable derivatives are display-addressable.
- Decode failures, decompression bombs, traversal names, SVG/script payloads, and
  excessive catalog sizes have adversarial tests.

### Integrity and replay

- Unique idempotency keys scoped to action and actor/client.
- Database uniqueness constraints back application checks.
- Atomic lifecycle transition, ledger effect, and audit insertion.
- Append-only audit contains actor, source, action, target, timestamp, and safe
  metadata without copying secret/session values.

### Exposure and operations

- Bind/expose only on the intended LAN; no router forwarding.
- Logs avoid parent notes, credentials, and raw uploaded content.
- Database and media backups inherit restricted household access and documented
  retention. Archive semantics preserve necessary audit history.

## Abuse cases and verification

| Abuse case | Required evidence |
|---|---|
| Unauthenticated admin request | 401/403 tests for every admin route class |
| Panel calls admin or approval route | scoped-token negative tests |
| Cross-child correction/history read | isolation and enumeration tests |
| Duplicate approval/redemption/reversal | concurrent/replay integration tests |
| Parent note leaks to display | recursive forbidden-key response tests |
| Malicious/oversized image | upload adversarial tests and storage inspection |
| CSRF/session fixation/brute PIN | auth integration tests for chosen design |
| Offline replay | no queued financial/progress write; idempotent retry test |

## Security gate exit

- Auth/session design selected and documented.
- Endpoint authorization matrix complete.
- Threat controls implemented with negative tests.
- Aikido full scan is clear, or tool unavailability is explicitly recorded with
  an approved equivalent scanner and manual evidence. A scan does not replace
  this threat model or security review.
