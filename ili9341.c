/**
 * Copyright (c) 2017 - 2020, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "sdk_common.h"

#if NRF_MODULE_ENABLED(ILI9341)

#include "nrf_lcd.h"
#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrfx_spim.h"

#define RST_PIN 12

// Set of commands described in ILI9341 datasheet.
#define ILI9341_NOP 0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID 0x04
#define ILI9341_RDDST 0x09

#define ILI9341_SLPIN 0x10
#define ILI9341_SLPOUT 0x11
#define ILI9341_PTLON 0x12
#define ILI9341_NORON 0x13

#define ILI9341_RDMODE 0x0A
#define ILI9341_RDMADCTL 0x0B
#define ILI9341_RDPIXFMT 0x0C
#define ILI9341_RDIMGFMT 0x0D
#define ILI9341_RDSELFDIAG 0x0F

#define ILI9341_INVOFF 0x20
#define ILI9341_INVON 0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON 0x29

#define ILI9341_CASET 0x2A
#define ILI9341_PASET 0x2B
#define ILI9341_RAMWR 0x2C
#define ILI9341_RAMRD 0x2E

#define ILI9341_PTLAR 0x30
#define ILI9341_MADCTL 0x36
#define ILI9341_PIXFMT 0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR 0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1 0xC0
#define ILI9341_PWCTR2 0xC1
#define ILI9341_PWCTR3 0xC2
#define ILI9341_PWCTR4 0xC3
#define ILI9341_PWCTR5 0xC4
#define ILI9341_VMCTR1 0xC5
#define ILI9341_VMCTR2 0xC7
#define ILI9341_PWCTRSEQ 0xCB
#define ILI9341_PWCTRA 0xCD
#define ILI9341_PWCTRB 0xCF

#define ILI9341_RDID1 0xDA
#define ILI9341_RDID2 0xDB
#define ILI9341_RDID3 0xDC
#define ILI9341_RDID4 0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1
#define ILI9341_DGMCTR1 0xE2
#define ILI9341_DGMCTR2 0xE3
#define ILI9341_TIMCTRA 0xE8
#define ILI9341_TIMCTRB 0xEA

#define ILI9341_ENGMCTR 0xF2
#define ILI9341_INCTR 0xF6
#define ILI9341_PUMP 0xF7

#define ILI9341_MADCTL_MY 0x80
#define ILI9341_MADCTL_MX 0x40
#define ILI9341_MADCTL_MV 0x20
#define ILI9341_MADCTL_ML 0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH 0x04

/* ILI9225 LCD Registers */
#define ILI9225_DRIVER_OUTPUT_CTRL (0x01u)      // Driver Output Control
#define ILI9225_LCD_AC_DRIVING_CTRL (0x02u)     // LCD AC Driving Control
#define ILI9225_ENTRY_MODE (0x03u)              // Entry Mode
#define ILI9225_DISP_CTRL1 (0x07u)              // Display Control 1
#define ILI9225_BLANK_PERIOD_CTRL1 (0x08u)      // Blank Period Control
#define ILI9225_FRAME_CYCLE_CTRL (0x0Bu)        // Frame Cycle Control
#define ILI9225_INTERFACE_CTRL (0x0Cu)          // Interface Control
#define ILI9225_OSC_CTRL (0x0Fu)                // Osc Control
#define ILI9225_POWER_CTRL1 (0x10u)             // Power Control 1
#define ILI9225_POWER_CTRL2 (0x11u)             // Power Control 2
#define ILI9225_POWER_CTRL3 (0x12u)             // Power Control 3
#define ILI9225_POWER_CTRL4 (0x13u)             // Power Control 4
#define ILI9225_POWER_CTRL5 (0x14u)             // Power Control 5
#define ILI9225_VCI_RECYCLING (0x15u)           // VCI Recycling
#define ILI9225_RAM_ADDR_SET1 (0x20u)           // Horizontal GRAM Address Set
#define ILI9225_RAM_ADDR_SET2 (0x21u)           // Vertical GRAM Address Set
#define ILI9225_GRAM_DATA_REG (0x22u)           // GRAM Data Register
#define ILI9225_GATE_SCAN_CTRL (0x30u)          // Gate Scan Control Register
#define ILI9225_VERTICAL_SCROLL_CTRL1 (0x31u)   // Vertical Scroll Control 1 Register
#define ILI9225_VERTICAL_SCROLL_CTRL2 (0x32u)   // Vertical Scroll Control 2 Register
#define ILI9225_VERTICAL_SCROLL_CTRL3 (0x33u)   // Vertical Scroll Control 3 Register
#define ILI9225_PARTIAL_DRIVING_POS1 (0x34u)    // Partial Driving Position 1 Register
#define ILI9225_PARTIAL_DRIVING_POS2 (0x35u)    // Partial Driving Position 2 Register
#define ILI9225_HORIZONTAL_WINDOW_ADDR1 (0x36u) // Horizontal Address Start Position
#define ILI9225_HORIZONTAL_WINDOW_ADDR2 (0x37u) // Horizontal Address End Position
#define ILI9225_VERTICAL_WINDOW_ADDR1 (0x38u)   // Vertical Address Start Position
#define ILI9225_VERTICAL_WINDOW_ADDR2 (0x39u)   // Vertical Address End Position
#define ILI9225_GAMMA_CTRL1 (0x50u)             // Gamma Control 1
#define ILI9225_GAMMA_CTRL2 (0x51u)             // Gamma Control 2
#define ILI9225_GAMMA_CTRL3 (0x52u)             // Gamma Control 3
#define ILI9225_GAMMA_CTRL4 (0x53u)             // Gamma Control 4
#define ILI9225_GAMMA_CTRL5 (0x54u)             // Gamma Control 5
#define ILI9225_GAMMA_CTRL6 (0x55u)             // Gamma Control 6
#define ILI9225_GAMMA_CTRL7 (0x56u)             // Gamma Control 7
#define ILI9225_GAMMA_CTRL8 (0x57u)             // Gamma Control 8
#define ILI9225_GAMMA_CTRL9 (0x58u)             // Gamma Control 9
#define ILI9225_GAMMA_CTRL10 (0x59u)            // Gamma Control 10

