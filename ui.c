#include "sdk_common.h"

void draw_options()
{
    nrf_gfx_display(&nrf_lcd_ili9225);

    text_print(5, 10, COLOR_WHITE, "Option 1");
    text_print(5, 30, COLOR_WHITE, "Option 2");
    text_print(5, 50, COLOR_WHITE, "Option 3");
    text_print(5, 70, COLOR_WHITE, "Option 4");
}

void tft_util_draw_digit(uint8_t digit, uint8_t x, uint8_t y,
                         uint16_t fg_color, uint16_t bg_color, uint8_t magnify)
{
    for (int xx = 0; xx < 3; xx++)
    {
        for (int yy = 0; yy < 5; yy++)
        {
            uint16_t color = FONT_DIGITS_3x5[digit][yy][xx] ? fg_color : bg_color;
            nrf_gfx_rect_t my_rect = NRF_GFX_RECT(x + xx * magnify,
                                                  y + yy * magnify,
                                                  magnify,
                                                  magnify);
            APP_ERROR_CHECK(nrf_gfx_rect_draw(&nrf_lcd_ili9225, &my_rect, 1, color, true));
        }
    }
}

void tft_util_draw_number(
    char *number, uint8_t x, uint8_t y,
    uint16_t fg_color, uint16_t bg_color, uint8_t spacing, uint8_t magnify)
{
    int cursor_x = x;
    int number_len = strlen(number);
    for (int i = 0; i < number_len; i++)
    {
        char ch = number[i];
        if (ch >= '0' && ch <= '9')
        {
            tft_util_draw_digit(ch - '0', cursor_x, y, fg_color, bg_color, magnify);
            cursor_x += 3 * magnify + spacing;
        }
        else if (ch == '.')
        {
            nrf_gfx_rect_t my_rect = NRF_GFX_RECT(cursor_x,
                                                  y + 4 * magnify,
                                                  magnify,
                                                  magnify);
            APP_ERROR_CHECK(nrf_gfx_rect_draw(&nrf_lcd_ili9225, &my_rect, 1, fg_color, true));
            cursor_x += magnify + spacing;
            /*         } else if (ch == '-') {
            tft->fillRectangle(cursor_x, y, cursor_x + 3 * magnify - 1, y + 5 * magnify - 1, bg_color);
            tft->fillRectangle(cursor_x, y + 2 * magnify, cursor_x + 3 * magnify - 1, y + 3 * magnify - 1, fg_color);
            cursor_x += 3 * magnify + spacing; */
            /*         } else if (ch == ' ') {
            tft->fillRectangle(cursor_x, y, cursor_x + 3 * magnify - 1, y + 5 * magnify - 1, bg_color);
            cursor_x += 3 * magnify + spacing; */
        }
    }
}

// Remember how many cells are currently filled so that we can update the indicators more efficiently.
uint8_t _battery_cells_filled = 0;

void _update_battery_indicator(float battery_percent, bool redraw)
{
    int width = 15;
    int space = 2;
    int cell_count = 10;

    nrf_gfx_rect_t my_rect = NRF_GFX_RECT(ILI9225_WIDTH / 2,
                                          ILI9225_HEIGHT / ILI9225_WIDTH,
                                          ILI9225_HEIGHT,
                                          BORDER);

    int cells_to_fill = round(battery_percent * cell_count);
    for (int i = 0; i < cell_count; i++)
    {
        bool is_filled = (i < _battery_cells_filled);
        bool should_be_filled = (i < cells_to_fill);
        if (should_be_filled != is_filled || redraw)
        {
            int x = (i) * (width + space);
            uint8_t green = (uint8_t)(255.0 / (cell_count - 1) * i);
            uint8_t red = 255 - green;
            uint16_t color = setColor(red, green, 0);
            my_rect.x = x + 4;
            my_rect.y = 1;
            my_rect.width = 15;
            my_rect.height = 15;
            APP_ERROR_CHECK(nrf_gfx_rect_draw(&nrf_lcd_ili9225, &my_rect, 1, color, true));
            if (!should_be_filled)
            {
                my_rect.x = x + 5;
                my_rect.y = 2;
                my_rect.width = 13;
                my_rect.height = 13;
                APP_ERROR_CHECK(nrf_gfx_rect_draw(&nrf_lcd_ili9225, &my_rect, 1, COLOR_BLACK, true));
            }
        }
    }
    _battery_cells_filled = cells_to_fill;
}