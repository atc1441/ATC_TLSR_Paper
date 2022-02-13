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
#include "font16.h"
#include "font30.h"

RAM uint8_t epd_update_state = 0;

RAM uint8_t epd_temperature_is_read = 0;
RAM uint8_t epd_temperature = 0;

uint8_t epd_buffer[epd_buffer_size];
RAM uint8_t epd_buffer_old[epd_buffer_size];
uint8_t epd_temp[epd_buffer_size]; // for OneBitDisplay to draw into
OBDISP obd;                        // virtual display structure
TIFFIMAGE tiff;

#define _refresh_time 10
uint8_t lut_20_part[] =
    {
        0x20, 0x00, _refresh_time, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t lut_21_part[] =
    {
        0x21, 0x00, _refresh_time, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t lut_22_part[] =
    {
        0x22, 0x80, _refresh_time, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t lut_23_part[] =
    {
        0x23, 0x40, _refresh_time, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t lut_24_part[] =
    {
        0x24, 0x00, _refresh_time, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

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

_attribute_ram_code_ uint8_t EPD_SPI_read(void)
{
    unsigned char i;
    uint8_t value = 0;

    gpio_shutdown(EPD_MOSI);
    gpio_set_output_en(EPD_MOSI, 0);
    gpio_set_input_en(EPD_MOSI, 1);
    gpio_write(EPD_CS, 0);
    EPD_ENABLE_WRITE_DATA();
    WaitUs(10);
    for (i = 0; i < 8; i++)
    {
        value <<= 1;
        if (gpio_read(EPD_MOSI) != 0)
        {
            value |= 1;
        }
        gpio_write(EPD_CLK, 1);
        gpio_write(EPD_CLK, 0);
    }
    gpio_set_output_en(EPD_MOSI, 1);
    gpio_set_input_en(EPD_MOSI, 0);
    gpio_write(EPD_CS, 1);
    return value;
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

_attribute_ram_code_ void _send_lut(uint8_t lut[])
{
    EPD_WriteCmd(lut[0]);
    for (int r = 1; r <= sizeof(lut_20_part); r++)
    {
        EPD_WriteData(lut[r]);
    }
}

_attribute_ram_code_ void _send_empty_lut(uint8_t lut)
{
    EPD_WriteCmd(lut);
    for (int r = 0; r <= 260; r++)
        EPD_WriteData(0x00);
}

_attribute_ram_code_ void EPD_LoadImage(unsigned char *image, int size)
{
    int i;
    EPD_WriteCmd(0x13);
    for (i = 0; i < size; i++)
    {
        EPD_WriteData(image[i]);
    }
    WaitMs(2);
}

_attribute_ram_code_ void init_epd(void)
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

    gpio_set_output_en(EPD_ENABLE, 0);
    gpio_set_input_en(EPD_ENABLE, 1);
    gpio_setup_up_down_resistor(EPD_ENABLE, PM_PIN_PULLUP_1M);
}

_attribute_ram_code_ uint8_t EPD_read_temp(void)
{
    if (epd_temperature_is_read)
        return epd_temperature;

    init_epd();
    // system power
    EPD_POWER_ON();
    // Reset the EPD driver IC
    gpio_write(EPD_RESET, 0);
    WaitMs(10);
    gpio_write(EPD_RESET, 1);
    WaitMs(10);
    // power on
    EPD_WriteCmd(0x04);

    // check BUSY pin
    EPD_CheckStatus();

    EPD_WriteCmd(0x40);
    epd_temperature = EPD_SPI_read();
    EPD_SPI_read();
    epd_temperature_is_read = 1;

    // power off
    EPD_WriteCmd(0x02);

    // deep sleep
    EPD_WriteCmd(0x07);
    EPD_WriteData(0xa5);

    EPD_POWER_OFF();

    return epd_temperature;
}

_attribute_ram_code_ void EPD_Display(unsigned char *image, int size, uint8_t full_or_partial)
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

    EPD_WriteCmd(0x40);
    epd_temperature = EPD_SPI_read();
    EPD_SPI_read();
    epd_temperature_is_read = 1;

    // panel setting
    EPD_WriteCmd(0x00);
    if (full_or_partial)
        EPD_WriteData(0b00011111);
    else
        EPD_WriteData(0b00111111);
    EPD_WriteData(0x0f);

    // resolution setting
    EPD_WriteCmd(0x61);
    EPD_WriteData(0x80);
    EPD_WriteData(0x01);
    EPD_WriteData(0x28);

    // Vcom and data interval setting
    EPD_WriteCmd(0X50);
    EPD_WriteData(0x97);

    if (!full_or_partial)
    {
        _send_lut(lut_20_part);
        _send_empty_lut(0x21); //_send_lut(lut_21_part);
        _send_lut(lut_22_part);
        _send_lut(lut_23_part);
        _send_empty_lut(0x24); //_send_lut(lut_24_part);

        EPD_WriteCmd(0x10);
        int i;
        for (i = 0; i < size; i++)
        {
            EPD_WriteData(epd_buffer_old[i]);
        }
    }
    // load image data to EPD
    EPD_LoadImage(image, size);

    // trigger display refresh
    EPD_WriteCmd(0x12);
    memcpy(epd_buffer_old, epd_buffer, size);

    epd_update_state = 1;
}

_attribute_ram_code_ void epd_set_sleep(void)
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
    epd_update_state = 0;
}

_attribute_ram_code_ uint8_t epd_state_handler(void)
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
    EPD_Display(epd_buffer, epd_buffer_size, 1);
} /* epd_display_tiff() */

_attribute_ram_code_ void epd_display(uint32_t time_is, uint16_t battery_mv, int16_t temperature, uint8_t full_or_partial)
{
    if (epd_update_state)
        return;
    obdCreateVirtualDisplay(&obd, 250, 122, epd_temp);
    obdFill(&obd, 0, 0); // fill with white

    char buff[25];
    sprintf(buff, "%02d:%02d", ((time_is / 60) / 60) % 24, (time_is / 60) % 60);
    obdWriteStringCustom(&obd, (GFXfont *)&Special_Elite_Regular_30, 10, 45, (char *)buff, 1);
    sprintf(buff, "Temperature %d C", temperature);
    obdWriteStringCustom(&obd, (GFXfont *)&Dialog_plain_16, 10, 80, (char *)buff, 1);
    sprintf(buff, "EPD Temperature %d C", EPD_read_temp());
    obdWriteStringCustom(&obd, (GFXfont *)&Dialog_plain_16, 10, 100, (char *)buff, 1);
    sprintf(buff, "Battery %dmV", battery_mv);
    obdWriteStringCustom(&obd, (GFXfont *)&Dialog_plain_16, 10, 120, (char *)buff, 1);
    FixBuffer(epd_temp, epd_buffer);
    EPD_Display(epd_buffer, epd_buffer_size, full_or_partial);
} /* epd_display() */

_attribute_ram_code_ void epd_display_char(uint8_t data)
{
    int i;
    for (i = 0; i < epd_buffer_size; i++)
    {
        epd_buffer[i] = data;
    }
    EPD_Display(epd_buffer, epd_buffer_size, 1);
}

_attribute_ram_code_ void epd_clear(void)
{
    memset(epd_buffer, 0x00, epd_buffer_size);
}
