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
#include "EPD_2in13_V2.h"
#include "Fonts/fonts.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>

extern uint8_t epd_buffer[];

static const char fabric_title_utf8[] = "TH\xE1\xBA\xBA KHO V\xE1\xBA\xA2I";

enum { VI_CELL_W = 8, VI_CELL_H = 14, VI_BASE_Y = 2 };
enum { VI_CONTENT_DIACRITIC_SHIFT_Y = 2 };

typedef enum {
  VI_MARK_NONE = 0,
  VI_MARK_BREVE,
  VI_MARK_CIRCUMFLEX,
  VI_MARK_HORN,
  VI_MARK_BAR,
} vi_mark_t;

typedef enum {
  VI_TONE_NONE = 0,
  VI_TONE_GRAVE,
  VI_TONE_ACUTE,
  VI_TONE_HOOK,
  VI_TONE_TILDE,
  VI_TONE_DOT,
} vi_tone_t;

typedef struct {
  char base;
  vi_mark_t mark;
  vi_tone_t tone;
} vi_glyph_t;

static uint32_t vi_decode_utf8(const char **str) {
  const unsigned char *s = (const unsigned char *)*str;

  if (s[0] < 0x80) {
    *str += 1;
    return s[0];
  }
  if ((s[0] & 0xE0) == 0xC0) {
    *str += 2;
    return ((uint32_t)(s[0] & 0x1F) << 6) | (uint32_t)(s[1] & 0x3F);
  }
  if ((s[0] & 0xF0) == 0xE0) {
    *str += 3;
    return ((uint32_t)(s[0] & 0x0F) << 12) | ((uint32_t)(s[1] & 0x3F) << 6) |
           (uint32_t)(s[2] & 0x3F);
  }

  *str += 1;
  return '?';
}

static vi_tone_t vi_tone_from_idx(uint8_t idx) {
  switch (idx) {
  case 0:
    return VI_TONE_GRAVE;
  case 1:
    return VI_TONE_ACUTE;
  case 2:
    return VI_TONE_HOOK;
  case 3:
    return VI_TONE_TILDE;
  case 4:
    return VI_TONE_DOT;
  default:
    return VI_TONE_NONE;
  }
}

