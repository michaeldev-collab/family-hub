# Elecrow CrowPanel 7" HMI — Phase G Reference (Secondary)

**Product:** [CrowPanel 7inch-HMI ESP32 Display 800×480](https://www.elecrow.com/esp32-display-7-inch-hmi-display-rgb-tft-lcd-touch-screen-support-lvgl.html)  
**SKU:** DIS08070H-1  
**Wiki:** [CrowPanel ESP32 7.0-inch with PlatformIO](https://www.elecrow.com/wiki/CrowPanel_ESP32_7.0-inch_with_PlatformIO.html)  
**Priority:** Secondary / V1.1 bonus — do not block Waveshare path.

Concise prep for Family Hub when this panel is used instead of (or alongside) Waveshare. Do not flash until `secrets.h` WiFi + endeavor host are set.

---

## Board at a glance

| Item | Value |
|------|--------|
| MCU | **ESP32-S3-WROOM-1-N4R8** (dual-core LX7, up to 240 MHz) |
| Wireless | 2.4 GHz Wi‑Fi (802.11 b/g/n), Bluetooth 5 |
| Flash / PSRAM | **4 MB Flash** + **8 MB PSRAM** |
| Display | 7" TN, **800×480**, RGB parallel, 65K colors |
| Display driver IC | **EK9716BD3** + **EK73002ACGB** |
| Touch | Capacitive, **GT911** (I²C addr **0x5D**) |
| GPIO expander | **PCA9557** (backlight/touch reset on some revs) |
| Power | USB Type-C **5 V @ 2 A** recommended; battery 3.7–4.2 V optional |
| Boot | **BOOT** + **RESET** for USB flash mode |

**Not the same as Waveshare:** Elecrow uses **LovyanGFX** + custom RGB pin map; Waveshare uses **ESP32_Display_Panel** + **CH422G** expander. Pin maps and init code are **not interchangeable**.

---

## Hardware revisions (critical)

| Rev | Notes |
|-----|--------|
| Pre-V3.0 | GT911 touch via `touch.h` in Elecrow sample; PCA9557 present |
| **V3.0+** | Schematic shows **PCA9557** for touch/display control; **GT911 sample code often fails** — touch pins or expander sequence differ ([LVGL forum report](https://forum.lvgl.io/t/crowpanel-esp32-display-7-inch/17434)) |

Identify your PCB silkscreen / purchase batch before trusting Elecrow's stock PlatformIO zip.

---

## Arduino / PlatformIO setup

### Board package

Elecrow does **not** ship a standard Arduino board name in Espressif core. Their wiki uses a **custom board JSON**:

1. Download [esp32-s3-devkitc-1-myboard.json](https://www.elecrow.com/wiki/CrowPanel_ESP32_7.0-inch_with_PlatformIO.html) from the wiki.
2. Place in PlatformIO boards dir, e.g. `~/.platformio/platforms/espressif32/boards/`.
3. Select board: **`Espressif ESP32-S3-DevKitC-1-N8 -ELECROW`** in PlatformIO new-project wizard.

Family Hub stub env uses generic `esp32-s3-devkitc-1` until custom JSON is installed — enough to compile `#ifdef ELECROW_7` stubs, **not** enough for LovyanGFX display without the JSON + libs.

### Recommended libraries (from Elecrow wiki)

| Library | Role |
|---------|------|
| **LovyanGFX** | RGB panel + DMA flush (`Panel_RGB`, `Bus_RGB`) |
| **lvgl** | **8.3.x** in Elecrow SquareLine examples; match SquareLine export version |
| **GT911** | Capacitive touch driver |
| **PCA9557** | I²C GPIO expander (reset/backlight on some boards) |
| Adafruit_GFX / Adafruit_BusIO | Dependency chain in samples |

Copy Elecrow-provided **`lv_conf.h`** into `.pio/libdeps/<env>/` — do not use Waveshare's `lv_conf.h`.

### Key pin map (Elecrow 7" RGB — from official PlatformIO tutorial)

| Function | GPIO |
|----------|------|
| RGB data B0–B4 | 15, 7, 6, 5, 4 |
| RGB data G0–G5 | 9, 46, 3, 8, 16, 1 |
| RGB data R0–R4 | 14, 21, 47, 48, 45 |
| HSYNC / VSYNC / DE / PCLK | 39, 40, 41, 0 |
| Backlight `TFT_BL` | **GPIO 2** (PWM via `ledc`) |
| Touch I²C | **SDA 19, SCL 20** |
| Crowtail / LED demo | GPIO 38 |

Touch init lives in Elecrow **`touch.h`** (not in main.cpp) — must match hardware rev.

---

## PlatformIO (Family Hub `elecrow7` env)

Stub in `firmware/platformio.ini` — compile-only until LovyanGFX stack is added:

```ini
[env:elecrow7]
board = esp32-s3-devkitc-1
board_build.arduino.memory_type = qio_opi
board_build.f_flash = 80000000L
board_upload.flash_size = 4MB
build_flags =
    -DCORE_DEBUG_LEVEL=1
    -DELECROW_7=1
    -DBOARD_HAS_PSRAM
    -DFIRMWARE_VERSION=\"0.1.0\"
; TODO when hardware in hand:
; 1. Install esp32-s3-devkitc-1-myboard.json (Elecrow wiki)
; 2. lib_deps: lovyanfx/LovyanGFX, lvgl/lvgl@8.3.11, GT911, PCA9557
; 3. Copy Elecrow lv_conf.h + touch.h from wiki demo zip
; 4. board = esp32-s3-devkitc-1-myboard  (after JSON install)
```

**Flash size:** N4R8 = **4 MB** flash (not 8 MB like many Waveshare units). Wrong partition table → upload failures or boot loops.

---

## Flash notes

| Topic | Guidance |
|-------|----------|
| USB | Single Type-C — power + serial + upload |
| Monitor baud | **115200** (some Elecrow samples use 9600 in snippets — prefer 115200) |
| Flash mode | Hold **BOOT**, tap **RESET**, release BOOT |
| PSRAM | Required for LVGL — `-DBOARD_HAS_PSRAM`, `qio_opi` |
| Driver | Install [Silicon Labs CP210x](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) if port missing |

---

## Family Hub integration checklist (Phase G-Elecrow, optional)

1. Complete **Waveshare Phase G** first (primary panel).
2. `cp firmware/include/secrets.example.h firmware/include/secrets.h`.
3. Install Elecrow board JSON + LovyanGFX/LVGL/GT911/PCA9557 per wiki.
4. `pio run -e elecrow7` — must link with display libs.
5. Implement `#elif defined(ELECROW_7)` in `ui_manager.cpp` (mirror Waveshare hooks).
6. Verify `/api/dashboard-state` + one write on **screen**.
7. Capture evidence: `docs/verification-evidence/G-elecrow-panel-home-*.jpg`.

See also: [panel-targets.md](panel-targets.md), [waveshare-esp32-s3-touch-lcd-7-reference.md](waveshare-esp32-s3-touch-lcd-7-reference.md).

---

## Troubleshooting — "could never get it to work"

Common failure modes reported by users and Elecrow forum threads. Work through in order.

### 1. Wrong board / flash size

**Symptom:** Upload succeeds but blank screen, or `invalid header` / boot loop.  
**Fix:** Use Elecrow custom **`esp32-s3-devkitc-1-myboard.json`** or set `board_upload.flash_size = 4MB`. N4R8 is **4 MB flash**, not 8 MB. Partition table must match.

### 2. Missing or wrong board package

**Symptom:** Board not listed; builds for generic DevKitC with wrong pins.  
**Fix:** Copy Elecrow JSON to PlatformIO `boards/` folder; restart VS Code / PlatformIO. Do **not** assume Waveshare board package works.

### 3. LovyanGFX / LVGL version mismatch

**Symptom:** Compile errors in `Panel_RGB.hpp`, `lv_conf.h` conflicts, or demos folder missing.  
**Fix:** Use LovyanGFX version from [Elecrow demo zip](https://www.elecrow.com/wiki/CrowPanel_ESP32_7.0-inch_with_PlatformIO.html). Match LVGL to SquareLine export (**8.3.11** in Advance docs). Copy **`demos`/`examples`** from lvgl into `src/` only if your tutorial requires it — prefer minimal deps.

### 4. Touch dead (especially V3.0 hardware)

**Symptom:** Display OK, no touch; or compile errors in `touch_init()`.  
**Fix:** Check PCB rev. V3.0 may need **PCA9557** reset sequence before GT911 init — stock GT911-only `touch.h` may be wrong. Compare schematic from wiki. Community starter: [MikeWarriner/CrowPanel7inch](https://github.com/MikeWarriner/CrowPanel7inch). EEZ Studio / alternate exports sometimes work when SquareLine + stock touch.h do not.

### 5. PSRAM not enabled

**Symptom:** Boot crash, `Guru Meditation`, or LVGL alloc failure.  
**Fix:** `board_build.arduino.memory_type = qio_opi`, `-DBOARD_HAS_PSRAM`. 800×480 LVGL buffer needs PSRAM.

### 6. Wrong USB port / driver

**Symptom:** No `/dev/ttyUSB*` or `COM` port.  
**Fix:** Install CP210x driver; try another cable (data, not charge-only). Hold BOOT during upload.

### 7. Backlight off (looks "dead")

**Symptom:** Upload OK, black screen, faint image with flashlight.  
**Fix:** Init **GPIO 2** backlight — `ledcSetup` + `ledcWrite(255)` or digital toggle per Elecrow sample. PCA9557 may also gate backlight on newer revs.

### 8. I²C touch bus wrong pins

**Symptom:** Display works, touch frozen.  
**Fix:** Confirm `Wire.begin(19, 20)` for SDA/SCL. GT911 address **0x5D** (sometimes 0x14 on other boards — not this one per Elecrow Advance docs).

### 9. Stale PlatformIO / library cache

**Symptom:** Errors that disappear on a clean machine ([forum report](https://forum.lvgl.io/t/crowpanel-esp32-display-7-inch/17434)).  
**Fix:** `pio run -t clean`, delete `.pio/libdeps/elecrow7`, fresh clone of Elecrow demo project, compare `platformio.ini` line-by-line.

### 10. Confused with Waveshare or "Advanced" Elecrow

**Symptom:** Using Waveshare `ESP32_Display_Panel` or CrowPanel **Advance** (ESP32-P4, 1024×600) docs on Basic 7" DIS08070H.  
**Fix:** This doc is **Basic CrowPanel 7"** (ESP32-S3 N4R8, 800×480, EK9716/EK73002). Advanced line uses different MCU, resolution, and STC8H backlight MCU — different codebase entirely.

---

## Community resources

| Resource | URL |
|----------|-----|
| Elecrow PlatformIO tutorial | https://www.elecrow.com/wiki/CrowPanel_ESP32_7.0-inch_with_PlatformIO.html |
| LVGL forum thread | https://forum.lvgl.io/t/crowpanel-esp32-display-7-inch/17434 |
| Community starter project | https://github.com/MikeWarriner/CrowPanel7inch |
| openHASP (alternate firmware) | Linked from product page |

---

*Last updated: 2026-06-18 — secondary panel; Waveshare remains primary.*
