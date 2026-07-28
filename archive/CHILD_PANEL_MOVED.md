# Child panel moved

On 2026-07-26 the child Waveshare panel firmware and panel API/web surfaces were
archived out of the live Family Hub product.

**Archive path:** `/run/media/stitch/data3/Operating/pi-iot/family-hub-child-panel-archive-20260726/`

See that archive's `README.md` for contents and restore notes.

Live tree no longer includes child firmware sources/bin or the child PlatformIO
env. Household `PANEL_TOKEN` chore complete remains. Unused SQLite panel tables
were left in place (no destructive migration).
