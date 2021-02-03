#include "sdk_common.h"

#if NRF_MODULE_ENABLED(ILI9225)

#include "nrf_lcd.h"
#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrfx_spim.h"
#include "ili9225.h"
#include "nrf_gfx.h"
#include <math.h>

#define RST_PIN 12

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

extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
static const nrf_gfx_font_desc_t *p_font = &orkney_8ptFontInfo;

static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(ILI9225_SPI_INSTANCE);

uint16_t setColor(uint8_t red8, uint8_t green8, uint8_t blue8)
{
    // rgb16 = red5 green6 blue5
    return (red8 >> 3) << 11 | (green8 >> 2) << 5 | (blue8 >> 3);
}

static inline void spi_write(const void *data, size_t size)
{
    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(data, size);
    nrfx_spim_xfer(&spi, &xfer_desc, 0);
}

static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(ILI9225_DC_PIN);
    spi_write(&c, sizeof(c));
}

static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(ILI9225_DC_PIN);
    spi_write(&c, sizeof(c));
}

static void set_addr_window(uint16_t x_0, uint16_t y_0, uint16_t x_1, uint16_t y_1)
{
    ASSERT(x_0 <= x_1);
    ASSERT(y_0 <= y_1);

    write_command(ILI9225_ENTRY_MODE);
    write_data(0x10);
    write_data(0x00);
    write_command(ILI9225_HORIZONTAL_WINDOW_ADDR1);
    write_data(x_1 >> 8);
    write_data(x_1);
    write_command(ILI9225_HORIZONTAL_WINDOW_ADDR2);
    write_data(x_0 >> 8);
    write_data(x_0);

    write_command(ILI9225_VERTICAL_WINDOW_ADDR1);
    write_data(y_1 >> 8);
    write_data(y_1);
    write_command(ILI9225_VERTICAL_WINDOW_ADDR2);
    write_data(y_0 >> 8);
    write_data(y_0);

    write_command(ILI9225_RAM_ADDR_SET1);
    write_data(x_1 >> 8);
    write_data(x_1);
    write_command(ILI9225_RAM_ADDR_SET2);
    write_data(y_1 >> 8);
    write_data(y_1);

    write_command(ILI9225_GRAM_DATA_REG);
}

