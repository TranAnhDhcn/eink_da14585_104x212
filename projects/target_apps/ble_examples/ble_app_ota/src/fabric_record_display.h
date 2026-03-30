/**
 ****************************************************************************************
 *
 * @file fabric_record_display.h
 *
 * @brief Component for displaying the Fabric Relaxing Record layout
 *
 ****************************************************************************************
 */

#ifndef _FABRIC_RECORD_DISPLAY_H_
#define _FABRIC_RECORD_DISPLAY_H_

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief Data structure holding the fabric record information
 */
typedef struct {
  char width[20]; // thay kho_vai
  char staff[20]; // thay nv_xa
  char po[20];
  char relax_date[20]; // thay ngay_xa
  char ok_date[20];    // thay ngay_ok
  char item[30];
  char color[20];
  char lot[20]; // tăng size
  char buy[10];
  char roll[10]; // thay qty_roll
  char yds[10];
  char note[20];
} fabric_record_t;

/**
 * @brief Draw the fabric relaxing record page
 * @param record Pointer to the struct containing the record data
 * @param force_redraw Whether to force a full redraw of the screen
 */
void draw_fabric_record_page(const fabric_record_t *record, bool force_redraw);

#endif // _FABRIC_RECORD_DISPLAY_H_
