// display.cpp — Family Hub panel bring-up.
//
// WAVESHARE_7B build: init-free RGB panel (1024x600, ST7262-class) + GT911
// touch + CH32V003 IO-extension (backlight / LCD reset / touch reset) + LVGL v8.
// All hardware values are taken from the official Waveshare 7B BSP
// (github.com/waveshareteam/ESP32-S3-Touch-LCD-7B) — see panel_config.h.
//
// FIRST_BOOT_DIAG (default on): staged serial diagnostics — flash/PSRAM
// report, I2C scan, solid R/G/B/W/black test screens, touch echo — so a first
// flash can distinguish backlight vs panel vs timing vs touch failures.
//
// Other builds: no-op stubs (serial UI stays in charge).
//
#include "display.h"

#if defined(WAVESHARE_7B)

#include "panel_config.h"
#include <Wire.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <esp_memory_utils.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#ifndef FIRST_BOOT_DIAG
#define FIRST_BOOT_DIAG 1   // set -DFIRST_BOOT_DIAG=0 once bring-up is proven
#endif

// On-screen cyan touch dot + "TOUCH x,y" label. Bring-up diagnostic only — in
// normal use it reads as a glitch floating over the UI. Off by default; enable
// with -DSHOW_TOUCH_MARKER=1 to visualize raw touch coordinates.
#ifndef SHOW_TOUCH_MARKER
#define SHOW_TOUCH_MARKER 0
#endif

static esp_lcd_panel_handle_t s_panel = nullptr;
static lv_disp_draw_buf_t     s_draw_buf;
static lv_disp_drv_t          s_disp_drv;
static lv_indev_drv_t         s_indev_drv;
static SemaphoreHandle_t      s_lock = nullptr;
static bool                   s_ready = false;
static uint8_t                s_gt911_addr = 0;   // 0 = touch not detected
// Task that runs lv_timer_handler()/the flush (the Arduino loop task). The RGB
// frame-finish ISR notifies it so the flush can wait for scan-out completion
// before LVGL reuses/flips a framebuffer (tear-free direct mode).
static TaskHandle_t           s_lvgl_task_handle = nullptr;
static bool                   s_first_timer_logged = false;
static bool                   s_first_flush_seen = false;
static bool                   s_first_flush_completed = false;
static bool                   s_first_touch_logged = false;
static uint16_t               s_touch_x = 0;
static uint16_t               s_touch_y = 0;
static uint32_t               s_touch_seq = 0;
static uint32_t               s_touch_rendered_seq = 0;
static uint32_t               s_last_touch_ms = 0;
static lv_obj_t*              s_touch_dot = nullptr;
static lv_obj_t*              s_touch_label = nullptr;

// ---------------------------------------------------------------------------
// CH32V003 IO extension @ 0x24 — register protocol from BSP io_extension.cpp.
// The output register holds all 8 pins at once, so we cache the last value
// (BSP does the same, initial value 0xFF = everything high/enabled).
// ---------------------------------------------------------------------------
static uint8_t s_ioext_shadow = 0xFF & ~(1U << IOEXT_IO_TP_RST) &
                                ~(1U << IOEXT_IO_DISP) &
                                ~(1U << IOEXT_IO_LCD_RST);
static bool    s_ioext_ok = false;
static bool    s_backlight_ready = false;

