#include "sdk_common.h"
#include "nrf_lcd.h"
#include "nrf_gfx.h"
#include "ili9225.h"

#define BORDER 2

extern const nrf_lcd_t nrf_lcd_ili9225;
extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
static const nrf_gfx_font_desc_t *p_font = &orkney_8ptFontInfo;

const bool FONT_DIGITS_3x5[10][5][3] = {
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
    },
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
    }};
// Remember how many cells are currently filled so that we can update the indicators more efficiently.
uint8_t _battery_cells_filled = 0;
float trip = 12.4f;
float speed = 10.1f;
uint16_t offset = 0;
uint8_t buffer[10];
uint32_t time;
uint16_t sin_table[176] = {
    10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 17, 17, 17, 17, 16, 16, 16, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10, 9, 9, 9, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10};
uint16_t base_y = 100;
// uint32_t charging_color = COLOR_DARKGREEN;


void text_print(uint8_t x, uint8_t y, uint16_t color, const char *text)
{
    nrf_gfx_point_t text_start = NRF_GFX_POINT(x, y);
    APP_ERROR_CHECK(nrf_gfx_print(&nrf_lcd_ili9225, &text_start, color, text, p_font, true));
}

void circle_draw(uint8_t x, uint8_t y, uint8_t radius, uint16_t color, bool fill)
{
    nrf_gfx_circle_t my_circle = NRF_GFX_CIRCLE(x, y, radius);
    int error = nrf_gfx_circle_draw(&nrf_lcd_ili9225, &my_circle, color, fill);
    APP_ERROR_CHECK(error);
}

void screen_clear(void)
{
    nrf_gfx_screen_fill(&nrf_lcd_ili9225, COLOR_BLACK);
}

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

void update_battery_indicator(float battery_percent, bool redraw)
{
    int width = 15;
    int space = 2;
    int cell_count = 10;

    nrf_gfx_rect_t my_rect = NRF_GFX_RECT(ILI9225_WIDTH / 2,
                                          ILI9225_HEIGHT / ILI9225_WIDTH,
                                          ILI9225_HEIGHT,
                                          BORDER);

    int cells_to_fill = battery_percent * cell_count;
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

	uint32_t time;
	uint32_t time_diff;
	uint8_t buffer[10];

void draw_ui()
{
    update_battery_indicator(0.49f, false);

    text_print(0, 110, COLOR_WHITE, "TRIP KM     ");
    text_print(0, 175, COLOR_WHITE, "TOTAL KM    ");
    text_print(110, 110, COLOR_WHITE, "WATTS       ");
    text_print(110, 175, COLOR_WHITE, "BATT V");

    text_print(145, 21, COLOR_WHITE, "KMH");

    char temperature[] = {'2', '0', '.', '3'};
    tft_util_draw_number(temperature, 4, 21, COLOR_WHITE, COLOR_BLACK, 2, 2);
    circle_draw(37, 22, 1, COLOR_WHITE, false);
    text_print(40, 21, COLOR_WHITE, "C");

    char buffer[10];

    float watts = 245.3f;
    sprintf(buffer, "%.1f", watts);
    tft_util_draw_number(buffer, 90, 130, COLOR_WHITE, COLOR_BLACK, 2, 6);

    float total = 108.4f;
    sprintf(buffer, "%.1f", total);
    tft_util_draw_number(buffer, 0, 190, COLOR_WHITE, COLOR_BLACK, 2, 6);

    float voltage = 39.9f;
    sprintf(buffer, "%.1f", voltage);
    tft_util_draw_number(buffer, 105, 190, COLOR_WHITE, COLOR_BLACK, 2, 6);


}

void update_ui(void)
{

    char buffer[10];
    sprintf(buffer, "%.1f", speed);
    tft_util_draw_number(buffer, 2, 35, COLOR_WHITE, COLOR_BLACK, 10, 14);

    sprintf(buffer, "%.1f", trip);
    tft_util_draw_number(buffer, 0, 130, COLOR_WHITE, COLOR_BLACK, 2, 6);
    speed+=0.001;
    trip+=0.001;
}

void overlay(void){
    nrf_gfx_rect_t my_rect = NRF_GFX_RECT(10,
                                          10,
                                          80,
                                          20);

    APP_ERROR_CHECK(nrf_gfx_rect_draw(&nrf_lcd_ili9225, &my_rect, 1, COLOR_BLACK, true));

    time_diff = app_timer_cnt_diff_compute(app_timer_cnt_get(), time);
    float seconds_elapsed = 1/ ((float)time_diff / (32 * 1000));
    sprintf(buffer, "%f", seconds_elapsed);
    
    time = app_timer_cnt_get();

    text_print(12, 12, COLOR_WHITE, buffer);

}

void charging_basic(void)
{
    ili9225_rect_draw(0, 100, 176, 120, COLOR_DARKGREEN);
    text_print(70, 140, COLOR_WHITE, "68%");
}

void charging(void)
{
    for (uint16_t i = 0; i < ILI9225_WIDTH; i++)
    {
        uint16_t i_offset = (i + offset) % 176;
        uint16_t start_y = base_y - sin_table[i_offset];
        uint16_t height = sin_table[i_offset];
        if (height > 0)
        {
            ili9225_draw_line(i, start_y, 1, height, COLOR_DARKGREEN);
        }

        height = 40 - sin_table[i_offset];
        if (height > 0)
        {
            ili9225_draw_line(i, base_y - 40, 1, height, COLOR_BLACK);
        }
    }

    offset += 5;
}