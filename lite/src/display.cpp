#include <daisy_seed.h>
#include <lvgl.h>
#include <src/misc/lv_style_gen.h>

#include "GC9A01.hpp"

const unsigned int kWidth = 240;
const unsigned int kHeight = 240;
constexpr unsigned int kBufferSize =
    (kWidth * kHeight / 10) * (LV_COLOR_DEPTH / 8);
static uint16_t draw_buf[kBufferSize / 4] = {0};

static GC9A01 display_driver;

static lv_obj_t *led_circle[8] = {nullptr};

static lv_obj_t *arc = nullptr;
static lv_style_t bg_arc_style;
static int arc_angle = 0;

static void flush_display(lv_display_t *display, const lv_area_t *area,
                          uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  display_driver.SetDrawWindow(area->x1, area->y1, w, h);
  display_driver.DrawBitmap(px_map, w * h);
  lv_display_flush_ready(display);
}

void setup_display(daisy::SpiHandle *spi, daisy::GPIO *dc, daisy::GPIO *rst) {
  display_driver.Init(spi, dc, rst);
  // display_driver.FillScreen(COLOR_BLACK);

  lv_init();
  lv_tick_set_cb(daisy::System::GetTick);
  lv_display_t *display = lv_display_create(kWidth, kHeight);
  lv_display_set_flush_cb(display, flush_display);
  lv_display_set_buffers(display, draw_buf, NULL, sizeof(draw_buf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  // arc = lv_arc_create(lv_screen_active());
  // lv_obj_set_size(arc, 200, 200);
  // lv_arc_set_rotation(arc, 135);
  // lv_arc_set_bg_angles(arc, 0, 360);
  // lv_arc_set_range(arc, 0, 360);
  // lv_arc_set_value(arc, 10);
  // lv_obj_center(arc);

  // lv_style_init(&bg_arc_style);
  // lv_style_set_bg_opa(&bg_arc_style, LV_OPA_COVER);
  // lv_style_set_bg_color(&bg_arc_style, lv_color_hex(0x000000));
  // lv_style_set_arc_color(&bg_arc_style, lv_color_hex(0x000000));
  // lv_obj_remove_style(arc, NULL, LV_PART_MAIN);
  // lv_obj_remove_style(arc, NULL, LV_PART_INDICATOR);
  // lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
  // lv_style_set_arc_opa(&bg_arc_style, LV_OPA_0);
  // lv_style_set_arc_color(&bg_arc_style, lv_color_hex(0xFFFFFF));
  // lv_obj_add_style(arc, &bg_arc_style, 0);
  // lv_obj_add_style(arc, &bg_arc_style, LV_PART_INDICATOR);

  for (int i = 0; i < 8; i++) {
    led_circle[i] = lv_led_create(lv_screen_active());
    // lv_obj_set_size(led_circle[i], 20, 20);
    lv_led_set_brightness(led_circle[i], 255);
    lv_led_set_color(led_circle[i], lv_palette_main(LV_PALETTE_RED));
    // lv_obj_set_style_radius(led_circle[i], LV_RADIUS_CIRCLE, 0);
    // lv_obj_set_style_bg_color(led_circle[i], lv_color_hex(0xFFFFFF), 0);
    // lv_obj_set_style_border_width(led_circle[i], 0, 0);
    // lv_obj_set_style_bg_opa(led_circle[i], LV_OPA_50, 0);
    lv_obj_align(led_circle[i], LV_ALIGN_CENTER, 70 * cos(i * 3.14 / 4),
                 70 * sin(i * 3.14 / 4));
  }

  lv_led_on(led_circle[0]);
}

void display_tick() {
  // lv_arc_set_value(arc, arc_angle);
  if ((arc_angle + 1) / (360 / 8) != arc_angle / (360 / 8)) {
    int led_on = (arc_angle + 1) / (360 / 8);
    lv_led_on(led_circle[led_on % 8]);
  }

  arc_angle += 1;
  if (arc_angle >= 360) {
    for (int i = 1; i < 8; i++) {
      lv_led_off(led_circle[i]);
    }
    arc_angle = 0;
  }
  lv_timer_handler();
}