static void command_list(void)
{

    nrf_gpio_cfg_output(RST_PIN);

    nrf_gpio_pin_set(RST_PIN);
    nrf_delay_ms(1);
    nrf_gpio_pin_clear(RST_PIN);
    nrf_delay_ms(10);
    nrf_gpio_pin_set(RST_PIN);
    nrf_delay_ms(50);

    /* Set SS bit and direction output from S528 to S1 */
    write_command(ILI9225_POWER_CTRL1);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_POWER_CTRL2);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_POWER_CTRL3);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_POWER_CTRL4);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_POWER_CTRL5);
    write_data(0x00);
    write_data(0x00);
    nrf_delay_ms(40);

    // Power-on sequence
    write_command(ILI9225_POWER_CTRL2);
    write_data(0x00);
    write_data(0x18);
    write_command(ILI9225_POWER_CTRL3);
    write_data(0x61);
    write_data(0x21);
    write_command(ILI9225_POWER_CTRL4);
    write_data(0x00);
    write_data(0x6F);
    write_command(ILI9225_POWER_CTRL5);
    write_data(0x49);
    write_data(0x5F);
    write_command(ILI9225_POWER_CTRL1);
    write_data(0x08);
    write_data(0x00);
    nrf_delay_ms(10);

    write_command(ILI9225_POWER_CTRL2);
    write_data(0x10);
    write_data(0x3B);
    nrf_delay_ms(50);

    write_command(ILI9225_DRIVER_OUTPUT_CTRL);
    write_data(0x01);
    write_data(0x1C);
    write_command(ILI9225_LCD_AC_DRIVING_CTRL);
    write_data(0x01);
    write_data(0x00);
    write_command(ILI9225_ENTRY_MODE);
    write_data(0x10);
    write_data(0x38);
    write_command(ILI9225_DISP_CTRL1);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_BLANK_PERIOD_CTRL1);
    write_data(0x08);
    write_data(0x08);

    write_command(ILI9225_FRAME_CYCLE_CTRL);
    write_data(0x11);
    write_data(0x00);
    write_command(ILI9225_INTERFACE_CTRL);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_OSC_CTRL);
    write_data(0x0D);
    write_data(0x01);
    write_command(ILI9225_VCI_RECYCLING);
    write_data(0x00);
    write_data(0x20);
    write_command(ILI9225_RAM_ADDR_SET1);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_RAM_ADDR_SET2);
    write_data(0x00);
    write_data(0x00);

    /* Set GRAM area */
    write_command(ILI9225_GATE_SCAN_CTRL);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_VERTICAL_SCROLL_CTRL1);
    write_data(0x00);
    write_data(0xDB);
    write_command(ILI9225_VERTICAL_SCROLL_CTRL2);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_VERTICAL_SCROLL_CTRL3);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_PARTIAL_DRIVING_POS1);
    write_data(0x00);
    write_data(0xDB);
    write_command(ILI9225_PARTIAL_DRIVING_POS2);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_HORIZONTAL_WINDOW_ADDR1);
    write_data(0x00);
    write_data(0xAF);
    write_command(ILI9225_HORIZONTAL_WINDOW_ADDR2);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_VERTICAL_WINDOW_ADDR1);
    write_data(0x00);
    write_data(0xDB);
    write_command(ILI9225_VERTICAL_WINDOW_ADDR2);
    write_data(0x00);
    write_data(0x00);

    /* Set GAMMA curve */
    write_command(ILI9225_GAMMA_CTRL1);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_GAMMA_CTRL2);
    write_data(0x08);
    write_data(0x08);
    write_command(ILI9225_GAMMA_CTRL3);
    write_data(0x08);
    write_data(0x0A);
    write_command(ILI9225_GAMMA_CTRL4);
    write_data(0x00);
    write_data(0x0A);
    write_command(ILI9225_GAMMA_CTRL5);
    write_data(0x0A);
    write_data(0x08);
    write_command(ILI9225_GAMMA_CTRL6);
    write_data(0x08);
    write_data(0x08);
    write_command(ILI9225_GAMMA_CTRL7);
    write_data(0x00);
    write_data(0x00);
    write_command(ILI9225_GAMMA_CTRL8);
    write_data(0x0A);
    write_data(0x00);
    write_command(ILI9225_GAMMA_CTRL9);
    write_data(0x07);
    write_data(0x10);
    write_command(ILI9225_GAMMA_CTRL10);
    write_data(0x07);
    write_data(0x10);

    write_command(ILI9225_DISP_CTRL1);
    write_data(0x00);
    write_data(0x12);
    nrf_delay_ms(50);

    write_command(ILI9225_DISP_CTRL1);
    write_data(0x10);
    write_data(0x17);
}

static ret_code_t hardware_init(void)
{
    ret_code_t err_code;

    nrf_gpio_cfg_output(ILI9225_DC_PIN);

    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
    spi_config.frequency = NRF_SPIM_FREQ_32M;

    spi_config.sck_pin = ILI9225_SCK_PIN;
    spi_config.miso_pin = ILI9225_MISO_PIN;
    spi_config.mosi_pin = ILI9225_MOSI_PIN;
    spi_config.ss_pin = ILI9225_SS_PIN;

    err_code = nrfx_spim_init(&spi, &spi_config, NULL, NULL);
    return err_code;
}

static ret_code_t ili9225_init(void)
{
    ret_code_t err_code;

    err_code = hardware_init();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    command_list();

    return err_code;
}

static void ili9225_uninit(void)
{
    // nrf_drv_spi_uninit(&spi);
}

static void ili9225_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    set_addr_window(x, y, x, y);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ILI9225_DC_PIN);

    spi_write(data, sizeof(data));

    nrf_gpio_pin_clear(ILI9225_DC_PIN);
}

/* static void ili9225_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    set_addr_window(x, y, width - 1, height - 1);

    uint8_t frame_length = 255;
    uint8_t data[frame_length];
    memset(data, 0xFF, sizeof(data));

    nrf_gpio_pin_set(ILI9225_DC_PIN);

    int32_t number_of_frames = (float)(width * height * 2) / 255.0f;

    /     for (size_t i = 0; i < number_of_frames + 1; i++)
    {
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, &data, sizeof(data), NULL, 0));
    } /

/    nrf_gpio_pin_clear(ILI9225_DC_PIN);
} */

