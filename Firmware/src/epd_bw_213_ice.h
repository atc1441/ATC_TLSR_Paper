#pragma once

uint8_t EPD_BW_213_ice_detect(void);
int8_t EPD_BW_213_ice_read_temp(void);
int8_t EPD_BW_213_ice_Display(unsigned char *image, int size, uint8_t full_or_partial);
void EPD_BW_213_ice_set_sleep(void);
