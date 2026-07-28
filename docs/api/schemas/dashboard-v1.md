# Dashboard schema v1 — field tables

`schema_version` = **1**

**Authoritative JSON Schema:** [dashboard-v1.schema.json](dashboard-v1.schema.json) (Ajv in `server/tests/panel-contracts.test.js`).

## Root

| Field | Required | Type |
|-------|----------|------|
| schema_version | yes | integer (=1) |
| state_version | yes | integer (≥0); ETag source |
| generated_at | yes | string |
| server_version | yes | string |
| today | yes | string `YYYY-MM-DD` |
| home | yes | object |
| grocery | yes | object |
| chores | yes | object |
| dinner | yes | object |
| notes | yes | object |
| connection | yes | object |

Empty `items` / `pinned` / `week` arrays are valid. `dinner.today` may be `null`.

## Reject

Missing `schema_version`, unsupported version, or missing any required VM object → panel must not commit as live Home state.
