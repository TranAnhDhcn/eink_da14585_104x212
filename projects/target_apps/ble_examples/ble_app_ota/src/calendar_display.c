#include "calendar_display.h"
#include "EPD_2in13_V2.h"
#include "analog_clock.h"
#include "GUI_Paint.h"
#include "etime.h"
#include "fonts.h"
#include <stdio.h>
extern uint8_t epd_buffer[];
// Bảng số ngày trong tháng
static const uint8_t days_in_month[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static uint8_t get_first_day_of_month(uint16_t year, uint8_t month) {
    return day_of_week_get(month, 1, year);
}

static uint8_t get_days_in_month(uint16_t year, uint8_t month) {
    uint8_t leap = is_leap(year);
    return days_in_month[leap][month - 1];
}

/**
 * @brief Vẽ khung và tiêu đề các thứ
 */
static void draw_layout_and_header(void) {
    // Vẽ viền ngoài
    Paint_DrawRectangle(1, 1, 212, 104, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    // Vẽ đường kẻ dọc (x=125)
    Paint_DrawLine(125, 1, 125, 104, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    // Vẽ nền đen cho tiêu đề thứ
    Paint_DrawRectangle(2, 2, 124, 18, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    const char* weeks[] = {"CN", "T2", "T3", "T4", "T5", "T6", "T7"};
    for (uint8_t i = 0; i < 7; i++) {
        // Căn chỉnh tọa độ x để các thứ nằm đều nhau
        EPD_DrawUTF8(5 + (i * 17), 4, 0, weeks[i], EPD_ASCII_7X12, 0, WHITE, BLACK);
    }
}

/**
 * @brief Vẽ lưới ngày tháng (Sửa lỗi không hiện ngày)
 */
static void draw_calendar_grid(uint16_t year, uint8_t month, uint8_t current_day) {
    uint8_t first_day = get_first_day_of_month(year, month);
    uint8_t days_count = get_days_in_month(year, month);
    
    uint8_t x_start = 5;
    uint8_t y_start = 22;
    uint8_t cell_w = 17;
    uint8_t cell_h = 13;
    
    char day_buf[3];
    uint8_t row = 0;
    uint8_t col = first_day;

    for (uint8_t day = 1; day <= days_count; day++) {
        uint8_t x_pos = x_start + (col * cell_w);
        uint8_t y_pos = y_start + (row * cell_h);
        
        sprintf(day_buf, "%d", day);
        
        if (day == current_day) {
            // Vẽ ô vuông đen cho ngày hiện tại
            Paint_DrawRectangle(x_pos - 1, y_pos - 1, x_pos + 14, y_pos + 11, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
            EPD_DrawUTF8(x_pos + 1, y_pos, 0, day_buf, EPD_ASCII_7X12, 0, WHITE, BLACK);
        } else {
            EPD_DrawUTF8(x_pos + 1, y_pos, 0, day_buf, EPD_ASCII_7X12, 0, BLACK, WHITE);
        }
        
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }
}

/**
 * @brief Vẽ nội dung bên phải (Sửa lỗi không hiện tháng, chuyển giờ sang phải)
 */
static void draw_right_panel(tm_t *tm) {
    char buf[20];
    
    // 1. Hiển thị Năm - Tháng (Tọa độ 130, 2)
    sprintf(buf, "%d-%02d", tm->tm_year + YEAR0, tm->tm_mon + 1);
    EPD_DrawUTF8(130, 2, 0, buf, EPD_ASCII_7X12, 0, BLACK, WHITE);

    
    // 2. Hiển thị Ngày lớn (Căn giữa ô bên phải)
    sprintf(buf, "%02d", tm->tm_mday);
    // WHITE là màu nền, BLACK là màu chữ. 
    // Màn hình bên phải từ X=125 đến 212 (rộng 87px). Chữ Font24 ~ 34px. 
    // Đặt X=151 để căn giữa.
		EPD_DrawUTF8(151, 22, 0, buf, EPD_ASCII_7X12, 0, BLACK, WHITE);

    // 3. Hiển thị Giờ số (Đã chuyển sang phải - Tọa độ 140, 72)
    sprintf(buf, "%02d:%02d", tm->tm_hour, tm->tm_min);
    EPD_DrawUTF8(140, 72, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    
    // 4. Nhiệt độ (Tọa độ 192, 80)
    EPD_DrawUTF8(192, 80, 0, "29C", EPD_ASCII_7X12, 0, BLACK, WHITE);
    
    // 5. Năm âm lịch (Tọa độ 126, 87)
    EPD_DrawUTF8(126, 87, 0, "Binh Ngo", EPD_ASCII_7X12, 0, BLACK, WHITE);
}

void draw_calendar_page(uint32_t unix_time, bool force_redraw) {
    tm_t tm;
    transformTime(unix_time, &tm);

    if (force_redraw)
    {
        Paint_NewImage(epd_buffer, EPD_2IN13_V2_WIDTH, EPD_2IN13_V2_HEIGHT, 270, WHITE);
        Paint_SelectImage(epd_buffer);
        Paint_SetMirroring(MIRROR_VERTICAL);
        Paint_Clear(WHITE);
    }
    else
    {
        Paint_SelectImage(epd_buffer);
        Paint_SetMirroring(MIRROR_VERTICAL);
        Paint_Clear(WHITE);
    }
    
    // Vẽ khung và tiêu đề thứ
    draw_layout_and_header();
    
    // Vẽ lịch ngày tháng bên trái
    draw_calendar_grid(tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);
    
    // Vẽ thông tin chi tiết bên phải
    draw_right_panel(&tm);
    
    // Thông tin bổ sung góc dưới trái (Ví dụ: Số ngày đếm ngược)
    EPD_DrawUTF8(1, 87, 0, "GKD: 333 ngay", EPD_ASCII_7X12, 0, BLACK, WHITE);
}