static void ili9225_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    set_addr_window(x, y, x + width - 1, y + height - 1);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ILI9225_DC_PIN);

    // Duff's device algorithm for optimizing loop.
    uint32_t i = (height * width + 7) / 8;

    /*lint -save -e525 -e616 -e646 */
    switch ((height * width) % 8)
    {
    case 0:
        do
        {
            spi_write(data, sizeof(data));
        case 7:
            spi_write(data, sizeof(data));
        case 6:
            spi_write(data, sizeof(data));
        case 5:
            spi_write(data, sizeof(data));
        case 4:
            spi_write(data, sizeof(data));
        case 3:
            spi_write(data, sizeof(data));
        case 2:
            spi_write(data, sizeof(data));
        case 1:
            spi_write(data, sizeof(data));
        } while (--i > 0);
    default:
        break;
    }
    /*lint -restore */

    nrf_gpio_pin_clear(ILI9225_DC_PIN);
}

uint32_t changing_color = 0x00;
static void ili9225_clear()
{
    set_addr_window(0, 0, 176 - 1, 220 - 1);

    // frame buffer
    uint8_t data[255];
    memset(data, changing_color, sizeof(data));

    nrf_gpio_pin_set(ILI9225_DC_PIN);

    for (size_t i = 0; i < 304; i++)
    {
        spi_write(&data, sizeof(data));
    }
    //nrf_delay_ms(100);

    nrf_gpio_pin_clear(ILI9225_DC_PIN);

    //changing_color+=0x01;
}

static void ili9225_rotation_set(nrf_lcd_rotation_t rotation)
{
    // not implemented yet
}

static void ili9225_display_invert(bool invert)
{
    nrf_gpio_pin_clear(ILI9225_DC_PIN);
    //write_command(invert ? ILI9225_INVON : ILI9225_INVOFF);
    uint8_t data_on[] = {0x00, ILI9225C_INVON};
    uint8_t data_off[] = {0x00, ILI9225C_INVOFF};
    spi_write(invert ? data_on : data_off, 2);
}

static lcd_cb_t ili9225_cb = {
    .height = ILI9225_HEIGHT,
    .width = ILI9225_WIDTH};

const nrf_lcd_t nrf_lcd_ili9225 = {
    .lcd_init = ili9225_init,
    .lcd_uninit = ili9225_uninit,
    .lcd_pixel_draw = ili9225_pixel_draw,
    .lcd_rect_draw = ili9225_rect_draw,
    .lcd_display = ili9225_clear,
    .lcd_rotation_set = ili9225_rotation_set,
    .lcd_display_invert = ili9225_display_invert,
    .p_lcd_cb = &ili9225_cb};

void text_print(uint8_t x, uint8_t y, uint16_t color, const char *text)
{
    nrf_gfx_point_t text_start = NRF_GFX_POINT(x, y);
    APP_ERROR_CHECK(nrf_gfx_print(&nrf_lcd_ili9225, &text_start, color, text, p_font, true));
}

void screen_clear(void)
{
    nrf_gfx_screen_fill(&nrf_lcd_ili9225, COLOR_BLACK);
}

float trip = 12.4f;
float speed = 10.1f;

void draw_ui()
{

    nrf_gfx_display(&nrf_lcd_ili9225);
    _update_battery_indicator(0.5f, true);

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
    sprintf(buffer, "%.1f", speed);
    tft_util_draw_number(buffer, 2, 35, COLOR_WHITE, COLOR_BLACK, 10, 14);

    sprintf(buffer, "%.1f", trip);
    tft_util_draw_number(buffer, 0, 130, COLOR_WHITE, COLOR_BLACK, 2, 6);

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

void circle_draw(uint8_t x, uint8_t y, uint8_t radius, uint16_t color, bool fill)
{
    nrf_gfx_circle_t my_circle = NRF_GFX_CIRCLE(x, y, radius);
    APP_ERROR_CHECK(nrf_gfx_circle_draw(&nrf_lcd_ili9225, &my_circle, color, fill));
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

// Remember how many cells are currently filled so that we can update the indicators more efficiently.
uint8_t _battery_cells_filled = 0;

void _update_battery_indicator(float battery_percent, bool redraw)
{
    int width = 15;
    int space = 2;
    int cell_count = 10;

    nrf_gfx_rect_t my_rect = NRF_GFX_RECT(nrf_gfx_width_get(&nrf_lcd_ili9225) / 2,
                                          nrf_gfx_height_get(&nrf_lcd_ili9225) / nrf_gfx_width_get(&nrf_lcd_ili9225),
                                          nrf_gfx_height_get(&nrf_lcd_ili9225),
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

void gfx_initialization(void)
{
    APP_ERROR_CHECK(nrf_gfx_init(&nrf_lcd_ili9225));
}

#endif // NRF_MODULE_ENABLED(ILI9225)
