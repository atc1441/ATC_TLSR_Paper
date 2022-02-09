#include <stdint.h>
#include "tl_common.h"
#include "main.h"
#include "epd.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "battery.h"

#include "OneBitDisplay.h"
#include "TIFF_G4.h"
extern const uint8_t ucMirror[];
#include "Roboto_Black_80.h"
#include "font_60.h"

RAM uint8_t epd_update_state = 0;

uint8_t epd_buffer[epd_buffer_size];
uint8_t epd_temp[epd_buffer_size]; // for OneBitDisplay to draw into
OBDISP obd;                        // virtual display structure
TIFFIMAGE tiff;

#define EPD_POWER_ON() gpio_write(EPD_ENABLE, 0)

#define EPD_POWER_OFF() gpio_write(EPD_ENABLE, 1)

#define EPD_ENABLE_WRITE_CMD() gpio_write(EPD_DC, 0)
#define EPD_ENABLE_WRITE_DATA() gpio_write(EPD_DC, 1)

#define EPD_IS_BUSY() (!gpio_read(EPD_BUSY))

_attribute_ram_code_ void EPD_SPI_Write(unsigned char value)
{
    unsigned char i;

    WaitUs(10);
    for (i = 0; i < 8; i++)
    {
        gpio_write(EPD_CLK, 0);
        if (value & 0x80)
        {
            gpio_write(EPD_MOSI, 1);
        }
        else
        {
            gpio_write(EPD_MOSI, 0);
        }
        value = (value << 1);
        gpio_write(EPD_CLK, 1);
    }
}

_attribute_ram_code_ void EPD_WriteCmd(unsigned char cmd)
{
    gpio_write(EPD_CS, 0);
    EPD_ENABLE_WRITE_CMD();
    EPD_SPI_Write(cmd);
    gpio_write(EPD_CS, 1);
}

_attribute_ram_code_ void EPD_WriteData(unsigned char data)
{
    gpio_write(EPD_CS, 0);
    EPD_ENABLE_WRITE_DATA();
    EPD_SPI_Write(data);
    gpio_write(EPD_CS, 1);
}

_attribute_ram_code_ void EPD_CheckStatus(void)
{
    while (EPD_IS_BUSY())
    {
    }
}

_attribute_ram_code_ void EPD_LoadImage(unsigned char *image, int size)
{
    int i;
    EPD_WriteCmd(0x10);
    for (i = 0; i < size; i++)
    {
        EPD_WriteData(0xff);
    }

    EPD_WriteCmd(0x13);
    for (i = 0; i < size; i++)
    {
        EPD_WriteData(image[i]);
    }
    WaitMs(2);
}

_attribute_ram_code_ void init_epd()
{
    gpio_set_func(EPD_RESET, AS_GPIO);
    gpio_set_output_en(EPD_RESET, 1);
    gpio_setup_up_down_resistor(EPD_RESET, PM_PIN_PULLUP_1M);

    gpio_set_func(EPD_DC, AS_GPIO);
    gpio_set_output_en(EPD_DC, 1);
    gpio_setup_up_down_resistor(EPD_DC, PM_PIN_PULLUP_1M);

    gpio_set_func(EPD_BUSY, AS_GPIO);
    gpio_set_output_en(EPD_BUSY, 0);
    gpio_set_input_en(EPD_BUSY, 1);
    gpio_setup_up_down_resistor(EPD_BUSY, PM_PIN_PULLUP_1M);

    gpio_set_func(EPD_CS, AS_GPIO);
    gpio_set_output_en(EPD_CS, 1);
    gpio_setup_up_down_resistor(EPD_CS, PM_PIN_PULLUP_1M);

    gpio_set_func(EPD_CLK, AS_GPIO);
    gpio_set_output_en(EPD_CLK, 1);
    gpio_setup_up_down_resistor(EPD_CLK, PM_PIN_PULLUP_1M);

    gpio_set_func(EPD_MOSI, AS_GPIO);
    gpio_set_output_en(EPD_MOSI, 1);
    gpio_setup_up_down_resistor(EPD_MOSI, PM_PIN_PULLUP_1M);

    // gpio_set_func(EPD_ENABLE, AS_GPIO);
    gpio_set_output_en(EPD_ENABLE, 0);
    gpio_set_input_en(EPD_ENABLE, 1);
    // gpio_shutdown(EPD_ENABLE);
    gpio_setup_up_down_resistor(EPD_ENABLE, PM_PIN_PULLUP_1M);
}

_attribute_ram_code_ void deinit_epd(void)
{
    gpio_shutdown(EPD_RESET);
    gpio_shutdown(EPD_DC);
    gpio_shutdown(EPD_CS);
    gpio_shutdown(EPD_CLK);
    gpio_shutdown(EPD_MOSI);
    gpio_shutdown(EPD_BUSY);
}

