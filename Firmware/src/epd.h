#pragma once

#define epd_height 128
#define epd_width 250
#define epd_buffer_size ((epd_height/8) * epd_width)

void EPD_DLY_LP(unsigned int ms);
void EPD_SPI_Write(unsigned char value);
uint8_t EPD_SPI_read(void);
void EPD_WriteCmd(unsigned char cmd);
void EPD_WriteData(unsigned char data);
void EPD_CheckStatus(void);
void EPD_LoadImage(unsigned char *image, int size);
void init_epd(void);
uint8_t EPD_read_temp(void);
void EPD_Display(unsigned char *image, int size, uint8_t full_or_partial);
void epd_display_tiff(uint8_t *pData, int iSize);
void epd_display(uint32_t time_is, uint16_t battery_mv, int16_t temperature, uint8_t full_or_partial);
void epd_set_sleep(void);
uint8_t epd_state_handler(void);
void epd_display_char(uint8_t data);
void epd_clear(void);