static bool ioext_write2(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(IOEXT_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool ioext_output(uint8_t pin, uint8_t value) {
  if (value) s_ioext_shadow |=  (1 << pin);
  else       s_ioext_shadow &= ~(1 << pin);
  return ioext_write2(IOEXT_REG_OUTPUT, s_ioext_shadow);
}

static bool ioext_init() {
  // Establish safe levels before switching the expander pins to outputs.
  s_ioext_ok = ioext_write2(IOEXT_REG_OUTPUT, s_ioext_shadow) &&
               ioext_write2(IOEXT_REG_MODE, 0xFF);
  return s_ioext_ok;
}

// Backlight duty via CH32V003 PWM register. BSP clamps at 97%: driving the
// AP3032 enable at a solid 100% duty turns the backlight OFF, not brighter.
void displaySetBacklight(uint8_t percent) {
  percent = constrain(percent, 0, 100);
  uint8_t dimming = 100 - percent;
  if (dimming > 97) dimming = 97;
  uint8_t raw = (uint8_t)(dimming * 255U / 100U);
  if (!ioext_write2(IOEXT_REG_PWM, raw)) {
    Serial.println("[disp] FAIL: backlight PWM write to CH32V003 @0x24 NACKed");
  }
  ioext_output(IOEXT_IO_DISP, s_backlight_ready && percent > 0);
}

// ---------------------------------------------------------------------------
// Diagnostics
// ---------------------------------------------------------------------------
static void diag_report_memory(const char* when) {
  Serial.printf("[diag] %s: flash=%uKB psram=%uKB (free %uKB) heap free=%uKB (min %uKB)\n",
                when,
                (unsigned)(ESP.getFlashChipSize() / 1024),
                (unsigned)(ESP.getPsramSize() / 1024),
                (unsigned)(ESP.getFreePsram() / 1024),
                (unsigned)(ESP.getFreeHeap() / 1024),
                (unsigned)(ESP.getMinFreeHeap() / 1024));
}

static void diag_report_internal_heap(const char* when) {
  Serial.printf("[diag] %s: internal heap free=%uKB largest=%uKB dma free=%uKB\n",
                when,
                (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024),
                (unsigned)(heap_caps_get_free_size(MALLOC_CAP_DMA) / 1024));
}

static void diag_i2c_scan() {
  Serial.print("[diag] I2C scan:");
  int found = 0;
  for (uint8_t a = 0x08; a < 0x78; ++a) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf(" 0x%02X", a); ++found; }
  }
  if (!found) Serial.print(" (none)");
  Serial.println();
  Serial.println("[diag] expected: 0x24 = CH32V003 IO ext, 0x5D (or 0x14) = GT911");
}

#if FIRST_BOOT_DIAG
// Fill the whole panel with one RGB565 color, drawn in strips from a small
// SRAM buffer so this works even if PSRAM allocation is broken.
static void diag_fill(uint16_t rgb565, const char* name) {
  const int STRIP = 10;
  static uint16_t strip[PANEL_H_RES * STRIP];
  for (int i = 0; i < PANEL_H_RES * STRIP; ++i) strip[i] = rgb565;
  for (int y = 0; y < PANEL_V_RES; y += STRIP) {
    int h = (y + STRIP <= PANEL_V_RES) ? STRIP : (PANEL_V_RES - y);
    esp_lcd_panel_draw_bitmap(s_panel, 0, y, PANEL_H_RES, y + h, strip);
  }
  Serial.printf("[diag] test screen: %s\n", name);
}

static void diag_test_screens() {
  Serial.println("[diag] color bars: verify RED then GREEN then BLUE render as"
                 " named (wrong order = data-pin/channel mix-up); rolling or"
                 " sheared image = timing problem; white uniform = good PCLK");
  struct { uint16_t c; const char* n; } seq[] = {
    { 0xF800, "RED"   },
    { 0x07E0, "GREEN" },
    { 0x001F, "BLUE"  },
    { 0xFFFF, "WHITE" },
    { 0x0000, "BLACK" },
  };
  for (auto& s : seq) { diag_fill(s.c, s.n); delay(1500); }
}
#endif

// ---------------------------------------------------------------------------
// Panel power + reset — all through the CH32V003 (no direct GPIOs for this).
// The panel driver is init-free: after VDD + reset + pixel clock it just
// displays whatever the RGB bus streams. There is NO SPI init sequence on
// this board (the old ST7701 assumption was wrong — see panel_config.h).
// ---------------------------------------------------------------------------
static bool panel_power_sequence() {
  if (!s_ioext_ok) {
    Serial.println("[disp] FAIL: CH32V003 IO extension @0x24 not responding —"
                   " cannot enable panel VDD / reset / backlight");
    return false;
  }
  bool ok = true;
  ok &= ioext_output(IOEXT_IO_LCD_VDD_EN, 1);   // panel VCOM/VDD rail on
  delay(10);
  ok &= ioext_output(IOEXT_IO_LCD_RST, 0);      // panel reset pulse
  delay(10);
  ok &= ioext_output(IOEXT_IO_LCD_RST, 1);
  delay(120);
  if (!ok) Serial.println("[disp] FAIL: IO-extension write NACKed during panel power-up");
  return ok;
}

