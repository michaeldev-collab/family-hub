> ⚠️ **Superseded for the 7B.** This doc covers the original ESP32-S3-Touch-LCD-7 (800×480, EK9716, CH422G). For the **7B (1024×600, ST7701, CH32V003)** see [waveshare-esp32-s3-touch-lcd-7b-reference.md](waveshare-esp32-s3-touch-lcd-7b-reference.md).

# Waveshare ESP32-S3-Touch-LCD-7 — Phase G Reference

**Official docs:** https://docs.waveshare.com/ESP32-S3-Touch-LCD-7  
**Product:** Waveshare ESP32-S3-Touch-LCD-7 (SKU 27078; non-touch variant SKU 30406 = ESP32-S3-LCD-7)

Concise prep for Family Hub Phase F/G when hardware arrives. Do not flash until `secrets.h` WiFi + endeavor host are set.

---

## Board at a glance

| Item | Value |
|------|--------|
| MCU | ESP32-S3 (Xtensa LX7 dual-core, up to 240 MHz) |
| Wireless | 2.4 GHz Wi‑Fi (802.11 b/g/n), Bluetooth 5 LE |
| Flash / PSRAM | 8 MB Flash + 8 MB PSRAM (some units 16 MB Flash + 8 MB PSRAM) |
| Display | 7" IPS, **800×480**, RGB interface, 65K colors |
| Touch | Capacitive, 5-point, I²C (GT911 class), optional on some SKUs |
| Power | USB Type-C 5 V (~450 mA) |
| Boot | Hold **BOOT**, press **RESET** to enter USB flash mode |

---

## Arduino IDE setup

1. Install Arduino IDE + Espressif ESP32 board support.
2. **Board package:** ESP32 Arduino **≥ 3.0.6**.
3. **Board name:** `Waveshare ESP32-S3-Touch-LCD-7` (install online or offline per Waveshare package).
4. Download Waveshare **example program package** from the docs page (contains `Arduino/libraries/`).

### Required libraries (from example package)

| Library | Min version | Notes |
|---------|-------------|--------|
| `ESP32_Display_Panel` | v0.1.4+ | RGB panel + bus init |
| `ESP32_IO_Expander` | v0.0.4+ | CH422G expander (backlight, SD CS, touch reset) |
| `lvgl` | **v8.4.0** | Offline install; copy `lv_conf.h` from examples |
| `lv_conf.h` | — | Manual install alongside LVGL |

Copy all folders from the package `Arduino/libraries/` into your Arduino libraries path (or PlatformIO `lib/`).

**Key example:** Section 08 — LVGL porting (`09_lvgl_Porting` in 4.3 docs; Section 08 on 7" docs). Start from Waveshare LVGL port, then wire Family Hub `ui_manager.cpp` hooks.

---

## PlatformIO (Family Hub `waveshare7` env)

When panel arrives, extend `firmware/platformio.ini` `[env:waveshare7]`:

```ini
board = esp32-s3-devkitc-1
board_build.arduino.memory_type = qio_opi
board_build.f_flash = 80000000L
board_build.flash_mode = qio
board_upload.flash_size = 8MB
build_flags =
    -DCORE_DEBUG_LEVEL=1
    -DWAVESHARE_7=1
    -DBOARD_HAS_PSRAM
    -DLV_CONF_INCLUDE_SIMPLE
    -DFIRMWARE_VERSION=\"1.0.0\"
lib_deps =
    bblanchon/ArduinoJson@^7.2.1
    lvgl/lvgl@8.4.0
    ; Add ESP32_Display_Panel + ESP32_IO_Expander from Waveshare zip (lib/ or git forks with CH422G)
```

Pin map for RGB/touch is defined in Waveshare `ESP_Panel_Board_Custom.h` — do not hand-roll unless matching their example.

---

## Flash notes

| Topic | Guidance |
|-------|----------|
| USB port | **USB** Type-C (GPIO19/20) for power + firmware upload |
| UART monitor | Separate **USB TO UART** port @ **115200** baud |
| Flash mode | Hold **BOOT**, tap **RESET**, release BOOT; then `pio run -e waveshare7 -t upload` |
| Partition | 8 MB flash; use `default.csv` or Waveshare-recommended layout |
| PSRAM | Required for LVGL framebuffer — enable `BOARD_HAS_PSRAM`, `qio_opi` memory type |
| CAN vs USB | CH422G `EXIO5`: low = USB mode, high = CAN — leave USB for development |

---

## Touch / display interfaces (from official pinout)

- **LCD:** RGB565 on GPIO0–48 (see docs LCD table); backlight via CH422G `EXIO2` (`DISP`).
- **Touch I²C:** SDA GPIO8, SCL GPIO9; IRQ GPIO4; reset via CH422G `EXIO1`.
- **I²C bus:** Shared with expander and touch — level select jumper 3.3 V typical.

---

## Family Hub integration checklist (Phase G)

1. `cp firmware/include/secrets.example.h firmware/include/secrets.h` — WiFi + `DEFAULT_SERVER_HOST` = endeavor LAN IP.
2. Install Waveshare libs; uncomment/add LVGL deps in `platformio.ini`.
3. `pio run -e waveshare7` — must link before flash.
4. Implement `#ifdef WAVESHARE_7` branches in `ui_manager.cpp` (status badge, home, touch writes).
5. Flash, verify `/api/dashboard-state` fetch + one write event on **screen** (not serial-only).
6. Capture photo evidence for `docs/verification-evidence/G-panel-home-*.jpg`.

See also: [waveshare-setup.md](waveshare-setup.md), [v1-roadmap/v1-phase-plan.md](v1-roadmap/v1-phase-plan.md) Phase F/G.
