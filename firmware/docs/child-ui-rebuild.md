# Child UI rebuild — ARCHIVED

> **ARCHIVED 2026-07-26.** Child panel is out of the live Family Hub tree.
> Restore path: `/run/media/stitch/data3/Operating/pi-iot/family-hub-child-panel-archive-20260726/`
> See also repo root `archive/CHILD_PANEL_MOVED.md` and `docs/panel-ux-cleanup-addendum.md`.

## Scope (historical)

Reworked the dedicated child firmware presentation layer in `src/child/child_ui.cpp` without changing API contracts or reward/task action behavior.

## Main changes

- Removed LVGL symbol-glyph dependencies from the child UI. The panel font configuration was rendering many `LV_SYMBOL_*` values as empty square boxes.
- Replaced the four-item navigation bar with three large toddler-friendly destinations: Home, Tasks, and Rewards.
- Rebuilt the dashboard header with a clear profile area, readable star balance, child switch control, and parent PIN control.
- Rebuilt the home dashboard into three visually distinct regions:
  - Today goal and progress
  - Up to four current task cards
  - Large term reward and progress
- Removed placeholder task cards from the home dashboard when there are fewer than four actual tasks.
- Added task-status color strips and readable `OK` / waiting badges.
- Reduced border and shadow weight to improve contrast and reduce visual clutter.
- Preserved current child actions, API state shape, task completion behavior, reward navigation, parent PIN flow, and screen lifecycle.

## Build verification

A PlatformIO build was attempted with:

```bash
platformio run -e waveshare7b-child-launcher
```

The build could not start because the environment requires downloading the pioarduino ESP32 platform from GitHub and outbound DNS/network access was unavailable in the execution environment. The source passed basic structural checks for balanced braces and parentheses. A full firmware build must be run on the development machine where the project PlatformIO packages are already installed.

## Recommended hardware verification

1. Build `waveshare7b-child-launcher`.
2. Install through M5 Launcher.
3. Confirm that no square placeholder glyphs remain.
4. Test Home, Tasks, Rewards, child switch, and long-press PIN controls.
5. Repeat navigation for at least 10 minutes while monitoring heap and PSRAM logs.
6. Confirm all uploaded task and reward images are cropped correctly at 96 px and do not overlap card boundaries.
