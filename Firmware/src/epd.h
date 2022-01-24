#pragma once

#define epd_height 128
#define epd_width 250
#define epd_buffer_size ((epd_height/8) * epd_width)

void EPD_DLY_LP(unsigned int ms);
void EPD_SPI_Write(unsigned char value);
void EPD_WriteCmd(unsigned char cmd);
void EPD_WriteData(unsigned char data);
void EPD_CheckStatus(void);
void EPD_LoadImage(unsigned char *image, int size);
void init_epd();
void deinit_epd(void);
void EPD_Display(unsigned char *image, int size);
void epd_display_tiff(uint8_t *pData, int iLen);
void epd_display_cmds(uint8_t *pData, int iLen);
void epd_display(uint32_t time_is);
void epd_set_sleep();
uint8_t epd_state_handler();
void epd_display_char(uint8_t data);
void epd_clear();
