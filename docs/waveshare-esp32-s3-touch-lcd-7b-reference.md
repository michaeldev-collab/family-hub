# Waveshare ESP32-S3-Touch-LCD-7B — Family Hub panel reference

**Official wiki:** https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7B
**Official BSP (source of truth for all values here):** https://github.com/waveshareteam/ESP32-S3-Touch-LCD-7B
**Schematic:** https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-7B/ESP32-S3-Touch-LCD-7B-Schematic.pdf
**Product:** ESP32-S3-Touch-LCD-7B (Type B) — *not* the original 800×480 -7.

> ⚠️ `waveshare-esp32-s3-touch-lcd-7-reference.md` describes the **original -7
> (800×480, EK9716, CH422G)** and does **not** apply to this board.

---

## Correction (2026-07-10): there is no ST7701 on this board

Earlier revisions of this doc (and the wiki's own FAQ) said the 7B panel uses
an ST7701 needing a 3-wire-SPI vendor init. **That is wrong**, verified three
ways against official sources:

1. The **schematic**'s 52-pin LCD connector (J1) carries only TTL-RGB signals
   plus strap pins (MODE, L/R, U/D, DITHB) — there are no SPI lines at all.
2. The official **BSP contains no ST7701 driver** and never sends an init
   sequence: `rgb_lcd_port.c` calls `esp_lcd_new_rgb_panel()` +
   `esp_lcd_panel_init()` only.
3. The wiki's resources list the **ST7262 datasheet** (an init-free RGB
   driver, same class as the -7's EK9716).

Consequence: the panel lights up from power + reset + a correct RGB clock.
**No vendor init exists or is needed.** The firmware's former ST7701 bit-bang
path has been deleted.

---

## Verified hardware map (wiki pinout table + schematic)

| Signal | Pin |
|--------|-----|
| DE / VSYNC / HSYNC / PCLK | GPIO 5 / 3 / 46 / 7 |
| R3..R7 (red, 5 bits) | GPIO 1, 2, 42, 41, 40 |
| G2..G7 (green, 6 bits) | GPIO 39, 0, 45, 48, 47, 21 |
| B3..B7 (blue, 5 bits) | GPIO 14, 38, 18, 17, 10 |
| I2C SDA / SCL (shared bus) | GPIO 8 / 9 |
| Touch interrupt (TP_IRQ) | GPIO 4 |
| USB D- / D+ | GPIO 19 / 20 |

**RGB timings (BSP `rgb_lcd_port.cpp`, verbatim):** PCLK 30 MHz active-low ·
HSYNC pulse 162, back porch 152, front porch 48 · VSYNC pulse 45, back porch
13, front porch 3 · 2 framebuffers in PSRAM · 10-line bounce buffer.

**I2C devices (wiki address map):**

| Addr | Device |
|------|--------|
| 0x24 | CH32V003 IO extension (backlight, resets, SD CS, battery ADC) |
| 0x5D | GT911 touch (0x14 if INT floats high during reset) |

**CH32V003 register protocol (BSP `io_extension.cpp`):** reg 0x02 = pin-mode
bitmask (1=output) · 0x03 = output bitmask (all 8 pins at once — cache and
read-modify-write) · 0x04 = input · 0x05 = backlight PWM 0–255 (**clamp duty
≤ 97%** — 100% turns the AP3032 boost enable off) · 0x06 = battery ADC (16-bit).

**EXIO pin assignments (schematic):** EXIO1 = touch reset · EXIO2 = DISP
(backlight enable) · EXIO3 = LCD reset · EXIO4 = SD CS · EXIO5 = USB/CAN
select · EXIO6 = LCD_VDD_EN (panel VCOM/VDD rail).
LCD reset and touch reset are **only** reachable through the CH32V003 — if
0x24 does not ACK, the panel cannot be brought up.

**GT911 (BSP `gt911.cpp/h`):** reset = drive GPIO4 (INT) low, pulse EXIO1 low
100 ms, release, wait 200 ms → address latches to 0x5D. Orientation: native —
swap_xy=0, mirror_x=0, mirror_y=0, range 0–1023 / 0–599. Status reg 0x814E
(write 0 to clear), point 1 at 0x8150 (x lo/hi, y lo/hi), product-ID reg
0x8140 reads `"911"`.

---

## Build / flash / monitor (PlatformIO)

```bash
cd firmware
pio run -e waveshare7b                          # compile (full reflash image)
pio run -e waveshare7b -t upload                # flash
pio device monitor -b 115200                    # serial log
```

### M5 Launcher app (no reflash)

If the panel already runs **M5 Launcher**, build the installable app images:

```bash
cd firmware
pio run -e waveshare7b-household-launcher
pio run -e waveshare7b-child-launcher
```

Output:

- `firmware/dist/family-hub-household.bin` — dashboard + chore completion
- `firmware/dist/family-hub-child.bin` — child rewards/behavior UI

Install from Launcher: copy each `.bin` to SD (FAT32, MBR) → Launcher menu →
pick the file → **Install**. Do **not** use `pio upload` for this path.

> **Note:** The legacy single dual-mode image (`family-hub-waveshare7b-launcher.bin`) was retired in firmware-split Step 7. Use the two split binaries above.

If upload cannot open the port (fresh board or crashed firmware): **hold BOOT,
press and release RESET, release BOOT**, then run the upload again and press
RESET after it finishes. On a working firmware the auto-reset circuit usually
makes this unnecessary. Use the **USB Type-C port** (native USB) or the
**UART Type-C port** (CH343) — both can flash; the UART port is the reliable
recovery path.

The `waveshare7b` env (see `firmware/platformio.ini`) sets: pioarduino
platform (Arduino core 3.x / IDF 5.x — required for `num_fbs` +
`bounce_buffer_size_px` in the esp_lcd RGB API), 16 MB flash, `qio_opi`
memory type, `partitions_16MB.csv`, LVGL 8.4 via `include/lv_conf.h`, and
`FIRST_BOOT_DIAG=1`.

All board constants live in **`firmware/include/panel_config.h`** — edit
there, not scattered through the code.

---

## First-boot diagnostic mode (`FIRST_BOOT_DIAG=1`, default on)

`displayBegin()` stages, each logged to serial:

1. Flash / PSRAM / heap report (build must show ~16384 KB flash, ~8192 KB PSRAM)
2. I2C scan — must list 0x24 and 0x5D (or 0x14)
3. CH32V003 init → LCD_VDD_EN on → LCD reset pulse → DISP on → backlight PWM
4. RGB panel init (30 MHz, BSP timings)
5. Solid **RED → GREEN → BLUE → WHITE → BLACK** test screens (1.5 s each)
6. GT911 reset + address probe (prints product ID)
7. 8-second touch echo — raw x/y printed on touch
8. Normal Family Hub LVGL UI starts

How to read failures:

| Symptom | Meaning |
|---------|---------|
| No serial at all | Wrong port / not booting — recovery flash via BOOT+RESET |
| PSRAM 0 KB | memory_type wrong — must be `qio_opi` |
| 0x24 missing from scan | CH32V003 not responding — no backlight/reset possible; bring-up aborts loudly |
| Backlight off but stages pass | DISP/PWM path — check `displaySetBacklight`, try 50% |
| Backlit but black through color bars | Panel power/reset or PCLK — LCD_VDD_EN / LCD_RST via 0x24 |
| Rolling / sheared color bars | Timing mismatch — porch values (should not happen; values are BSP-verbatim) |
| Colors in wrong order | Data-lane order swapped — compare `panel_config.h` pin list to wiki table |
| GT911 absent | Touch cable / reset sequence; check 0x14 fallback in scan |
| Touch mirrored/swapped | Adjust orientation in `lvgl_touch()` — BSP says native, verify with echo |
| Alloc FAIL after panel init | PSRAM exhausted — framebuffers take ~2.4 MB, buffers ~160 KB |

Set `-DFIRST_BOOT_DIAG=0` in `platformio.ini` once hardware bring-up passes.

---

## Integration checklist

1. `cp firmware/include/secrets.example.h firmware/include/secrets.h` — set WiFi + `DEFAULT_SERVER_HOST`.
2. `pio run -e waveshare7b` — must link.
3. Flash + monitor; walk the diagnostic stages above.
4. Confirm `/api/dashboard-state` fetch renders on **screen** (not serial-only) and one touch write works.
5. Photo evidence → `docs/verification-evidence/G-7b-home-*.jpg`.
6. Flip `FIRST_BOOT_DIAG` to 0, reflash, confirm normal boot time.

## Verification status

- **Source-verified** (official wiki/schematic/BSP): pins, timings, I2C
  addresses, CH32V003 protocol, GT911 reset/orientation, init-free panel.
- **Compile-verified**: see `docs/verification-evidence/` for the current
  `pio run -e waveshare7b` log.
- **Hardware-verified**: ❌ not yet — no unit flashed. Nothing above is
  hardware-verified until the first-boot checklist passes on a real board.
