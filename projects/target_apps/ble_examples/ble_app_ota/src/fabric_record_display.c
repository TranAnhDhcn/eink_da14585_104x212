/**
 ****************************************************************************************
 *
 * @file fabric_record_display.c
 *
 * @brief Layout implementation for Fabric Relaxing Record display mode
 *
 ****************************************************************************************
 */

#include "fabric_record_display.h"
#include "GUI_Paint.h"
#include "EPD_2in13_V2.h"
#include "fonts.h"
#include "custom_drawing.h"
#include <stdio.h>
#include <string.h>

extern uint8_t epd_buffer[];

void draw_fabric_record_page(const fabric_record_t* record, bool force_redraw)
{
   char buf[50];

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

    // ===== 1. TITLE =====
    EPD_DrawUTF8(5, 1, 0, "FABRIC RELAXING RECORD",
        EPD_ASCII_11X16, 0, BLACK, WHITE);

    // ===== 2. LEFT COLUMN =====
    int y = 20;
    int line = 15;

    sprintf(buf, "Width: %s", record->width);
    EPD_DrawUTF8(5, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "PO: %s", record->po);
    EPD_DrawUTF8(5, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "Item: %s", record->item);
    EPD_DrawUTF8(5, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "Lot: %s", record->lot);
    EPD_DrawUTF8(5, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);

    // ===== 3. RIGHT COLUMN =====
    y = 20;

    sprintf(buf, "NV xa: %s", record->staff);
    EPD_DrawUTF8(120, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "Xa luc: %s", record->relax_date);
    EPD_DrawUTF8(120, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "OK luc: %s", record->ok_date);
    EPD_DrawUTF8(120, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "Color: %s", record->color);
    EPD_DrawUTF8(120, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "Buy: %s", record->buy);
    EPD_DrawUTF8(120, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "Roll: %s", record->roll);
    EPD_DrawUTF8(120, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);
    y += line;

    sprintf(buf, "YDS: %s", record->yds);
    EPD_DrawUTF8(120, y, 0, buf, EPD_ASCII_11X16, 0, BLACK, WHITE);

    // ===== 4. FOOTER =====
    EPD_DrawUTF8(90, 105, 0, record->note,
        EPD_ASCII_11X16, 0, BLACK, WHITE);
}
