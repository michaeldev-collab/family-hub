#pragma once
//
// panel_config.h — Waveshare ESP32-S3-Touch-LCD-7B (Type B) panel definition
// =========================================================================
// Board: ESP32-S3-Touch-LCD-7B
//   MCU      : ESP32-S3-WROOM-1-N16R8  (16 MB Flash, 8 MB OPI PSRAM)
//   Panel    : 7" IPS, 1024 x 600, RGB565, init-free TTL-RGB driver
//              (ST7262-class — the wiki FAQ's "ST7701" is a copy-paste error;
//               the schematic LCD connector has no SPI lines and the official
//               BSP sends no vendor init sequence at all)
//   Touch    : GT911, 5-point, I2C 0x5D (0x14 fallback)
//   Board ctl: CH32V003 MCU as I2C IO extension @ 0x24 — owns backlight
//              enable/PWM, LCD reset, LCD VDD enable, touch reset, SD CS
//
// Every value below is taken from the official sources:
//   - Wiki: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7B  (pinout,
//     I2C address map: 0x24 = IO extension, 0x5D = GT911)
//   - Schematic: ESP32-S3-Touch-LCD-7B-Schematic.pdf (EXIO net assignments)
//   - BSP: github.com/waveshareteam/ESP32-S3-Touch-LCD-7B
//     examples/Arduino/examples/13_LVGL_TRANSPLANT/{rgb_lcd_port,io_extension,
//     gt911}.{h,cpp}  (timings, register protocol, reset sequences)
//
// This file only applies when WAVESHARE_7B is defined (see platformio.ini env).
//
#ifdef WAVESHARE_7B

// ---- Geometry -----------------------------------------------------------
#define PANEL_H_RES            1024
#define PANEL_V_RES            600
#define PANEL_COLOR_DEPTH      16        // RGB565
#define PANEL_DATA_WIDTH       16

// ---- RGB sync / control pins (wiki "LCD port" table) --------------------
#define PANEL_PIN_DE           5
#define PANEL_PIN_VSYNC        3
#define PANEL_PIN_HSYNC        46
#define PANEL_PIN_PCLK         7

// ---- RGB data pins ------------------------------------------------------
// Named R0..R4/G0..G5/B0..B4 here (RGB565 bus order); the wiki/schematic name
// the same lines R3..R7/G2..G7/B3..B7 (panel's 8-bit-per-channel numbering).
// Verified identical to BSP rgb_lcd_port.h DATA0..DATA15 (B first, R last).
#define PANEL_PIN_R0           1    // R3
#define PANEL_PIN_R1           2    // R4
#define PANEL_PIN_R2           42   // R5
#define PANEL_PIN_R3           41   // R6
#define PANEL_PIN_R4           40   // R7
#define PANEL_PIN_G0           39   // G2
#define PANEL_PIN_G1           0    // G3
#define PANEL_PIN_G2           45   // G4
#define PANEL_PIN_G3           48   // G5
#define PANEL_PIN_G4           47   // G6
#define PANEL_PIN_G5           21   // G7
#define PANEL_PIN_B0           14   // B3
#define PANEL_PIN_B1           38   // B4
#define PANEL_PIN_B2           18   // B5
#define PANEL_PIN_B3           17   // B6
#define PANEL_PIN_B4           10   // B7

