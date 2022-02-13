#pragma once

typedef struct Settings_struct
{
	uint32_t magic;
	uint32_t len;
	uint8_t temp_C_or_F;
	uint8_t advertising_temp_C_or_F;
	uint8_t blinking_smiley;
	uint8_t comfort_smiley;
	uint8_t show_batt_enabled;
	uint8_t advertising_interval;//advise new values - multiply by 10 for value
	uint8_t measure_interval;//time = loop interval * factor (def: about 7 * X)
	int8_t temp_offset;
	uint8_t temp_alarm_point;//divide by ten for value
	uint8_t crc;// Needs to be at the last position otherwise the settings can not be validated on next boot!!!!
} settings_struct;


void init_flash(void);
void reset_settings_to_default(void);
void save_settings_to_flash(void);
uint8_t get_crc(void);