static int vi_map_codepoint(uint32_t cp, vi_glyph_t *glyph) {
  glyph->base = '?';
  glyph->mark = VI_MARK_NONE;
  glyph->tone = VI_TONE_NONE;

  if (cp < 0x80) {
    glyph->base = (char)cp;
    return 1;
  }

  switch (cp) {
  case 0x00C0:
    glyph->base = 'A';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00C1:
    glyph->base = 'A';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EA2:
    glyph->base = 'A';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x00C3:
    glyph->base = 'A';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1EA0:
    glyph->base = 'A';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x0102:
    glyph->base = 'A';
    glyph->mark = VI_MARK_BREVE;
    return 1;
  case 0x00C2:
    glyph->base = 'A';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    return 1;
  case 0x1EAE:
    glyph->base = 'A';
    glyph->mark = VI_MARK_BREVE;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EA4:
    glyph->base = 'A';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x00E0:
    glyph->base = 'a';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00E1:
    glyph->base = 'a';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EA3:
    glyph->base = 'a';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x00E3:
    glyph->base = 'a';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1EA1:
    glyph->base = 'a';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x0103:
    glyph->base = 'a';
    glyph->mark = VI_MARK_BREVE;
    return 1;
  case 0x00E2:
    glyph->base = 'a';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    return 1;
  case 0x1EAF:
    glyph->base = 'a';
    glyph->mark = VI_MARK_BREVE;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EA5:
    glyph->base = 'a';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x0110:
    glyph->base = 'D';
    glyph->mark = VI_MARK_BAR;
    return 1;
  case 0x0111:
    glyph->base = 'd';
    glyph->mark = VI_MARK_BAR;
    return 1;
  case 0x00C8:
    glyph->base = 'E';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00C9:
    glyph->base = 'E';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EBA:
    glyph->base = 'E';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x1EBC:
    glyph->base = 'E';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1EB8:
    glyph->base = 'E';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x00CA:
    glyph->base = 'E';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    return 1;
  case 0x1EBE:
    glyph->base = 'E';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x00E8:
    glyph->base = 'e';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00E9:
    glyph->base = 'e';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EBB:
    glyph->base = 'e';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x1EBD:
    glyph->base = 'e';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1EB9:
    glyph->base = 'e';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x00EA:
    glyph->base = 'e';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    return 1;
  case 0x1EBF:
    glyph->base = 'e';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x00CC:
    glyph->base = 'I';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00CD:
    glyph->base = 'I';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EC8:
    glyph->base = 'I';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x0128:
    glyph->base = 'I';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1ECA:
    glyph->base = 'I';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x00EC:
    glyph->base = 'i';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00ED:
    glyph->base = 'i';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EC9:
    glyph->base = 'i';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x0129:
    glyph->base = 'i';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1ECB:
    glyph->base = 'i';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x00D2:
    glyph->base = 'O';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00D3:
    glyph->base = 'O';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1ECE:
    glyph->base = 'O';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x00D5:
    glyph->base = 'O';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1ECC:
    glyph->base = 'O';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x00D4:
    glyph->base = 'O';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    return 1;
  case 0x01A0:
    glyph->base = 'O';
    glyph->mark = VI_MARK_HORN;
    return 1;
  case 0x1ED0:
    glyph->base = 'O';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EDA:
    glyph->base = 'O';
    glyph->mark = VI_MARK_HORN;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x00F2:
    glyph->base = 'o';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00F3:
    glyph->base = 'o';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1ECF:
    glyph->base = 'o';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x00F5:
    glyph->base = 'o';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1ECD:
    glyph->base = 'o';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x00F4:
    glyph->base = 'o';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    return 1;
  case 0x01A1:
    glyph->base = 'o';
    glyph->mark = VI_MARK_HORN;
    return 1;
  case 0x1ED1:
    glyph->base = 'o';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EDB:
    glyph->base = 'o';
    glyph->mark = VI_MARK_HORN;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x00D9:
    glyph->base = 'U';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00DA:
    glyph->base = 'U';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EE6:
    glyph->base = 'U';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x0168:
    glyph->base = 'U';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1EE4:
    glyph->base = 'U';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x01AF:
    glyph->base = 'U';
    glyph->mark = VI_MARK_HORN;
    return 1;
  case 0x1EE8:
    glyph->base = 'U';
    glyph->mark = VI_MARK_HORN;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x00F9:
    glyph->base = 'u';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00FA:
    glyph->base = 'u';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EE7:
    glyph->base = 'u';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x0169:
    glyph->base = 'u';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1EE5:
    glyph->base = 'u';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x01B0:
    glyph->base = 'u';
    glyph->mark = VI_MARK_HORN;
    return 1;
  case 0x1EE9:
    glyph->base = 'u';
    glyph->mark = VI_MARK_HORN;
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EF2:
    glyph->base = 'Y';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00DD:
    glyph->base = 'Y';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EF6:
    glyph->base = 'Y';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x1EF8:
    glyph->base = 'Y';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1EF4:
    glyph->base = 'Y';
    glyph->tone = VI_TONE_DOT;
    return 1;
  case 0x1EF3:
    glyph->base = 'y';
    glyph->tone = VI_TONE_GRAVE;
    return 1;
  case 0x00FD:
    glyph->base = 'y';
    glyph->tone = VI_TONE_ACUTE;
    return 1;
  case 0x1EF7:
    glyph->base = 'y';
    glyph->tone = VI_TONE_HOOK;
    return 1;
  case 0x1EF9:
    glyph->base = 'y';
    glyph->tone = VI_TONE_TILDE;
    return 1;
  case 0x1EF5:
    glyph->base = 'y';
    glyph->tone = VI_TONE_DOT;
    return 1;
  default:
    break;
  }

  if (cp >= 0x1EB0 && cp <= 0x1EB7) {
    glyph->base = (cp & 1) ? 'a' : 'A';
    glyph->mark = VI_MARK_BREVE;
    glyph->tone = vi_tone_from_idx(
        (uint8_t)((cp - (glyph->base == 'A' ? 0x1EB0 : 0x1EB1)) / 2));
    return 1;
  }
  if (cp >= 0x1EA6 && cp <= 0x1EAD) {
    glyph->base = (cp & 1) ? 'a' : 'A';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = vi_tone_from_idx(
        (uint8_t)((cp - (glyph->base == 'A' ? 0x1EA6 : 0x1EA7)) / 2));
    return 1;
  }
  if (cp >= 0x1EC0 && cp <= 0x1EC7) {
    glyph->base = (cp & 1) ? 'e' : 'E';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = vi_tone_from_idx(
        (uint8_t)((cp - (glyph->base == 'E' ? 0x1EC0 : 0x1EC1)) / 2));
    return 1;
  }
  if (cp >= 0x1ED2 && cp <= 0x1ED9) {
    glyph->base = (cp & 1) ? 'o' : 'O';
    glyph->mark = VI_MARK_CIRCUMFLEX;
    glyph->tone = vi_tone_from_idx(
        (uint8_t)((cp - (glyph->base == 'O' ? 0x1ED2 : 0x1ED3)) / 2));
    return 1;
  }
  if (cp >= 0x1EDC && cp <= 0x1EE3) {
    glyph->base = (cp & 1) ? 'o' : 'O';
    glyph->mark = VI_MARK_HORN;
    glyph->tone = vi_tone_from_idx(
        (uint8_t)((cp - (glyph->base == 'O' ? 0x1EDC : 0x1EDD)) / 2));
    return 1;
  }
  if (cp >= 0x1EEA && cp <= 0x1EF1) {
    glyph->base = (cp & 1) ? 'u' : 'U';
    glyph->mark = VI_MARK_HORN;
    glyph->tone = vi_tone_from_idx(
        (uint8_t)((cp - (glyph->base == 'U' ? 0x1EEA : 0x1EEB)) / 2));
    return 1;
  }

  return 0;
}