// ---------------------------------------------------------------------------
// RGB panel — geometry/timings mirror the BSP, with two stability adjustments
// for Arduino/IDF cache behavior: no bounce-buffer ISR copy from PSRAM, and a
// lower PCLK to reduce PSRAM bandwidth pressure.
// ---------------------------------------------------------------------------
// RGB frame-finish ISR: fires when the panel completes scanning a frame (via
// the bounce-buffer path). Wakes the LVGL/flush task so its draw_bitmap flip
// is known to have been shown before LVGL reuses the other framebuffer.
IRAM_ATTR static bool rgb_on_frame_finish(esp_lcd_panel_handle_t panel,
                                          const esp_lcd_rgb_panel_event_data_t* edata,
                                          void* user_ctx) {
  BaseType_t hp_task_awoken = pdFALSE;
  if (s_lvgl_task_handle) {
    vTaskNotifyGiveFromISR(s_lvgl_task_handle, &hp_task_awoken);
  }
  return hp_task_awoken == pdTRUE;
}

static bool rgb_panel_init() {
  esp_lcd_rgb_panel_config_t cfg = {};
  cfg.clk_src = LCD_CLK_SRC_DEFAULT;
  cfg.timings.pclk_hz           = PANEL_PCLK_HZ;
  cfg.timings.h_res             = PANEL_H_RES;
  cfg.timings.v_res             = PANEL_V_RES;
  cfg.timings.hsync_pulse_width = PANEL_HSYNC_PULSE;
  cfg.timings.hsync_back_porch  = PANEL_HSYNC_BACK;
  cfg.timings.hsync_front_porch = PANEL_HSYNC_FRONT;
  cfg.timings.vsync_pulse_width = PANEL_VSYNC_PULSE;
  cfg.timings.vsync_back_porch  = PANEL_VSYNC_BACK;
  cfg.timings.vsync_front_porch = PANEL_VSYNC_FRONT;
  cfg.timings.flags.pclk_active_neg = PANEL_PCLK_ACTIVE_NEG;
  cfg.data_width        = PANEL_DATA_WIDTH;
  cfg.bits_per_pixel    = PANEL_COLOR_DEPTH;
  cfg.num_fbs           = PANEL_NUM_FBS;
  cfg.bounce_buffer_size_px = PANEL_BOUNCE_PX;
  cfg.psram_trans_align = 64;
  cfg.hsync_gpio_num = PANEL_PIN_HSYNC;
  cfg.vsync_gpio_num = PANEL_PIN_VSYNC;
  cfg.de_gpio_num    = PANEL_PIN_DE;
  cfg.pclk_gpio_num  = PANEL_PIN_PCLK;
  cfg.disp_gpio_num  = -1;
  const int data[] = {   // DATA0..15 = B3..B7, G2..G7, R3..R7 (BSP order)
    PANEL_PIN_B0, PANEL_PIN_B1, PANEL_PIN_B2, PANEL_PIN_B3, PANEL_PIN_B4,
    PANEL_PIN_G0, PANEL_PIN_G1, PANEL_PIN_G2, PANEL_PIN_G3, PANEL_PIN_G4, PANEL_PIN_G5,
    PANEL_PIN_R0, PANEL_PIN_R1, PANEL_PIN_R2, PANEL_PIN_R3, PANEL_PIN_R4,
  };
  for (int i = 0; i < PANEL_DATA_WIDTH; ++i) cfg.data_gpio_nums[i] = data[i];
  cfg.flags.fb_in_psram = 1;
  cfg.flags.bb_invalidate_cache = 0;

  Serial.printf("[disp] RGB config: fbs=%u fb_in_psram=1 bounce_px=%u psram_align=%u\n",
                (unsigned)cfg.num_fbs,
                (unsigned)cfg.bounce_buffer_size_px,
                (unsigned)cfg.psram_trans_align);

  esp_err_t err = esp_lcd_new_rgb_panel(&cfg, &s_panel);
  if (err != ESP_OK) {
    Serial.printf("[disp] FAIL: esp_lcd_new_rgb_panel err=0x%X (PSRAM missing"
                  " or framebuffer alloc failed?)\n", err);
    return false;
  }
  err = esp_lcd_panel_init(s_panel);
  if (err != ESP_OK) {
    Serial.printf("[disp] FAIL: esp_lcd_panel_init err=0x%X\n", err);
    return false;
  }

  // Register the frame-finish callback used by the direct-mode flush to wait
  // for scan-out. With a bounce buffer the relevant event is on_bounce_frame_finish.
  esp_lcd_rgb_panel_event_callbacks_t cbs = {};
#if PANEL_BOUNCE_PX > 0
  cbs.on_bounce_frame_finish = rgb_on_frame_finish;
#else
  cbs.on_vsync = rgb_on_frame_finish;
#endif
  err = esp_lcd_rgb_panel_register_event_callbacks(s_panel, &cbs, nullptr);
  if (err != ESP_OK) {
    Serial.printf("[disp] FAIL: register RGB event callbacks err=0x%X\n", err);
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// GT911 touch — reset sequence + address probe per BSP gt911.cpp:
// hold INT (GPIO4) low while releasing TP_RST (EXIO1) => address 0x5D.
// ---------------------------------------------------------------------------
enum class TouchFrameResult : uint8_t { NoFrame, Released, Pressed, Error };

static bool gt911_read_register(uint16_t reg, uint8_t* data, size_t length) {
  if (!s_gt911_addr || !data || !length) return false;
  Wire.beginTransmission(s_gt911_addr);
  Wire.write((uint8_t)(reg >> 8));
  Wire.write((uint8_t)reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)s_gt911_addr, (int)length, (int)true) != length) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (size_t i = 0; i < length; ++i) data[i] = Wire.read();
  return true;
}

