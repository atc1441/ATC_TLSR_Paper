#pragma once

//#define epd_height 128 // fix for displaying image on 213_bwr
//#define epd_width 250 // fix for displaying image on 213_bwr
#define epd_height 200
#define epd_width 200
#define epd_buffer_size ((epd_height/8) * epd_width)

void set_EPD_model(uint8_t model_nr);
void init_epd(void);
int8_t EPD_read_temp(void);
void EPD_Display(unsigned char *image, int size, uint8_t full_or_partial);
void epd_display_tiff(uint8_t *pData, int iSize);
void epd_display(uint32_t time_is, uint16_t battery_mv, int16_t temperature, uint8_t full_or_partial);
void epd_set_sleep(void);
uint8_t epd_state_handler(void);
void epd_display_char(uint8_t data);
void epd_clear(void);