static void vi_draw_dot(uint16_t x, uint16_t y) {
  Paint_SetPixel(x, y, BLACK);
  Paint_SetPixel(x + 1, y, BLACK);
}

static uint16_t vi_diacritic_shift_y(uint16_t y) {
  return (y <= 10) ? 0 : VI_CONTENT_DIACRITIC_SHIFT_Y;
}

static void vi_draw_mark(uint16_t x, uint16_t y, vi_mark_t mark) {
  uint16_t dy = vi_diacritic_shift_y(y);

  switch (mark) {
  case VI_MARK_BREVE:
    Paint_SetPixel(x + 2, y + dy + 2, BLACK);
    Paint_SetPixel(x + 3, y + dy + 3, BLACK);
    Paint_SetPixel(x + 4, y + dy + 2, BLACK);
    break;
  case VI_MARK_CIRCUMFLEX:
    Paint_SetPixel(x + 1, y + dy + 3, BLACK);
    Paint_SetPixel(x + 2, y + dy + 2, BLACK);
    Paint_SetPixel(x + 3, y + dy + 1, BLACK);
    Paint_SetPixel(x + 4, y + dy + 2, BLACK);
    Paint_SetPixel(x + 5, y + dy + 3, BLACK);
    Paint_SetPixel(x + 3, y + dy + 2, WHITE);
    break;
  case VI_MARK_HORN:
    Paint_SetPixel(x + 6, y + dy + 2, BLACK);
    Paint_SetPixel(x + 7, y + dy + 1, BLACK);
    Paint_SetPixel(x + 7, y + dy + 2, BLACK);
    break;
  case VI_MARK_BAR:
    Paint_DrawLine(x + 1, y + 7, x + 5, y + 7, BLACK, DOT_PIXEL_1X1,
                   LINE_STYLE_SOLID);
    break;
  default:
    break;
  }
}

static void vi_draw_tone(uint16_t x, uint16_t y, vi_tone_t tone,
                         vi_mark_t mark) {
  uint16_t ax = x + 2;
  uint16_t ay = y + 1;
  uint16_t dy = vi_diacritic_shift_y(y);
  int separate_top_mark = 0;

  if (mark == VI_MARK_BREVE || mark == VI_MARK_CIRCUMFLEX) {
    ax = x + 6;
    ay = y;
    separate_top_mark = 1;
  } else if (mark == VI_MARK_HORN) {
    ax = x + 5;
    ay = y + 1;
  }

  ay += dy;

  switch (tone) {
  case VI_TONE_GRAVE:
    Paint_SetPixel(ax, ay + 0, BLACK);
    Paint_SetPixel(ax + 1, ay + 1, BLACK);
    break;
  case VI_TONE_ACUTE:
    Paint_SetPixel(ax, ay + 1, BLACK);
    Paint_SetPixel(ax + 1, ay + 0, BLACK);
    break;
  case VI_TONE_HOOK:
    Paint_SetPixel(ax, ay + 0, BLACK);
    Paint_SetPixel(ax + 1, ay + 0, BLACK);
    Paint_SetPixel(ax + 1, ay + 1, BLACK);
    Paint_SetPixel(ax, ay + 2, BLACK);
    break;
  case VI_TONE_TILDE:
    Paint_SetPixel(ax, ay + 1, BLACK);
    Paint_SetPixel(ax + 1, ay + 0, BLACK);
    Paint_SetPixel(ax + 2, ay + 1, BLACK);
    Paint_SetPixel(ax + 3, ay + 0, BLACK);
    break;
  case VI_TONE_DOT:
    vi_draw_dot(x + 3, y + 13);
    break;
  default:
    break;
  }
}

static size_t vi_utf8_prefix_bytes(const char *text, uint8_t max_chars) {
  const char *p = text;
  uint8_t count = 0;

  while (*p != '\0' && count < max_chars) {
    vi_decode_utf8(&p);
    count++;
  }

  return (size_t)(p - text);
}

