/**
 ****************************************************************************************
 *
 * @file calendar_display_modified.h
 *
 * @brief Modified calendar display with analog clock functionality header file
 *
 ****************************************************************************************
 */

#ifndef __CALENDAR_DISPLAY_MODIFIED_H
#define __CALENDAR_DISPLAY_MODIFIED_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Vẽ trang lịch đầy đủ (phiên bản gốc, duy trì tính tương thích)
 * @param unix_time Dấu thời gian Unix hiện tại
 */
void draw_calendar_page(uint32_t unix_time, bool force_redraw);

#endif // __CALENDAR_DISPLAY_MODIFIED_H
