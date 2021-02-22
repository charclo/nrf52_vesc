#include "sdk_common.h"

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

uint8_t frame_buffer[440][176] = {0};
uint8_t orientation = 2;
uint8_t _maxY = 220;
uint8_t _maxX = 176;

 #define BUFFER_SIZE  176
  
  typedef struct ArrayList
  {
    uint8_t buffer[BUFFER_SIZE];
  } ArrayList_type;

  ArrayList_type MyArrayList[440];

  //replace 'Channel' below by the specific data channel you want to use,
  //         for instance 'NRF_SPIM->RXD', 'NRF_TWIM->RXD', etc.
  //NRF_SPIM3->TXD.MAXCNT = BUFFER_SIZE;
  //NRF_SPIM->TXD = &MyArrayList;
  // NRF_SPIM3.PTR = &MyArrayList;

static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(ILI9225_SPI_INSTANCE);

void _swap(uint16_t *a, uint16_t *b)
{
    uint16_t w = *a;
    *a = *b;
    *b = w;
}

void orientCoordinates(uint16_t *x1, uint16_t *y1)
{

    switch (orientation)
    {
    case 0: // ok
        break;
    case 1: // ok
        *y1 = _maxY - *y1 - 1;
        _swap(x1, y1);
        break;
    case 2: // ok
        *x1 = _maxX - *x1 - 1;
        *y1 = _maxY - *y1 - 1;
        break;
    case 3: // ok
        *x1 = _maxX - *x1 - 1;
        _swap(x1, y1);
        break;
    }
}

uint16_t setColor(uint8_t red8, uint8_t green8, uint8_t blue8)
{
    // rgb16 = red5 green6 blue5
    return (red8 >> 3) << 11 | (green8 >> 2) << 5 | (blue8 >> 3);
}

static inline void spi_write(const void *data, size_t size)
{
    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(data, size);
    nrfx_spim_xfer(&spi, &xfer_desc,  NRFX_SPIM_FLAG_TX_POSTINC);
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
    write_data(0x10); write_data(0x00);
    write_command(ILI9225_HORIZONTAL_WINDOW_ADDR1);
    write_data(x_1 >> 8); write_data(x_1);
    write_command(ILI9225_HORIZONTAL_WINDOW_ADDR2);
    write_data(x_0 >> 8); write_data(x_0);

    write_command(ILI9225_VERTICAL_WINDOW_ADDR1);
    write_data(y_1 >> 8); write_data(y_1);
    write_command(ILI9225_VERTICAL_WINDOW_ADDR2);
    write_data(y_0 >> 8); write_data(y_0);

    write_command(ILI9225_RAM_ADDR_SET1);
    write_data(x_1 >> 8); write_data(x_1);
    write_command(ILI9225_RAM_ADDR_SET2);
    write_data(y_1 >> 8); write_data(y_1);

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
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_POWER_CTRL2);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_POWER_CTRL3);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_POWER_CTRL4);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_POWER_CTRL5);
    write_data(0x00); write_data(0x00);
    nrf_delay_ms(40);

    // Power-on sequence
    write_command(ILI9225_POWER_CTRL2);
    write_data(0x00); write_data(0x18);
    write_command(ILI9225_POWER_CTRL3);
    write_data(0x61); write_data(0x21);
    write_command(ILI9225_POWER_CTRL4);
    write_data(0x00); write_data(0x6F);
    write_command(ILI9225_POWER_CTRL5);
    write_data(0x49); write_data(0x5F);
    write_command(ILI9225_POWER_CTRL1);
    write_data(0x08); write_data(0x00);
    nrf_delay_ms(10);

    write_command(ILI9225_POWER_CTRL2);
    write_data(0x10); write_data(0x3B);
    nrf_delay_ms(50);

    write_command(ILI9225_DRIVER_OUTPUT_CTRL);
    write_data(0x01); write_data(0x1C);
    write_command(ILI9225_LCD_AC_DRIVING_CTRL);
    write_data(0x01); write_data(0x00);
    write_command(ILI9225_ENTRY_MODE);
    write_data(0x10); write_data(0x38);
    write_command(ILI9225_DISP_CTRL1);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_BLANK_PERIOD_CTRL1);
    write_data(0x08); write_data(0x08);

    write_command(ILI9225_FRAME_CYCLE_CTRL);
    write_data(0x11); write_data(0x00);
    write_command(ILI9225_INTERFACE_CTRL);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_OSC_CTRL);
    write_data(0x0D); write_data(0x01);
    write_command(ILI9225_VCI_RECYCLING);
    write_data(0x00); write_data(0x20);
    write_command(ILI9225_RAM_ADDR_SET1);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_RAM_ADDR_SET2);
    write_data(0x00); write_data(0x00);

    /* Set GRAM area */
    write_command(ILI9225_GATE_SCAN_CTRL);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_VERTICAL_SCROLL_CTRL1);
    write_data(0x00); write_data(0xDB);
    write_command(ILI9225_VERTICAL_SCROLL_CTRL2);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_VERTICAL_SCROLL_CTRL3);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_PARTIAL_DRIVING_POS1);
    write_data(0x00); write_data(0xDB);
    write_command(ILI9225_PARTIAL_DRIVING_POS2);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_HORIZONTAL_WINDOW_ADDR1);
    write_data(0x00); write_data(0xAF);
    write_command(ILI9225_HORIZONTAL_WINDOW_ADDR2);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_VERTICAL_WINDOW_ADDR1);
    write_data(0x00); write_data(0xDB);
    write_command(ILI9225_VERTICAL_WINDOW_ADDR2);
    write_data(0x00); write_data(0x00);

    /* Set GAMMA curve */
    write_command(ILI9225_GAMMA_CTRL1);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_GAMMA_CTRL2);
    write_data(0x08); write_data(0x08);
    write_command(ILI9225_GAMMA_CTRL3);
    write_data(0x08); write_data(0x0A);
    write_command(ILI9225_GAMMA_CTRL4);
    write_data(0x00); write_data(0x0A);
    write_command(ILI9225_GAMMA_CTRL5);
    write_data(0x0A); write_data(0x08);
    write_command(ILI9225_GAMMA_CTRL6);
    write_data(0x08); write_data(0x08);
    write_command(ILI9225_GAMMA_CTRL7);
    write_data(0x00); write_data(0x00);
    write_command(ILI9225_GAMMA_CTRL8);
    write_data(0x0A); write_data(0x00);
    write_command(ILI9225_GAMMA_CTRL9);
    write_data(0x07); write_data(0x10);
    write_command(ILI9225_GAMMA_CTRL10);
    write_data(0x07); write_data(0x10);

    write_command(ILI9225_DISP_CTRL1);
    write_data(0x00); write_data(0x12);
    nrf_delay_ms(50);

    write_command(ILI9225_DISP_CTRL1);
    write_data(0x10); write_data(0x17);
}