static int vi_is_combining_codepoint(uint32_t cp) {
  switch (cp) {
  case 0x0300:
  case 0x0301:
  case 0x0302:
  case 0x0303:
  case 0x0306:
  case 0x0309:
  case 0x031B:
  case 0x0323:
    return 1;
  default:
    return 0;
  }
}

static void vi_apply_combining_codepoint(vi_glyph_t *glyph, uint32_t cp) {
  switch (cp) {
  case 0x0300:
    glyph->tone = VI_TONE_GRAVE;
    break;
  case 0x0301:
    glyph->tone = VI_TONE_ACUTE;
    break;
  case 0x0302:
    glyph->mark = VI_MARK_CIRCUMFLEX;
    break;
  case 0x0303:
    glyph->tone = VI_TONE_TILDE;
    break;
  case 0x0306:
    glyph->mark = VI_MARK_BREVE;
    break;
  case 0x0309:
    glyph->tone = VI_TONE_HOOK;
    break;
  case 0x031B:
    glyph->mark = VI_MARK_HORN;
    break;
  case 0x0323:
    glyph->tone = VI_TONE_DOT;
    break;
  default:
    break;
  }
}

static void draw_vi_text(uint16_t x, uint16_t y, const char *text) {
  const char *p = text;
  uint16_t cursor_x = x;

  while (*p != '\0') {
    const char *char_start = p;
    vi_glyph_t glyph;
    char ascii[2];
    uint32_t cp = vi_decode_utf8(&p);

    if (!vi_map_codepoint(cp, &glyph)) {
      if (cp < 0x80) {
        glyph.base = (char)cp;
        glyph.mark = VI_MARK_NONE;
        glyph.tone = VI_TONE_NONE;
      } else {
        glyph.base = '?';
      }
    }

    while (*p != '\0') {
      const char *peek = p;
      uint32_t next_cp = vi_decode_utf8(&peek);

      if (!vi_is_combining_codepoint(next_cp)) {
        break;
      }

      vi_apply_combining_codepoint(&glyph, next_cp);
      p = peek;
    }

    ascii[0] = glyph.base;
    ascii[1] = '\0';
    EPD_DrawUTF8(cursor_x, y + VI_BASE_Y, 0, ascii, EPD_ASCII_Font8, 0, BLACK,
                 WHITE);
    if (glyph.mark != VI_MARK_NONE) {
      vi_draw_mark(cursor_x, y, glyph.mark);
    }
    if (glyph.tone != VI_TONE_NONE) {
      vi_draw_tone(cursor_x, y, glyph.tone, glyph.mark);
    }

    if ((uint32_t)(unsigned char)glyph.base == '?' && cp >= 0x80) {
      EPD_DrawUTF8(cursor_x, y + VI_BASE_Y, 0, char_start, EPD_ASCII_Font8, 0,
                   BLACK, WHITE);
    }
    cursor_x += VI_CELL_W;
  }
}

static void draw_field(uint16_t x, uint16_t y, const char *label,
                       const char *value, uint8_t max_value_len) {
  char line[32];
  size_t value_len = vi_utf8_prefix_bytes(value, max_value_len);

  snprintf(line, sizeof(line), "%s:%.*s", label, (int)value_len, value);
  draw_vi_text(x, y, line);
}

void draw_fabric_record_page(const fabric_record_t *record, bool force_redraw) {
  (void)force_redraw;

  Paint_NewImage(epd_buffer, EPD_2IN13_V2_WIDTH, EPD_2IN13_V2_HEIGHT, 270,
                 WHITE);
  Paint_SelectImage(epd_buffer);
  Paint_SetMirroring(MIRROR_VERTICAL);
  Paint_Clear(WHITE);

  Paint_DrawRectangle(1, 1, 211, 103, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawRectangle(2, 2, 182, 22, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawRectangle(3, 3, 181, 21, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  draw_vi_text(8, 6, fabric_title_utf8);
  Paint_DrawLine(106, 25, 106, 100, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  draw_field(4, 27, "W", record->width, 13);
  draw_field(4, 39, "PO", record->po, 12);
  draw_field(4, 51, "IT", record->item, 12);
  draw_field(4, 63, "LO", record->lot, 12);
  draw_field(4, 75, "NV", record->staff, 12);
  draw_field(4, 87, "XA", record->relax_date, 12);

  draw_field(110, 27, "OK", record->ok_date, 12);
  draw_field(110, 39, "CO", record->color, 12);
  draw_field(110, 51, "BU", record->buy, 12);
  draw_field(110, 63, "RO", record->roll, 12);
  draw_field(110, 75, "YD", record->yds, 12);
  draw_field(110, 87, "NT", record->note, 12);
}