#define ILI9225C_INVOFF 0x20
#define ILI9225C_INVON 0x21

// autoincrement modes (register ILI9225_ENTRY_MODE, bit 5..3 )
enum autoIncMode_t
{
    R2L_BottomUp,
    BottomUp_R2L,
    L2R_BottomUp,
    BottomUp_L2R,
    R2L_TopDown,
    TopDown_R2L,
    L2R_TopDown,
    TopDown_L2R
};

/* RGB 16-bit color table definition (RG565) */
#define COLOR_BLACK 0x0000       /*   0,   0,   0 */
#define COLOR_WHITE 0xFFFF       /* 255, 255, 255 */
#define COLOR_BLUE 0x001F        /*   0,   0, 255 */
#define COLOR_GREEN 0x07E0       /*   0, 255,   0 */
#define COLOR_RED 0xF800         /* 255,   0,   0 */
#define COLOR_NAVY 0x000F        /*   0,   0, 128 */
#define COLOR_DARKBLUE 0x0011    /*   0,   0, 139 */
#define COLOR_DARKGREEN 0x03E0   /*   0, 128,   0 */
#define COLOR_DARKCYAN 0x03EF    /*   0, 128, 128 */
#define COLOR_CYAN 0x07FF        /*   0, 255, 255 */
#define COLOR_TURQUOISE 0x471A   /*  64, 224, 208 */
#define COLOR_INDIGO 0x4810      /*  75,   0, 130 */
#define COLOR_DARKRED 0x8000     /* 128,   0,   0 */
#define COLOR_OLIVE 0x7BE0       /* 128, 128,   0 */
#define COLOR_GRAY 0x8410        /* 128, 128, 128 */
#define COLOR_GREY 0x8410        /* 128, 128, 128 */
#define COLOR_SKYBLUE 0x867D     /* 135, 206, 235 */
#define COLOR_BLUEVIOLET 0x895C  /* 138,  43, 226 */
#define COLOR_LIGHTGREEN 0x9772  /* 144, 238, 144 */
#define COLOR_DARKVIOLET 0x901A  /* 148,   0, 211 */
#define COLOR_YELLOWGREEN 0x9E66 /* 154, 205,  50 */
#define COLOR_BROWN 0xA145       /* 165,  42,  42 */
#define COLOR_DARKGRAY 0x7BEF    /* 128, 128, 128 */
#define COLOR_DARKGREY 0x7BEF    /* 128, 128, 128 */
#define COLOR_SIENNA 0xA285      /* 160,  82,  45 */
#define COLOR_LIGHTBLUE 0xAEDC   /* 172, 216, 230 */
#define COLOR_GREENYELLOW 0xAFE5 /* 173, 255,  47 */
#define COLOR_SILVER 0xC618      /* 192, 192, 192 */
#define COLOR_LIGHTGRAY 0xC618   /* 192, 192, 192 */
#define COLOR_LIGHTGREY 0xC618   /* 192, 192, 192 */
#define COLOR_LIGHTCYAN 0xE7FF   /* 224, 255, 255 */
#define COLOR_VIOLET 0xEC1D      /* 238, 130, 238 */
#define COLOR_AZUR 0xF7FF        /* 240, 255, 255 */
#define COLOR_BEIGE 0xF7BB       /* 245, 245, 220 */
#define COLOR_MAGENTA 0xF81F     /* 255,   0, 255 */
#define COLOR_TOMATO 0xFB08      /* 255,  99,  71 */
#define COLOR_GOLD 0xFEA0        /* 255, 215,   0 */
#define COLOR_ORANGE 0xFD20      /* 255, 165,   0 */
#define COLOR_SNOW 0xFFDF        /* 255, 250, 250 */
#define COLOR_YELLOW 0xFFE0      /* 255, 255,   0 */

