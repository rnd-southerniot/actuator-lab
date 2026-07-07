/*
 * Optional on-board 2.4" LCD live view for the REV Core Hex bench firmware.
 *
 * Uses the ST BSP (ILI9341 over LTDC, framebuffer in external SDRAM). Renders a text
 * panel (state/mode/pos/vel/current/bus) plus a scrolling velocity trace. All drawing
 * runs from the main loop only (BSP_LCD is not ISR-safe) and is decoupled from control:
 * a failed/absent panel just disables the view (display_ok()==0) — it never blocks boot
 * or motion safety.
 */

#include "display.h"

#include <stdio.h>
#include <string.h>

#include "stm32f429i_discovery_lcd.h"
#include "fonts.h"

static uint8_t g_lcd_ok;
static uint16_t g_w, g_h;

/* Plot geometry (filled in display_init once the panel size is known). */
static uint16_t g_plot_left, g_plot_right, g_plot_top, g_plot_bottom, g_plot_cy;
static uint16_t g_plot_x;
static uint16_t g_prev_y;
static uint8_t g_have_prev;

#define VEL_FULL_SCALE_CPS 700 /* a little above the ~600 cps mechanical max */
#define TEXT_TOP 22
#define TEXT_BOT 98

int display_ok(void) { return g_lcd_ok; }

static uint16_t vel_to_y(int32_t vel_cps) {
  int32_t half = (int32_t)(g_plot_bottom - g_plot_top) / 2 - 1;
  int32_t y = (int32_t)g_plot_cy - (vel_cps * half) / VEL_FULL_SCALE_CPS;
  if (y < (int32_t)(g_plot_top + 1U)) {
    y = (int32_t)(g_plot_top + 1U);
  } else if (y > (int32_t)(g_plot_bottom - 1U)) {
    y = (int32_t)(g_plot_bottom - 1U);
  }
  return (uint16_t)y;
}

void display_init(void) {
  g_lcd_ok = 0;
  if (BSP_LCD_Init() != LCD_OK) {
    return; /* optional view — leave disabled */
  }
  BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, LCD_FRAME_BUFFER);
  BSP_LCD_SelectLayer(LCD_FOREGROUND_LAYER);
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_BLACK);

  g_w = (uint16_t)BSP_LCD_GetXSize();
  g_h = (uint16_t)BSP_LCD_GetYSize();

  g_plot_left = 2;
  g_plot_right = (uint16_t)(g_w - 3U);
  g_plot_top = 110;
  g_plot_bottom = (uint16_t)(g_h - 6U);
  g_plot_cy = (uint16_t)((g_plot_top + g_plot_bottom) / 2U);
  g_plot_x = (uint16_t)(g_plot_left + 1U);
  g_have_prev = 0;

  /* Title */
  BSP_LCD_SetFont(&Font16);
  BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
  BSP_LCD_DisplayStringAt(0, 2, (uint8_t *)"REV CORE HEX", CENTER_MODE);

  /* Plot frame + zero line */
  BSP_LCD_SetTextColor(LCD_COLOR_DARKGRAY);
  BSP_LCD_DrawRect(g_plot_left, g_plot_top, (uint16_t)(g_plot_right - g_plot_left),
                   (uint16_t)(g_plot_bottom - g_plot_top));
  BSP_LCD_DrawHLine(g_plot_left, g_plot_cy, (uint16_t)(g_plot_right - g_plot_left));
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
  BSP_LCD_DisplayStringAt(2, (uint16_t)(g_plot_top + 1U), (uint8_t *)"vel", LEFT_MODE);

  g_lcd_ok = 1;
}

static uint32_t state_color(const char *state) {
  if (strcmp(state, "ARMED") == 0) {
    return LCD_COLOR_GREEN;
  }
  if (strcmp(state, "FAULT") == 0) {
    return LCD_COLOR_RED;
  }
  return LCD_COLOR_WHITE;
}

void display_update(const display_data_t *d) {
  if (!g_lcd_ok || d == NULL) {
    return;
  }

  char line[40];

  /* --- text panel: erase then redraw --- */
  BSP_LCD_SetFont(&Font12);
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_FillRect(0, TEXT_TOP, g_w, (uint16_t)(TEXT_BOT - TEXT_TOP));

  BSP_LCD_SetTextColor(state_color(d->state));
  snprintf(line, sizeof(line), "ST:%s  M:%s", d->state, d->mode);
  BSP_LCD_DisplayStringAt(2, 24, (uint8_t *)line, LEFT_MODE);

  BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
  snprintf(line, sizeof(line), "pos:%ld  vel:%ld cps", (long)d->pos, (long)d->vel_cps);
  BSP_LCD_DisplayStringAt(2, 38, (uint8_t *)line, LEFT_MODE);

  snprintf(line, sizeof(line), "I:%ld mA  V:%ld mV", (long)d->cur_ma, (long)d->bus_mv);
  BSP_LCD_DisplayStringAt(2, 52, (uint8_t *)line, LEFT_MODE);

  snprintf(line, sizeof(line), "duty:%ld  INA:%s", (long)d->duty, d->ina_ok ? "ok" : "--");
  BSP_LCD_DisplayStringAt(2, 66, (uint8_t *)line, LEFT_MODE);

  BSP_LCD_SetTextColor(d->fs_ok ? LCD_COLOR_GREEN : LCD_COLOR_RED);
  snprintf(line, sizeof(line), "FS:%s", d->fs_ok ? "OK" : "FAULT");
  BSP_LCD_DisplayStringAt(2, 80, (uint8_t *)line, LEFT_MODE);

  /* --- scrolling velocity trace --- */
  uint16_t y = vel_to_y(d->vel_cps);

  /* erase the current column inside the frame, then restore the zero line pixel */
  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_DrawVLine(g_plot_x, (uint16_t)(g_plot_top + 1U), (uint16_t)(g_plot_bottom - g_plot_top - 1U));
  BSP_LCD_SetTextColor(LCD_COLOR_DARKGRAY);
  BSP_LCD_DrawPixel(g_plot_x, g_plot_cy, LCD_COLOR_DARKGRAY);

  BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
  if (g_have_prev) {
    BSP_LCD_DrawLine((uint16_t)(g_plot_x - 1U), g_prev_y, g_plot_x, y);
  } else {
    BSP_LCD_DrawPixel(g_plot_x, y, LCD_COLOR_GREEN);
  }
  g_prev_y = y;
  g_have_prev = 1;

  g_plot_x++;
  if (g_plot_x >= g_plot_right) {
    g_plot_x = (uint16_t)(g_plot_left + 1U);
    g_have_prev = 0;
  }
}
