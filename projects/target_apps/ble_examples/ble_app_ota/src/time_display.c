/**
 ****************************************************************************************
 *
 * @file time_display.c
 *
 * @brief Digital time display component implementation
 *
 ****************************************************************************************
 */

#include "time_display.h"
#include "EPD_2in13_V2.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "custom_drawing.h"
#include "etime.h"
#include "fonts.h"
#include "lunar.h"
#include <stdio.h>
#include <time.h>

// External buffer defined in user_custs1_impl.c
extern uint8_t epd_buffer[];

static void draw_small_dot(uint16_t x, uint16_t y) {
  Paint_SetPixel(x, y, BLACK);
  Paint_SetPixel(x + 1, y, BLACK);
  Paint_SetPixel(x, y + 1, BLACK);
  Paint_SetPixel(x + 1, y + 1, BLACK);
}

static void draw_lunar_label(uint16_t x, uint16_t y) {
  y -= 3;
  EPD_DrawUTF8(x, y, 0, "Am lich:", EPD_ASCII_Font16, 0, BLACK, WHITE);

  // Circumflex for 'A' in "Am"
  Paint_SetPixel(x + 3, y + 2, BLACK);
  Paint_SetPixel(x + 4, y + 1, BLACK);
  Paint_SetPixel(x + 5, y + 0, BLACK);
  Paint_SetPixel(x + 6, y + 1, BLACK);
  Paint_SetPixel(x + 7, y + 2, BLACK);

  // Dot below 'i' in "lich"
  draw_small_dot(x + 48, y + 16);
}

/**
 * @brief Draw the digital time page (includes date, week, image, and lunar
 * date)
 * @param unix_time Current Unix timestamp
 * @param force_redraw Whether to force a full redraw of the screen
 */
void draw_time_page(uint32_t unix_time, bool force_redraw) {
  tm_t tm;
  transformTime(unix_time, &tm);

  char buf[50];
  char buf2[50];

  // Coordinates and size for the illustration image
  const UWORD img_x = 140;
  const UWORD img_y = 20;
  const UWORD img_w = 70;
  const UWORD img_h = 70;

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
	
	  // 1. Draw Day of Week
  sprintf(buf, "%s", WEEK_VN[tm.tm_wday]);
	EPD_DrawUTF8(5, 1, 0, buf, EPD_ASCII_Font16, 0, BLACK, WHITE);

  // 2. Draw Date (DD-MM-YYYY)
  sprintf(buf, "%d-%02d-%02d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + YEAR0);
  EPD_DrawUTF8(60, 1, 1, buf, EPD_ASCII_Font16, EPD_FontUTF8_16x16, BLACK,
               WHITE);

  // 3. Draw Digital Time
  {
    struct tm display_time;
    display_time.tm_hour = tm.tm_hour;
    display_time.tm_min = tm.tm_min;
    Draw_Time_String(5, 30, &display_time, 25, 50, BLACK);
  }

  // 4. Draw Illustration Image based on hour
  const unsigned char *image_to_display = NULL;
  if (tm.tm_hour < 7 || tm.tm_hour >= 22) {
    image_to_display = gImage_Sleep;
  } else if (tm.tm_hour < 8) {
    image_to_display = gImage_breakfast;
  } else if (tm.tm_hour < 11) {
    image_to_display = gImage_Morning;
  } else if (tm.tm_hour < 13) {
    image_to_display = gImage_Lunch;
  } else if (tm.tm_hour < 14) {
    image_to_display = gImage_rest;
  } else if (tm.tm_hour < 19) {
    image_to_display = gImage_Work;
  } else if (tm.tm_hour < 20) {
    image_to_display = gImage_dinner;
  } else {
    image_to_display = gImage_rest;
  }

  if (image_to_display != NULL) {
    Paint_DrawImage(image_to_display, img_x, img_y, img_w, img_h, BLACK, WHITE);
  }

  // 5. Draw Lunar Date
  struct Lunar_Date lunar;
  LUNAR_SolarToLunar(&lunar, tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);
  if (lunar.IsLeap) {
    sprintf(buf2, " %02d-%02d-%04d (N)", lunar.Date, lunar.Month, lunar.Year);
  } else {
    sprintf(buf2, " %02d-%02d-%04d", lunar.Date, lunar.Month, lunar.Year);
  }
  draw_lunar_label(0, 90);
  EPD_DrawUTF8(84, 88, 0, buf2, EPD_ASCII_Font16, 0, BLACK, WHITE);
}
