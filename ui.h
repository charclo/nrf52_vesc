#include "sdk_common.h"

void text_print(uint8_t x, uint8_t y, uint16_t color, const char *text);
void circle_draw(uint8_t x, uint8_t y, uint8_t radius, uint16_t color, bool fill);
void charging_basic(void);
void charging(void);
void draw_ui();
void draw_options();
void tft_util_draw_digit(uint8_t digit, uint8_t x, uint8_t y,
                         uint16_t fg_color, uint16_t bg_color, uint8_t magnify);
void tft_util_draw_number(
    char *number, uint8_t x, uint8_t y,
    uint16_t fg_color, uint16_t bg_color, uint8_t spacing, uint8_t magnify);
void _update_battery_indicator(float battery_percent, bool redraw);
void screen_clear(void);