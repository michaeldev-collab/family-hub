# Safety Boundaries — Family Hub v0.1

## Allowed (low-risk)

- Display dinner schedule, grocery list, chores, notes
- Add grocery items
- Mark simple chores complete
- Set tonight's dinner (informational only)
- Read-only device status (future phase)
- **Reminders (informational only)** — schedule notify-at times and member phone mapping; delivery via n8n + Google Voice is outbound text/voice only. Reminders must **not** trigger device control, automation rules, or physical actuators.

## Out of scope / high-risk (requires explicit approval)

- Door locks, garage doors
- Ovens, heaters, HVAC control
- Security alarms and cameras
- Water valves, gas-connected systems
- Any physical control affecting safety, privacy, access, or property damage

## Architecture safety rules

- **Server + database** is the only source of truth for family ops data.
- **ESP32 panel** is a thin client — it must not own persistent state or automation rules.
- **Home automation** (MQTT, Home Assistant, ESPHome) is a separate domain from family ops.
- Writes must not appear successful unless the server confirms.
- No shell/command execution from the API.
- No public internet exposure in v0.1.

## Stop before

- Controlling high-risk devices
- Exposing API publicly
- Storing production secrets in firmware
- Modifying production HA/ESPHome/MQTT config without a separate plan
