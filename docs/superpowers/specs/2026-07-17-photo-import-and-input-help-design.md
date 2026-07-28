# Web UI: Photo Import for Children & Rewards + Input Help Notes

Date: 2026-07-17 · Status: approved

## Goal

1. Let parents attach photos to child profiles and rewards directly from their
   forms in the Rewards & Behavior web UI (no more copy-pasting media IDs).
2. Show a short helper note under every input explaining what belongs in it.

## Scope

- Frontend only: `web/js/rewards-admin.js` (+ small CSS in
  `web/css/rewards-admin.css`). No server changes — upload
  (`POST /api/v1/admin/media`), thumbnails, panel-sized derivatives, and the
  `profileAssetId` / `mediaAssetId` fields already exist and are validated
  server-side (`ASSET_TYPES` includes `child_profile` and `reward`).
- Applied to the **child** and **reward** forms. The widget is generic;
  tasks/routines/rules/consequences keep their current media-ID text inputs
  for now (can adopt later).
- The standalone media library sections stay for housekeeping (archive).

## Design

### 1. `mediaField(label, name, currentId, assetType, help)` widget

Rendered inside `childForm()` (name `profileAssetId`, type `child_profile`)
and `rewardForm()` (name `mediaAssetId`, type `reward`). Markup:

- Hidden input `name=<name>` holding the asset id — picked up by the existing
  form serialization, so create/update submissions are unchanged.
- Thumbnail preview via existing `mediaThumb()` (★ placeholder when empty).
- `Upload photo` file input (`accept="image/png,image/jpeg,image/webp"`).
  On change: `ParentAPI.media.upload(assetType, file)` → on success set the
  hidden input + preview to the new asset id and refresh `state.data.media`.
- `Choose existing` — a `<details>` grid of active media of that `assetType`
  (thumbnails); clicking one sets the hidden input + preview.
- `Remove photo` button clears the hidden input (child/reward keeps no photo).

Event handling follows the existing delegation pattern in `rewards-admin.js`
(`data-ra-action` attributes handled in the root click/change listener).

### 2. Help notes under all inputs

- `field()` already renders `options.help` as `<small class="muted">` with
  `aria-describedby`. Add concrete, parent-facing `help:` text to every
  `field()` call in every form: login, children, tasks, task assignments,
  routines, approvals/decisions, rewards, reward access, terms, day status,
  rules, incidents, consequences, star corrections, display settings,
  child-focus (first–then) forms, media upload.
- Hand-rolled inputs that bypass `field()` (assignment rows, routine task
  rows) get matching `<small class="muted">` notes.
- Tone: one short sentence, concrete example where format matters
  (e.g. "Weekdays as numbers: 1,3,5 = Mon, Wed, Fri").

## Error handling

Upload failures (too large, wrong type, network) flow through the existing
`mutate()` → error banner path. Server enforces type/size limits; the widget
does not duplicate validation beyond the `accept` attribute.

## Testing / verification

- `npm test` in `server/` (includes web-admin and browser smoke tests).
- Manual browser pass against the temp server: log in, upload a photo for a
  child and a reward, confirm thumbnail + saved asset id, confirm the panel
  display payload references the asset, confirm help notes render under
  inputs in every section.
