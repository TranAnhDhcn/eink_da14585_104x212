/**
 ****************************************************************************************
 *
 * @file analog_clock.c
 *
 * @brief Analog clock display functionality for e-paper display
 *
 ****************************************************************************************
 */

#include "analog_clock.h"
#include "EPD_2in13_V2.h"
#include "GUI_Paint.h"
#include "etime.h"
#include "fonts.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void draw_clock_frame(uint16_t center_x, uint16_t center_y,
                             uint16_t radius, bool force_redraw) {
  if (!force_redraw) {
    return;
  }

  Paint_DrawCircle(center_x, center_y, radius, BLACK, DOT_PIXEL_2X2,
                   DRAW_FILL_EMPTY);

  for (uint8_t hour = 1; hour <= 12; hour++) {
    double angle = (hour * 30 - 90) * M_PI / 180.0;
    uint16_t num_x = center_x + (radius - 15) * cos(angle);
    uint16_t num_y = center_y + (radius - 15) * sin(angle);
    char hour_str[3];
    uint16_t text_x = num_x - (hour >= 10 ? 7 : 3);
    uint16_t text_y = num_y - 6;
    uint16_t tick_x = center_x + (radius - 8) * cos(angle);
    uint16_t tick_y = center_y + (radius - 8) * sin(angle);

    sprintf(hour_str, "%d", hour);
    EPD_DrawUTF8(text_x, text_y, 0, hour_str, EPD_ASCII_Font8, 0, BLACK, WHITE);
    Paint_DrawPoint(tick_x, tick_y, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
  }

  for (uint8_t minute = 0; minute < 60; minute++) {
    if (minute % 5 != 0) {
      double angle = (minute * 6 - 90) * M_PI / 180.0;
      uint16_t tick_x = center_x + (radius - 5) * cos(angle);
      uint16_t tick_y = center_y + (radius - 5) * sin(angle);
      Paint_DrawPoint(tick_x, tick_y, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    }
  }

  Paint_DrawPoint(center_x, center_y, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
}

static void clear_hands_area(uint16_t center_x, uint16_t center_y,
                             uint16_t radius) {
  Paint_DrawCircle(center_x, center_y, radius - 10, WHITE, DOT_PIXEL_1X1,
                   DRAW_FILL_FULL);
}

static void draw_clock_hands(uint16_t center_x, uint16_t center_y, uint8_t hour,
                             uint8_t minute, uint16_t radius) {
  double hour_angle = ((hour % 12) * 30 + minute * 0.5 - 90) * M_PI / 180.0;
  double minute_angle = (minute * 6 - 90) * M_PI / 180.0;
  uint16_t hour_hand_length = radius * 0.5;
  uint16_t minute_hand_length = radius * 0.7;
  uint16_t hour_end_x = center_x + hour_hand_length * cos(hour_angle);
  uint16_t hour_end_y = center_y + hour_hand_length * sin(hour_angle);
  uint16_t minute_end_x = center_x + minute_hand_length * cos(minute_angle);
  uint16_t minute_end_y = center_y + minute_hand_length * sin(minute_angle);

  Paint_DrawLine(center_x, center_y, hour_end_x, hour_end_y, BLACK,
                 DOT_PIXEL_4X4, LINE_STYLE_SOLID);
  Paint_DrawLine(center_x, center_y, minute_end_x, minute_end_y, BLACK,
                 DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  Paint_DrawPoint(center_x, center_y, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
}

void draw_analog_clock(uint16_t x, uint16_t y, uint16_t size,
                       uint32_t unix_time, bool force_redraw) {
  tm_t tm;
  transformTime(unix_time, &tm);

  uint16_t center_x = x + size / 2;
  uint16_t center_y = y + size / 2;
  uint16_t radius = size / 2 - 2;

  if (force_redraw) {
    Paint_DrawRectangle(x, y, x + size, y + size, WHITE, DOT_PIXEL_1X1,
                        DRAW_FILL_FULL);
    draw_clock_frame(center_x, center_y, radius, true);
  } else {
    clear_hands_area(center_x, center_y, radius);
  }

  draw_clock_hands(center_x, center_y, tm.tm_hour, tm.tm_min, radius);
}

extern uint8_t epd_buffer[];
void draw_calendar_with_analog_clock(uint32_t unix_time, bool force_redraw) {
  tm_t tm;
  transformTime(unix_time, &tm);

  if (force_redraw) {
    Paint_NewImage(epd_buffer, EPD_2IN13_V2_WIDTH, EPD_2IN13_V2_HEIGHT, 270,
                   WHITE);
    Paint_SelectImage(epd_buffer);
    Paint_SetMirroring(MIRROR_VERTICAL);
    Paint_Clear(WHITE);
  } else {
    Paint_SelectImage(epd_buffer);
    Paint_SetMirroring(MIRROR_VERTICAL);
    Paint_Clear(WHITE);
  }

  uint16_t year = tm.tm_year + YEAR0;
  uint8_t month = tm.tm_mon + 1;
  uint8_t current_day = tm.tm_mday;

  if (force_redraw) {
    char title_buf[20];
    const char *week_names_vi[] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
    const uint8_t days_in_month[2][12] = {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
    uint8_t x_start = 0;
    uint8_t y_pos = 20;
    uint8_t cell_width = 16;
    uint8_t y_start = 40;
    uint8_t cell_height = 12;
    uint8_t leap = is_leap(year);
    uint8_t first_day = day_of_week_get(month, 1, year);
    uint8_t days_count = days_in_month[leap][month - 1];
    char day_buf[3];
    uint8_t row = 0;
    uint8_t col = first_day;

    Paint_Clear(WHITE);

    sprintf(title_buf, "%d / %d", year, month);
    EPD_DrawUTF8(2, 0, 1, title_buf, EPD_ASCII_Font16, 0, BLACK, WHITE);

    Paint_DrawRectangle(1, 1, 113, 104, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(x_start + 1, y_pos - 2, x_start + 7 * cell_width - 1,
                        y_pos + 12, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    for (uint8_t i = 0; i < 7; i++) {
      uint8_t x_pos = x_start + i * cell_width;
      EPD_DrawUTF8(x_pos + 4, y_pos + 1, 0, week_names_vi[i], EPD_ASCII_Font8,
                   0, WHITE, BLACK);
    }

    for (uint8_t day = 1; day <= days_count; day++) {
      uint8_t x_pos = x_start + col * cell_width + 2;
      uint8_t y_day = y_start + row * cell_height + 1;

      sprintf(day_buf, "%d", day);

      if (day == current_day) {
        Paint_DrawRectangle(x_pos - 1, y_day - 1, x_pos + 13, y_day + 11, BLACK,
                            DOT_PIXEL_1X1, DRAW_FILL_FULL);
        EPD_DrawUTF8(x_pos, y_day, 0, day_buf, EPD_ASCII_Font8, 0, WHITE,
                     BLACK);
      } else {
        EPD_DrawUTF8(x_pos, y_day, 0, day_buf, EPD_ASCII_Font8, 0, BLACK,
                     WHITE);
      }

      col++;
      if (col >= 7) {
        col = 0;
        row++;
      }
    }
  }

  draw_analog_clock(117, 10, 95, unix_time, force_redraw);
}