// ---- RGB timings (porches from BSP) --------------------------------------
// PCLK 16 MHz (BSP uses 30). With the vendor direct-mode double-FB + bounce
// model below, the panel no longer underruns on touch, so PCLK can be raised
// toward 30 MHz for higher refresh once this config is confirmed stable.
#define PANEL_PCLK_HZ          (16 * 1000 * 1000)
#define PANEL_HSYNC_PULSE      162
#define PANEL_HSYNC_BACK       152
#define PANEL_HSYNC_FRONT      48
#define PANEL_VSYNC_PULSE      45
#define PANEL_VSYNC_BACK       13
#define PANEL_VSYNC_FRONT      3
#define PANEL_PCLK_ACTIVE_NEG  1
// Vendor direct-mode config (official BSP AVOID_TEAR_MODE 3): two PSRAM
// framebuffers used directly as LVGL's draw buffers (see display.cpp), so the
// driver flips them on VSYNC (tear-free) and no separate internal draw buffers
// are allocated — which frees the internal SRAM the bounce buffer needs.
#define PANEL_NUM_FBS          2
// Bounce buffer ON: RGB DMA is fed from internal SRAM instead of straight from
// PSRAM, so CPU/PSRAM spikes during touch redraws no longer starve the scanout
// (kills the on-touch wobble = FIFO underrun). Fits now that LVGL's buffers are
// the framebuffers, not a separate 40 KB internal allocation. Safe re: the old
// "bounce ISR during flash-cache-off" concern because SPI-flash writes are
// gated off while the display is live and WiFi runs persistent(false).
#define PANEL_BOUNCE_PX        (PANEL_H_RES * 10)

// ---- Shared I2C bus (GT911 + CH32V003) ----------------------------------
#define BOARD_I2C_SDA          8
#define BOARD_I2C_SCL          9
#define BOARD_I2C_FREQ_HZ      100000  // 100 kHz — CH32 IO-ext was flaky at 400 kHz

// ---- CH32V003 IO extension @ 0x24 (BSP io_extension.{h,cpp}) ------------
#define IOEXT_ADDR             0x24
#define IOEXT_REG_MODE         0x02   // bitmask: 1 = output
#define IOEXT_REG_OUTPUT       0x03   // output level bitmask (all 8 pins)
#define IOEXT_REG_INPUT        0x04
#define IOEXT_REG_PWM          0x05   // backlight duty 0..255 (see clamp note)
#define IOEXT_REG_ADC          0x06   // battery voltage, 16-bit read
#define IOEXT_IO_TP_RST        1      // EXIO1 — GT911 reset
#define IOEXT_IO_DISP          2      // EXIO2 — backlight enable (AP3032 EN)
#define IOEXT_IO_LCD_RST       3      // EXIO3 — LCD panel reset (J1 pin 44)
#define IOEXT_IO_SD_CS         4      // EXIO4 — SD card chip select
#define IOEXT_IO_USB_CAN_SEL   5      // EXIO5 — 0 = USB, 1 = CAN
#define IOEXT_IO_LCD_VDD_EN    6      // EXIO6 — panel VCOM/VDD enable

// ---- Sleep / wake button (sensor header GP6 ↔ GND) ----------------------
// Waveshare "GPIO" HY2.0 header: GP6 is the only fully free ESP32 GPIO.
// Not to be confused with IOEXT_IO_LCD_VDD_EN (EXIO6 on the CH32 expander).
#define SLEEP_BUTTON_GPIO      6

// ---- Touch (GT911) ------------------------------------------------------
#define TOUCH_GT911_ADDR       0x5D   // INT low during reset -> 0x5D
#define TOUCH_GT911_ADDR_ALT   0x14   // fallback if INT floated high
#define TOUCH_GT911_INT_PIN    4      // wiki: GPIO4 = TP_IRQ
// GT911 reset is EXIO1 on the IO extension (no direct GPIO).
// BSP orientation: swap_xy=0, mirror_x=0, mirror_y=0 (native 1024x600).

// ---- Backlight ----------------------------------------------------------
#define BL_DEFAULT_PERCENT     80     // 0..100; Waveshare clamps PWM Value at
                                      // 97 — do not write 100/255 for "off"

// ---- LVGL draw-buffer sizing --------------------------------------------
// Two partial buffers in internal DMA-capable SRAM.
// 1024 * 10 * 2B = 20 KB each / 40 KB total, leaving heap for LVGL, Wi-Fi,
// HTTP, FreeRTOS, and runtime allocations after the RGB framebuffer exists.
#define LVGL_BUF_LINES         10

#endif // WAVESHARE_7B
