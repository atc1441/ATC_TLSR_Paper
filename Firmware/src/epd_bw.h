#pragma once

uint8_t EPD_BW_read_temp(void);
uint8_t EPD_BW_Display(unsigned char *image, int size, uint8_t full_or_partial);
void EPD_BW_set_sleep(void);