static ret_code_t hardware_init(void)
{
    // memset(frame_buffer, 0xFF, 176);
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

void ili9225_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
/*     set_addr_window(x, y, x + width - 1, y + height - 1);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ILI9225_DC_PIN);

    // Duff's device algorithm for optimizing loop.
    uint32_t i = (height * width + 7) / 8;

    /*lint -save -e525 -e616 -e646 */
/*     switch ((height * width) % 8)
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
    } */
    /*lint -restore */

    /*nrf_gpio_pin_clear(ILI9225_DC_PIN); */

    orientCoordinates(&x, &y);

    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
        frame_buffer[(y-i)*2][(x-j)*2] = color >> 8;
        frame_buffer[(y-i)*2][(x-j)*2+1] = color;
        }
    }
}

void ili9225_clear()
{
    set_addr_window(0, 0, _maxX - 1, _maxY - 1);

    nrf_gpio_pin_set(ILI9225_DC_PIN);

    for (size_t i = 0; i < (220*2); i++)
    {
        spi_write(&frame_buffer[i][0], 176);
    }

    nrf_gpio_pin_clear(ILI9225_DC_PIN);

    //memset(frame_buffer, 0x00, 176*440);
}

void ili9225_draw_line(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    //set_addr_window(x, y, x + width, y + height);

    //uint16_t buffer_size = width * height * 2;

    // frame buffer
    //uint8_t data[buffer_size];
    //memset(data, color, sizeof(data));

/*     nrf_gpio_pin_set(ILI9225_DC_PIN);

    spi_write(&data, sizeof(data));

    nrf_gpio_pin_clear(ILI9225_DC_PIN); */

    orientCoordinates(&x, &y);

    for (size_t i = 0; i < height; i++)
    {
        frame_buffer[(y-i)*2][x*2] = color >> 8;
        frame_buffer[(y-i)*2][x*2+1] = color;
    }

    for (size_t i = 0; i < width; i++)
    {
        frame_buffer[(y)*2][(x-i)*2] = color >> 8;
        frame_buffer[(y)*2][(x-i)*2+1] = color;
    }
}

static void ili9225_rotation_set(nrf_lcd_rotation_t rotation)
{
    // not implemented yet
}

static void ili9225_display_invert(bool invert)
{
    nrf_gpio_pin_clear(ILI9225_DC_PIN);
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


void ili9225_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    orientCoordinates(&x, &y);

    frame_buffer[y*2][x*2] = color >> 8;
    frame_buffer[y*2][x*2+1] = color;


    /*     if ((x >= 176) || (y >= 220))
        return;

    orientCoordinates(&x, &y);

    set_addr_window(x, y, x, y);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ILI9225_DC_PIN);

    spi_write(data, sizeof(data));

    nrf_gpio_pin_clear(ILI9225_DC_PIN); */

/*     write_command(ILI9225_RAM_ADDR_SET1);
    write_data(x >> 8);
    write_data(x);
    write_command(ILI9225_RAM_ADDR_SET2);
    write_data(y >> 8);
    write_data(y);
    write_command(ILI9225_GRAM_DATA_REG);
    write_data(color >> 8);
    write_data(color); */
}

// endif // NRF_MODULE_ENABLED(ILI9225)
