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
#include "Fonts/fonts.h"
#include "GUI_Paint.h"
#include "etime.h"
#include "fonts.h"
#include <math.h>

// Hằng số toán học
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern uint8_t epd_buffer[];

/**
 * @brief Vẽ khung đồng hồ và các vạch chia
 * @param center_x Tọa độ X tâm đồng hồ
 * @param center_y Tọa độ Y tâm đồng hồ
 * @param radius Bán kính đồng hồ
 * @param force_redraw Có bắt buộc vẽ lại tất cả các phần tử hay không
 */
static void draw_clock_frame(uint16_t center_x, uint16_t center_y,
                             uint16_t radius, bool force_redraw) {
  if (!force_redraw)
    return; // Bỏ qua vẽ khung khi không bắt buộc vẽ lại

  // Vẽ khung tròn bên ngoài
  Paint_DrawCircle(center_x, center_y, radius, BLACK, DOT_PIXEL_2X2,
                   DRAW_FILL_EMPTY);

  // Vẽ 12 số giờ và các điểm vạch chia
  for (uint8_t hour = 1; hour <= 12; hour++) {
    double angle = (hour * 30 - 90) * M_PI /
                   180.0; // 12 giờ là 0 độ, theo chiều kim đồng hồ

    // Tính toán vị trí con số (hơi lùi vào trong)
    uint16_t num_x = center_x + (radius - 15) * cos(angle);
    uint16_t num_y = center_y + (radius - 15) * sin(angle);

    // Vẽ con số giờ
    char hour_str[3];
    sprintf(hour_str, "%d", hour);

    // Điều chỉnh vị trí hiển thị theo vị trí con số để căn giữa
    uint16_t text_x = num_x - (hour >= 10 ? 7 : 3);
    uint16_t text_y = num_y - 6;

    EPD_DrawUTF8(text_x, text_y, 0, hour_str, EPD_ASCII_Font8, 0, BLACK, WHITE);

    // Vẽ điểm vạch chia giờ (mặt ngoài của con số)
    uint16_t tick_x = center_x + (radius - 8) * cos(angle);
    uint16_t tick_y = center_y + (radius - 8) * sin(angle);
    Paint_DrawPoint(tick_x, tick_y, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
  }

  // Vẽ điểm vạch chia phút (điểm nhỏ)
  for (uint8_t minute = 0; minute < 60; minute++) {
    if (minute % 5 != 0) // Bỏ qua vị trí vạch chia giờ
    {
      double angle = (minute * 6 - 90) * M_PI / 180.0;
      uint16_t tick_x = center_x + (radius - 5) * cos(angle);
      uint16_t tick_y = center_y + (radius - 5) * sin(angle);
      Paint_DrawPoint(tick_x, tick_y, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    }
  }

  // Vẽ điểm tâm
  Paint_DrawPoint(center_x, center_y, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
}

/**
 * @brief Xóa vùng kim (dùng để xóa kim cũ trước khi làm mới cục bộ)
 * @param center_x Tọa độ X tâm đồng hồ
 * @param center_y Tọa độ Y tâm đồng hồ
 * @param radius Bán kính đồng hồ
 */
static void clear_hands_area(uint16_t center_x, uint16_t center_y,
                             uint16_t radius) {
  // Xóa vùng hình tròn mà kim có thể đi qua (không bao gồm khung ngoài)
  Paint_DrawCircle(center_x, center_y, radius - 10, WHITE, DOT_PIXEL_1X1,
                   DRAW_FILL_FULL);
}

/**
 * @brief Vẽ các kim đồng hồ
 * @param center_x Tọa độ X tâm đồng hồ
 * @param center_y Tọa độ Y tâm đồng hồ
 * @param hour Giờ (0-23)
 * @param minute Phút (0-59)
 * @param radius Bán kính đồng hồ
 */
static void draw_clock_hands(uint16_t center_x, uint16_t center_y, uint8_t hour,
                             uint8_t minute, uint16_t radius) {
  // Tính toán góc của kim (12 giờ là 0 độ, chiều kim đồng hồ là dương)
  double hour_angle = ((hour % 12) * 30 + minute * 0.5 - 90) * M_PI / 180.0;
  double minute_angle = (minute * 6 - 90) * M_PI / 180.0;

  // Độ dài các kim
  uint16_t hour_hand_length = radius * 0.5;   // Kim giờ dài bằng 50% bán kính
  uint16_t minute_hand_length = radius * 0.7; // Kim phút dài bằng 70% bán kính

  // Tính điểm cuối kim giờ
  uint16_t hour_end_x = center_x + hour_hand_length * cos(hour_angle);
  uint16_t hour_end_y = center_y + hour_hand_length * sin(hour_angle);

  // Tính điểm cuối kim phút
  uint16_t minute_end_x = center_x + minute_hand_length * cos(minute_angle);
  uint16_t minute_end_y = center_y + minute_hand_length * sin(minute_angle);

  // Vẽ kim giờ (dày hơn)
  Paint_DrawLine(center_x, center_y, hour_end_x, hour_end_y, BLACK,
                 DOT_PIXEL_4X4, LINE_STYLE_SOLID);

  // Vẽ kim phút (mỏng hơn)
  Paint_DrawLine(center_x, center_y, minute_end_x, minute_end_y, BLACK,
                 DOT_PIXEL_2X2, LINE_STYLE_SOLID);

  // Vẽ lại điểm tâm (đảm bảo nằm trên các kim)
  Paint_DrawPoint(center_x, center_y, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
}

/**
 * @brief Vẽ đồng hồ kim
 * @param x Tọa độ X góc trên bên trái đồng hồ
 * @param y Tọa độ Y góc trên bên trái đồng hồ
 * @param size Độ dài cạnh hình vuông của đồng hồ
 * @param unix_time Dấu thời gian Unix hiện tại
 * @param force_redraw Có bắt buộc vẽ lại tất cả các phần tử hay không (true: vẽ
 * lại hoàn toàn, false: chỉ cập nhật các kim)
 */
void draw_analog_clock(uint16_t x, uint16_t y, uint16_t size,
                       uint32_t unix_time, bool force_redraw) {
  tm_t tm;
  transformTime(unix_time, &tm);

  // Tính toán tâm và bán kính đồng hồ
  uint16_t center_x = x + size / 2;
  uint16_t center_y = y + size / 2;
  uint16_t radius = size / 2 - 2; // Để lại lề

  if (force_redraw) {
    // Bắt buộc vẽ lại: xóa toàn bộ vùng đồng hồ
    Paint_DrawRectangle(x, y, x + size, y + size, WHITE, DOT_PIXEL_1X1,
                        DRAW_FILL_FULL);

    // Vẽ khung đồng hồ và các con số
    draw_clock_frame(center_x, center_y, radius, true);
  } else {
    // Làm mới cục bộ: chỉ xóa vùng các kim
    clear_hands_area(center_x, center_y, radius);
  }

  // Vẽ các kim
  draw_clock_hands(center_x, center_y, tm.tm_hour, tm.tm_min, radius);
}

/**
 * @brief Vẽ trang lịch kèm đồng hồ kim
 * @param unix_time Dấu thời gian Unix hiện tại
 * @param force_redraw Có bắt buộc vẽ lại đồng hồ hay không
 */
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
    // Xóa canvas khi vẽ lại hoàn toàn
    Paint_Clear(WHITE);

    // Vẽ tiêu đề lịch
    char title_buf[20];
    sprintf(title_buf, "%d / %d", year, month);
    EPD_DrawUTF8(2, 0, 1, title_buf, EPD_ASCII_Font16, 0, BLACK, WHITE);

    // Vẽ hàng tiêu đề các thứ trong tuần
    const char *week_names_vi[] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
    uint8_t x_start = 0;
    uint8_t y_pos = 20;
    uint8_t cell_width = 16;

    for (uint8_t i = 0; i < 7; i++) {
      uint8_t x_pos = x_start + i * cell_width;
      EPD_DrawUTF8(x_pos + 2, y_pos, 0, week_names_vi[i], EPD_ASCII_Font8, 0,
                   BLACK, WHITE);
    }

    // Vẽ lưới lịch
    uint8_t y_start = 40;
    uint8_t cell_height = 12;
    uint8_t grid_width = 7 * cell_width;
    uint8_t grid_height = 7 * cell_height;

    // Vẽ các đường nằm ngang
    for (uint8_t i = 0; i <= 6; i++) {
      uint8_t y = y_start + i * cell_height;
      Paint_DrawLine(x_start, y, x_start + grid_width, y, BLACK, DOT_PIXEL_1X1,
                     LINE_STYLE_SOLID);
    }

    // Vẽ các đường thẳng đứng
    for (uint8_t i = 0; i <= 7; i++) {
      uint8_t x = x_start + i * cell_width;
      Paint_DrawLine(x, y_start, x, y_start + grid_height, BLACK, DOT_PIXEL_1X1,
                     LINE_STYLE_SOLID);
    }

    // Vẽ các con số ngày
    const uint8_t days_in_month[2][12] = {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, // Năm thường
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}  // Năm nhuận
    };

    uint8_t leap = is_leap(year);
    uint8_t first_day = day_of_week_get(month, 1, year);
    uint8_t days_count = days_in_month[leap][month - 1];

    char day_buf[3];
    uint8_t row = 0;
    uint8_t col = first_day;

    for (uint8_t day = 1; day <= days_count; day++) {
      uint8_t x_pos = x_start + col * cell_width + 2;
      uint8_t y_pos = y_start + row * cell_height + 2;

      sprintf(day_buf, "%d", day);

      if (day == current_day) {
        // Hiển thị nghịch màu cho ngày hiện tại
        Paint_DrawRectangle(x_start + col * cell_width,
                            y_start + row * cell_height + 3,
                            x_start + (col + 1) * cell_width - 9,
                            y_start + (row + 1) * cell_height + 3, BLACK,
                            DOT_PIXEL_1X1, DRAW_FILL_FULL);
        EPD_DrawUTF8(x_pos, y_pos, 0, day_buf, EPD_ASCII_Font8, 0, WHITE,
                     BLACK);
      } else {
        EPD_DrawUTF8(x_pos, y_pos, 0, day_buf, EPD_ASCII_Font8, 0, BLACK,
                     WHITE);
      }

      col++;
      if (col >= 7) {
        col = 0;
        row++;
      }
    }
  }

  // Vẽ đồng hồ kim ở góc dưới bên phải (120x120 pixel)
  uint16_t clock_x = 115; // Vị trí phù hợp cho màn hình rộng 290
  uint16_t clock_y = 10;  // Vị trí căn giữa theo chiều dọc
  uint16_t clock_size = 95;

  draw_analog_clock(clock_x, clock_y, clock_size, unix_time, force_redraw);
}