static bool gt911_write_register(uint16_t reg, uint8_t value) {
  if (!s_gt911_addr) return false;
  Wire.beginTransmission(s_gt911_addr);
  Wire.write((uint8_t)(reg >> 8));
  Wire.write((uint8_t)reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool gt911_probe(uint8_t addr) {
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() != 0) return false;
  s_gt911_addr = addr;
  uint8_t id[4] = {};
  if (!gt911_read_register(0x8140, id, sizeof(id))) {
    s_gt911_addr = 0;
    return false;
  }
  Serial.printf("[disp] GT911 @0x%02X product ID: %.4s\n", addr,
                reinterpret_cast<const char*>(id));
  return true;
}

static bool gt911_init() {
  pinMode(TOUCH_GT911_INT_PIN, OUTPUT);
  digitalWrite(TOUCH_GT911_INT_PIN, LOW);       // INT low => addr 0x5D
  if (s_ioext_ok) {
    ioext_output(IOEXT_IO_TP_RST, 0);           // reset touch
    delay(100);
    ioext_output(IOEXT_IO_TP_RST, 1);           // release with INT still low
    delay(200);
  } else {
    Serial.println("[disp] WARN: no IO extension — cannot pulse GT911 reset");
    delay(200);
  }
  pinMode(TOUCH_GT911_INT_PIN, INPUT);          // hand INT back to GT911

  if (gt911_probe(TOUCH_GT911_ADDR))          { s_gt911_addr = TOUCH_GT911_ADDR; }
  else if (gt911_probe(TOUCH_GT911_ADDR_ALT)) { s_gt911_addr = TOUCH_GT911_ADDR_ALT; }
  if (!s_gt911_addr) {
    Serial.println("[disp] FAIL: GT911 not found at 0x5D or 0x14 — touch disabled");
    return false;
  }
  // Force normal coordinate mode. Raw-data mode can produce repeated or
  // malformed frames that LVGL interprets as multiple button presses.
  if (!gt911_write_register(0x8040, 0x00)) {
    Serial.println("[disp] FAIL: GT911 coordinate-mode command NACKed");
    s_gt911_addr = 0;
    return false;
  }
  uint8_t range[4] = {};
  if (gt911_read_register(0x8048, range, sizeof(range))) {
    const uint16_t x_max = range[0] | ((uint16_t)range[1] << 8);
    const uint16_t y_max = range[2] | ((uint16_t)range[3] << 8);
    Serial.printf("[disp] GT911 coordinate range: %ux%u\n", x_max, y_max);
  }
  return true;
}

static TouchFrameResult gt911_read(uint16_t* x, uint16_t* y) {
  if (!s_gt911_addr) return TouchFrameResult::Error;
  uint8_t status = 0;
  if (!gt911_read_register(0x814E, &status, 1)) return TouchFrameResult::Error;
  if (!(status & 0x80)) return TouchFrameResult::NoFrame;

  const uint8_t point_count = status & 0x0F;
  if (point_count == 0) {
    return gt911_write_register(0x814E, 0) ? TouchFrameResult::Released
                                           : TouchFrameResult::Error;
  }
  if (point_count > 5) {
    Serial.printf("[disp] GT911 invalid point count: %u\n", point_count);
    gt911_write_register(0x814E, 0);
    return TouchFrameResult::Error;
  }

  uint8_t points[5 * 8] = {};
  if (!gt911_read_register(0x814F, points, point_count * 8)) {
    return TouchFrameResult::Error;
  }
  const uint16_t raw_x = points[1] | ((uint16_t)points[2] << 8);
  const uint16_t raw_y = points[3] | ((uint16_t)points[4] << 8);
  if (!gt911_write_register(0x814E, 0)) return TouchFrameResult::Error;
  if (raw_x >= PANEL_H_RES || raw_y >= PANEL_V_RES) {
    Serial.printf("[disp] GT911 out-of-range point: %u,%u\n", raw_x, raw_y);
    return TouchFrameResult::Error;
  }
  *x = raw_x;
  *y = raw_y;
  return TouchFrameResult::Pressed;
}

#if FIRST_BOOT_DIAG
static void diag_touch_echo(uint32_t window_ms) {
  if (!s_gt911_addr) { Serial.println("[diag] touch echo skipped (no GT911)"); return; }
  Serial.printf("[diag] touch echo for %us — touch corners; expect (0,0) top-left"
                " .. (1023,599) bottom-right; swapped/mirrored values mean"
                " orientation flags need adjusting\n", (unsigned)(window_ms / 1000));
  uint32_t until = millis() + window_ms;
  while (millis() < until) {
    uint16_t x, y;
    if (gt911_read(&x, &y) == TouchFrameResult::Pressed) {
      Serial.printf("[diag] touch x=%u y=%u\n", x, y);
    }
    delay(20);
  }
}
#endif

// ---------------------------------------------------------------------------
// LVGL flush + touch glue
// ---------------------------------------------------------------------------
static void lvgl_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* px) {
  if (!s_first_flush_seen) {
    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;
    Serial.printf("[lvgl] first flush callback area=(%d,%d)-(%d,%d) %ldx%ld px=%p internal=%s dma=%s\n",
                  (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2,
                  (long)w, (long)h, px,
                  esp_ptr_internal(px) ? "yes" : "no",
                  esp_ptr_dma_capable(px) ? "yes" : "no");
    s_first_flush_seen = true;
  }

  // Direct mode (vendor AVOID_TEAR_MODE 3): LVGL's draw buffers ARE the two RGB
  // framebuffers. Only act on the final area of a refresh; then hand the just-
  // rendered framebuffer (px) to the driver, which flips it to scan-out on the
  // next VSYNC, and wait for that flip to finish before LVGL reuses the other
  // framebuffer. A timeout guards against a missed notification hanging the loop.
  esp_err_t err = ESP_OK;
  if (lv_disp_flush_is_last(drv)) {
    // In LVGL direct mode `px` is a complete RGB framebuffer even when `area`
    // describes only the dirty rectangle. Hand the complete buffer to the RGB
    // driver so it flips framebuffers; treating it as a compact partial-area
    // buffer corrupts scanout after navigation/partial redraws.
    ulTaskNotifyValueClear(nullptr, ULONG_MAX);
    err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, PANEL_H_RES, PANEL_V_RES, px);
    if (err != ESP_OK) {
      Serial.printf("[lvgl] flush draw_bitmap failed err=0x%X\n", err);
    } else {
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));   // wait for scan-out (bounded)
    }
    if (err == ESP_OK && !s_first_flush_completed) {
      s_first_flush_completed = true;
      Serial.println("[lvgl] first flush completed");
    }
  }
  lv_disp_flush_ready(drv);
}