_attribute_ram_code_ void EPD_Display(unsigned char *image, int size)
{
    init_epd();
    // system power
    EPD_POWER_ON();
    // Reset the EPD driver IC
    gpio_write(EPD_RESET, 0);
    WaitMs(10);
    gpio_write(EPD_RESET, 1);
    WaitMs(10);
    // Booster soft start
    EPD_WriteCmd(0x06);
    EPD_WriteData(0x17);
    EPD_WriteData(0x17);
    EPD_WriteData(0x17);
    // power on
    EPD_WriteCmd(0x04);

    // check BUSY pin
    EPD_CheckStatus();

    // panel setting
    EPD_WriteCmd(0x00);
    EPD_WriteData(0x1f);
    EPD_WriteData(0x0f);

    // resolution setting
    EPD_WriteCmd(0x61);
    EPD_WriteData(0x80);
    EPD_WriteData(0x01);
    EPD_WriteData(0x28);

    // Vcom and data interval setting
    EPD_WriteCmd(0X50);
    EPD_WriteData(0x97);

    // load image data to EPD
    EPD_LoadImage(image, size);

    // trigger display refresh
    EPD_WriteCmd(0x12);

    epd_update_state = 1;
}

_attribute_ram_code_ void epd_set_sleep()
{
    // Vcom and data interval setting
    EPD_WriteCmd(0x50);
    EPD_WriteData(0xf7);

    // power off
    EPD_WriteCmd(0x02);

    // deep sleep
    EPD_WriteCmd(0x07);
    EPD_WriteData(0xa5);

    EPD_POWER_OFF();
    deinit_epd();
    epd_update_state = 0;
}

_attribute_ram_code_ uint8_t epd_state_handler()
{
    switch (epd_update_state)
    {
    case 0:
        // Nothing todo
        break;
    case 1: // check if refresh is done and sleep epd if so
        if (!EPD_IS_BUSY())
            epd_set_sleep();
        break;
    }
    return epd_update_state;
}

_attribute_ram_code_ void FixBuffer(uint8_t *pSrc, uint8_t *pDst)
{
    int x, y;
    uint8_t *s, *d;
    for (y = 0; y < 16; y++)
    { // byte rows
        d = &pDst[y];
        s = &pSrc[y * 250];
        for (x = 0; x < 250; x++)
        {
            d[x * 16] = ~ucMirror[s[249 - x]]; // invert and flip
        }                                      // for x
    }                                          // for y
}

_attribute_ram_code_ void TIFFDraw(TIFFDRAW *pDraw)
{
    uint8_t uc = 0, ucSrcMask, ucDstMask, *s, *d;
    int x, y;

    s = pDraw->pPixels;
    y = pDraw->y;                          // current line
    d = &epd_buffer[(249 * 16) + (y / 8)]; // rotated 90 deg clockwise
    ucDstMask = 0x80 >> (y & 7);           // destination mask
    ucSrcMask = 0;                         // src mask
    for (x = 0; x < pDraw->iWidth; x++)
    {
        // Slower to draw this way, but it allows us to use a single buffer
        // instead of drawing and then converting the pixels to be the EPD format
        if (ucSrcMask == 0)
        { // load next source byte
            ucSrcMask = 0x80;
            uc = *s++;
        }
        if (!(uc & ucSrcMask))
        { // black pixel
            d[-(x * 16)] &= ~ucDstMask;
        }
        ucSrcMask >>= 1;
    }
} /* TIFFDraw() */

_attribute_ram_code_ void epd_display_tiff(uint8_t *pData, int iSize)
{
    // test G4 decoder
    memset(epd_buffer, 0xff, epd_buffer_size); // clear to white
    TIFF_openRAW(&tiff, 250, 122, BITDIR_MSB_FIRST, pData, iSize, TIFFDraw);
    TIFF_setDrawParameters(&tiff, 65536, TIFF_PIXEL_1BPP, 0, 0, 250, 122, NULL);
    TIFF_decode(&tiff);
    TIFF_close(&tiff);
    EPD_Display(epd_buffer, epd_buffer_size);
} /* epd_display_tiff() */

_attribute_ram_code_ void epd_display(uint32_t time_is)
{
    if (epd_update_state)
        return;
    obdCreateVirtualDisplay(&obd, 250, 122, epd_temp);
    obdFill(&obd, 0, 0); // fill with white

    char buff[25];
    sprintf(buff, "%02d:%02d:%02d", ((time_is / 60) / 60) % 24, (time_is / 60) % 60, time_is % 60);
    obdWriteStringCustom(&obd, (GFXfont *)&DSEG14_Classic_Mini_Regular_40, 10, 45, (char *)buff, 1);
    obdWriteStringCustom(&obd, (GFXfont *)&Roboto_Black_80, 0, 120, (char *)"Time", 1);
    FixBuffer(epd_temp, epd_buffer);
    EPD_Display(epd_buffer, epd_buffer_size);
} /* epd_display() */

_attribute_ram_code_ void epd_display_char(uint8_t data)
{
    int i;
    for (i = 0; i < epd_buffer_size; i++)
    {
        epd_buffer[i] = data;
    }
    EPD_Display(epd_buffer, epd_buffer_size);
}

_attribute_ram_code_ void epd_clear()
{
    memset(epd_buffer, 0x00, epd_buffer_size);
}
