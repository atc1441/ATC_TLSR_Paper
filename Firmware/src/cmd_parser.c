#include <stdint.h>
#include "main.h"
#include "epd.h"
#include "tl_common.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"

#include "flash.h"

extern void doEPD();
extern settings_struct settings;

#define testPin GPIO_PD3
void cmd_parser(void * p){
	rf_packet_att_data_t *req = (rf_packet_att_data_t*)p;
	uint8_t inData = req->dat[0];
	if(inData == 0xFF){
	gpio_set_func(testPin, AS_GPIO);
	gpio_set_output_en(testPin, 1);
	gpio_set_input_en(testPin, 0); 
	sleep_ms(500);
	}else if(inData == 0xCC){
	gpio_set_func(GPIO_PD2, AS_GPIO);
	gpio_set_output_en(GPIO_PD2, 1);
	gpio_set_input_en(GPIO_PD2, 0); 
	sleep_ms(500);
	}else if(inData == 0x0F){
		settings.advertising_temp_C_or_F = true;//Advertising Temp in F
	}else if(inData == 0x0C){
		settings.advertising_temp_C_or_F = false;//Advertising Temp in C
	}else if(inData == 0xB1){
		epd_display();
	}else if(inData == 0xB0){
		settings.show_batt_enabled = false;//Disable battery on LCD
	}else if(inData == 0xA0){
		settings.blinking_smiley = false;
		settings.comfort_smiley = false;
	}else if(inData == 0xA1){
		settings.blinking_smiley = false;
		settings.comfort_smiley = false;
	}else if(inData == 0xA2){ 
		settings.blinking_smiley = false;
		settings.comfort_smiley = false;
	}else if(inData == 0xA3){
		settings.blinking_smiley = false;
		settings.comfort_smiley = true; // Comfort Indicator
	}else if(inData == 0xAB){
		settings.blinking_smiley = true;//Smiley blinking
	}else if(inData == 0xAE){
		settings.advertising_type = false;//Advertising type Custom
	}else if(inData == 0xAF){
		settings.advertising_type = true;//Advertising type Mi Like
	}else if(inData == 0xFE){
		settings.advertising_interval = req->dat[1];//Set advertising interval with second byte, value*10second / 0=main_delay
	}else if(inData == 0xFA){
		settings.temp_offset = req->dat[1];//Set temp offset, -12,5 - +12,5 °C
	}else if(inData == 0xFB){
		settings.humi_offset = req->dat[1];//Set humi offset, -50 - +50 %
		if(settings.humi_offset<-50)settings.humi_offset=-50;
		if(settings.humi_offset>50)settings.humi_offset=50;
	}else if(inData == 0xFC){
		settings.temp_alarm_point = req->dat[1];//Set temp alarm point value divided by 10 for temp in °C
		if(settings.temp_alarm_point==0)settings.temp_alarm_point = 1;
	}else if(inData == 0xFD){
		settings.humi_alarm_point = req->dat[1];//Set humi alarm point
		if(settings.humi_alarm_point==0)settings.humi_alarm_point = 1;
		if(settings.humi_alarm_point>50)settings.humi_alarm_point = 50;
	}else if(inData == 0xDD){// Set display segments directly
	
	}else if(inData == 0xDE){// Save settings in flash to default
		reset_settings_to_default();
		save_settings_to_flash();
	}else if(inData == 0xDF){// Save current settings in flash
		save_settings_to_flash();
	}
}