static void lvgl_touch(lv_indev_drv_t*, lv_indev_data_t* data) {
  static uint16_t last_x = 0, last_y = 0;
  static bool touch_active = false;
  static uint32_t last_pressed_frame_ms = 0;
  uint16_t x = 0, y = 0;
  const TouchFrameResult frame = gt911_read(&x, &y);
  if (frame == TouchFrameResult::Pressed) {
    last_x = x; last_y = y;
    touch_active = true;
    last_pressed_frame_ms = millis();
    const uint32_t now = millis();
    if ((uint16_t)abs((int)x - (int)s_touch_x) > 8 ||
        (uint16_t)abs((int)y - (int)s_touch_y) > 8 ||
        now - s_last_touch_ms > 250) {
      s_touch_x = x;
      s_touch_y = y;
      s_last_touch_ms = now;
      ++s_touch_seq;
    }
    data->state = LV_INDEV_STATE_PRESSED;
    if (!s_first_touch_logged) {
      Serial.printf("[lvgl] first touch read through LVGL x=%u y=%u\n", x, y);
      s_first_touch_logged = true;
    }
  } else if (frame == TouchFrameResult::Released) {
    touch_active = false;
  } else if (frame == TouchFrameResult::NoFrame && touch_active &&
             millis() - last_pressed_frame_ms > 80) {
    // Some GT911 firmware revisions stop publishing frames after lift instead
    // of sending a ready frame with point_count=0. Synthesize one release only
    // after a quiet interval: long enough to bridge normal report gaps, short
    // enough for LVGL to complete the click promptly.
    touch_active = false;
  }
  // NoFrame means the finger state has not changed. Preserving the held state
  // prevents gaps between GT911 reports from becoming repeated LVGL clicks.
  data->state = touch_active ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  data->point.x = last_x;
  data->point.y = last_y;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
bool displayReady() { return s_ready; }

static void lvgl_update_touch_marker() {
#if !SHOW_TOUCH_MARKER
  return;   // touch dot disabled in normal operation
#else
  if (s_touch_rendered_seq == s_touch_seq) return;
  s_touch_rendered_seq = s_touch_seq;

  lv_obj_t* scr = lv_scr_act();
  if (!s_touch_dot) {
    s_touch_dot = lv_obj_create(scr);
    lv_obj_set_size(s_touch_dot, 22, 22);
    lv_obj_set_style_radius(s_touch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_touch_dot, lv_color_hex(0x88C0D0), 0);
    lv_obj_set_style_border_width(s_touch_dot, 0, 0);
    lv_obj_clear_flag(s_touch_dot, LV_OBJ_FLAG_SCROLLABLE);
  }
  if (!s_touch_label) {
    s_touch_label = lv_label_create(scr);
    lv_obj_set_style_text_color(s_touch_label, lv_color_hex(0x88C0D0), 0);
    lv_obj_set_style_text_font(s_touch_label, &lv_font_montserrat_20, 0);
  }

  const int dot_x = constrain((int)s_touch_x - 11, 0, PANEL_H_RES - 22);
  const int dot_y = constrain((int)s_touch_y - 11, 0, PANEL_V_RES - 22);
  lv_obj_set_pos(s_touch_dot, dot_x, dot_y);

  char label[32];
  snprintf(label, sizeof(label), "TOUCH %u,%u", s_touch_x, s_touch_y);
  lv_label_set_text(s_touch_label, label);
  lv_obj_align(s_touch_label, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
  lv_obj_move_foreground(s_touch_dot);
  lv_obj_move_foreground(s_touch_label);
#endif  // SHOW_TOUCH_MARKER
}

static void lvgl_handler_once(const char* context) {
  if (!s_first_timer_logged) {
    Serial.printf("[lvgl] first lv_timer_handler() call (%s)\n", context);
    s_first_timer_logged = true;
  }
  lv_timer_handler();
}

static bool lvgl_first_render_smoke() {
  Serial.println("[lvgl] creating internal boot screen");
  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* label = lv_label_create(scr);
  lv_label_set_text(label, "FAMILY HUB\nDISPLAY OK\nTOUCH TO TEST");
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(label);
  lv_obj_invalidate(scr);

  Serial.println("[lvgl] first render started");
  lvgl_handler_once("first-render-smoke");
  if (!s_first_flush_completed) {
    Serial.println("[lvgl] FAIL: first render did not complete a flush");
    return false;
  }

  // Keep this valid boot screen visible until UiManager is admitted to build
  // the first dashboard. UiManager cleans it immediately before constructing
  // its persistent object tree.
  return true;
}

bool displayBegin() {
  Serial.println("[disp] === WAVESHARE_7B bring-up: 1024x600 RGB + GT911 + LVGL ===");
  s_lock = xSemaphoreCreateRecursiveMutex();
  // The flush + lv_timer_handler run in this (the Arduino loop/setup) task; the
  // frame-finish ISR notifies it. Capture the handle before the first render.
  s_lvgl_task_handle = xTaskGetCurrentTaskHandle();

  // Stage 1: memory report
  diag_report_memory("boot");
  if (ESP.getPsramSize() == 0) {
    Serial.println("[disp] FAIL: PSRAM not detected — check memory_type=qio_opi"
                   " build config; RGB framebuffers cannot be allocated");
    return false;
  }

  // Stage 2: I2C + bus scan
  Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL, BOARD_I2C_FREQ_HZ);
  diag_i2c_scan();

  // Stage 3: board control (CH32V003) + panel power/reset + backlight
  Serial.println("[disp] stage: IO extension + panel power sequence");
  ioext_init();
  if (!panel_power_sequence()) return false;

  // Stage 4: RGB panel (init-free driver — no vendor init sequence exists)
  Serial.printf("[disp] stage: RGB panel init (PCLK %uMHz, BSP porch timings)\n",
                (unsigned)(PANEL_PCLK_HZ / 1000000));
  if (!rgb_panel_init()) return false;
  displaySetBacklight(BL_DEFAULT_PERCENT);  // preload PWM; gate stays off
  Serial.println("[disp] backlight PWM preloaded; gate held off until first frame");
  diag_report_memory("after panel init");

#if FIRST_BOOT_DIAG
  // Stage 5: solid-color test screens
  diag_test_screens();
#endif

  // Stage 6: touch
  Serial.println("[disp] stage: GT911 touch init");
  gt911_init();   // failure is non-fatal: UI still renders, writes need touch

#if FIRST_BOOT_DIAG
  diag_touch_echo(8000);
#endif

  // Stage 7: LVGL
  Serial.println("[disp] stage: LVGL init");
  lv_init();
  Serial.println("[lvgl] lv_init complete");
  // Direct mode: use the two RGB framebuffers (in PSRAM) directly as LVGL's
  // draw buffers. No separate internal-SRAM allocation — that frees the SRAM
  // the bounce buffer needs, and lets the driver flip framebuffers on VSYNC for
  // tear-free output. buffer_size is the full screen.
  diag_report_internal_heap("before LVGL framebuffer bind");
  void* fb0 = nullptr;
  void* fb1 = nullptr;
  esp_err_t fberr = esp_lcd_rgb_panel_get_frame_buffer(s_panel, 2, &fb0, &fb1);
  if (fberr != ESP_OK || !fb0 || !fb1) {
    Serial.printf("[disp] FAIL: get 2 RGB framebuffers err=0x%X fb0=%p fb1=%p\n", fberr, fb0, fb1);
    return false;
  }
  const size_t px = (size_t)PANEL_H_RES * PANEL_V_RES;   // full-screen buffers
  Serial.printf("[lvgl] direct-mode buffers = framebuffers fb0=%p fb1=%p (%u px each, PSRAM)\n",
                fb0, fb1, (unsigned)px);
  lv_disp_draw_buf_init(&s_draw_buf, fb0, fb1, px);
  Serial.println("[lvgl] draw-buffer registration complete");

  lv_disp_drv_init(&s_disp_drv);
  s_disp_drv.hor_res  = PANEL_H_RES;
  s_disp_drv.ver_res  = PANEL_V_RES;
  s_disp_drv.flush_cb = lvgl_flush;
  s_disp_drv.draw_buf = &s_draw_buf;
  s_disp_drv.full_refresh = 0;
  s_disp_drv.direct_mode  = 1;    // LVGL renders straight into the framebuffers
  lv_disp_drv_register(&s_disp_drv);
  Serial.println("[lvgl] display driver registered");

  lv_indev_drv_init(&s_indev_drv);
  s_indev_drv.type    = LV_INDEV_TYPE_POINTER;
  s_indev_drv.read_cb = lvgl_touch;
  if (s_gt911_addr) {
    lv_indev_drv_register(&s_indev_drv);
    Serial.println("[lvgl] input driver registered");
  } else {
    Serial.println("[lvgl] input driver skipped (GT911 unavailable)");
  }

  if (!lvgl_first_render_smoke()) {
    return false;
  }

  s_backlight_ready = true;
  displaySetBacklight(BL_DEFAULT_PERCENT);
  s_ready = true;
  diag_report_memory("panel ready");
  Serial.println("[disp] === panel ready ===");
  return true;
}

void displayTick() {
  if (!s_ready) return;
  displayLock();
  lvgl_handler_once("loop");
  lvgl_update_touch_marker();
  displayUnlock();
}

bool displayRenderMemoryAvailable(size_t requiredTextBytes, const char* boundary) {
  if (!s_ready) return true;

  lv_mem_monitor_t lv = {};
  displayLock();
  lv_mem_monitor(&lv);
  displayUnlock();

  const size_t free8 = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  const size_t largest8 = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  // One copy can exist in an Arduino String while LVGL reallocates its own
  // label copy. Requiring 2x aggregate free and 1x contiguous free follows
  // that concrete allocation path. LVGL is checked separately because its
  // fixed 48 KiB arena is not represented by ESP heap metrics.
  const size_t aggregateNeed = requiredTextBytes * 2U;
  const bool admitted = free8 >= aggregateNeed && largest8 >= requiredTextBytes &&
                        lv.free_size >= aggregateNeed &&
                        lv.free_biggest_size >= requiredTextBytes;
  Serial.printf("[mem] %s render_need=%u free8=%u largest8=%u "
                "lv_free=%u lv_largest=%u lv_frag=%u admitted=%s\n",
                boundary ? boundary : "render",
                (unsigned)requiredTextBytes, (unsigned)free8,
                (unsigned)largest8, (unsigned)lv.free_size,
                (unsigned)lv.free_biggest_size, (unsigned)lv.frag_pct,
                admitted ? "yes" : "no");
  return admitted;
}

void displayLock()   { if (s_lock) xSemaphoreTakeRecursive(s_lock, portMAX_DELAY); }
void displayUnlock() { if (s_lock) xSemaphoreGiveRecursive(s_lock); }

#else  // ---- non-7B builds: no-op stubs -------------------------------------

bool displayReady()                    { return false; }
bool displayBegin()                    { return false; }
void displayTick()                     {}
bool displayRenderMemoryAvailable(size_t, const char*) { return true; }
void displayLock()                     {}
void displayUnlock()                   {}
void displaySetBacklight(uint8_t)      {}

#endif