//static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(ILI9341_SPI_INSTANCE);
static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(ILI9341_SPI_INSTANCE);

static inline void spi_write(const void *data, size_t size)
{
    nrfx_spim_xfer_desc_t xfer_desc = NRFX_SPIM_XFER_TX(data, size);
    nrfx_spim_xfer(&spi, &xfer_desc, 0);
}

static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(ILI9341_DC_PIN);
    spi_write(&c, sizeof(c));
}

static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(ILI9341_DC_PIN);
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

    nrf_gpio_cfg_output(ILI9341_DC_PIN);

    //nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;
    spi_config.frequency = NRF_SPIM_FREQ_4M;

    spi_config.sck_pin = ILI9341_SCK_PIN;
    spi_config.miso_pin = ILI9341_MISO_PIN;
    spi_config.mosi_pin = ILI9341_MOSI_PIN;
    spi_config.ss_pin = ILI9341_SS_PIN;

    // err_code = nrf_drv_spi_init(&spi, &spi_config, NULL, NULL);
    err_code = nrfx_spim_init(&spi, &spi_config, NULL, NULL);
    return err_code;
}

static ret_code_t ili9341_init(void)
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

static void ili9341_uninit(void)
{
    // nrf_drv_spi_uninit(&spi);
}

static void ili9341_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    set_addr_window(x, y, x, y);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ILI9341_DC_PIN);

    spi_write(data, sizeof(data));

    nrf_gpio_pin_clear(ILI9341_DC_PIN);
}

static void ili9225_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    set_addr_window(x, y, width - 1, height - 1);

    uint8_t frame_length = 255;
    uint8_t data[frame_length];
    memset(data, 0xFF, sizeof(data));

    nrf_gpio_pin_set(ILI9341_DC_PIN);

    int32_t number_of_frames = (float)(width * height * 2) / 255.0f;

    /*     for (size_t i = 0; i < number_of_frames + 1; i++)
    {
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, &data, sizeof(data), NULL, 0));
    } */

    nrf_gpio_pin_clear(ILI9341_DC_PIN);
}

static void ili9341_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    set_addr_window(x, y, x + width - 1, y + height - 1);

    const uint8_t data[2] = {color >> 8, color};

    nrf_gpio_pin_set(ILI9341_DC_PIN);

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

    nrf_gpio_pin_clear(ILI9341_DC_PIN);
}

uint32_t changing_color = 0x00;
static void ili9225_clear()
{
    set_addr_window(0, 0, 176 - 1, 220 - 1);

    // frame buffer
    const uint8_t data[255];
    memset(data, changing_color, sizeof(data));

    nrf_gpio_pin_set(ILI9341_DC_PIN);

    for (size_t i = 0; i < 304; i++)
    {
        spi_write(&data, sizeof(data));
    }
    //nrf_delay_ms(100);

    nrf_gpio_pin_clear(ILI9341_DC_PIN);

    //changing_color+=0x01;
}

static void ili9341_rotation_set(nrf_lcd_rotation_t rotation)
{
    write_command(ILI9341_MADCTL);
    switch (rotation)
    {
    case NRF_LCD_ROTATE_0:
        write_data(ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);
        break;
    case NRF_LCD_ROTATE_90:
        write_data(ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
        break;
    case NRF_LCD_ROTATE_180:
        write_data(ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
        break;
    case NRF_LCD_ROTATE_270:
        write_data(ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
        break;
    default:
        break;
    }
}

static void ili9341_display_invert(bool invert)
{
    nrf_gpio_pin_clear(ILI9341_DC_PIN);
    //write_command(invert ? ILI9341_INVON : ILI9341_INVOFF);
    uint8_t data_on[] = {0x00, ILI9225C_INVON};
    uint8_t data_off[] = {0x00, ILI9225C_INVOFF};
    spi_write(invert ? data_on : data_off, 2);
}

static lcd_cb_t ili9341_cb = {
    .height = ILI9341_HEIGHT,
    .width = ILI9341_WIDTH};

const nrf_lcd_t nrf_lcd_ili9341 = {
    .lcd_init = ili9341_init,
    .lcd_uninit = ili9341_uninit,
    .lcd_pixel_draw = ili9341_pixel_draw,
    .lcd_rect_draw = ili9341_rect_draw,
    .lcd_display = ili9225_clear,
    .lcd_rotation_set = ili9341_rotation_set,
    .lcd_display_invert = ili9341_display_invert,
    .p_lcd_cb = &ili9341_cb};

#endif // NRF_MODULE_ENABLED(ILI9341)
