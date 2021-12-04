#include <stdint.h>
#include "tl_common.h"
#include "main.h"
#include "epd.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "battery.h"

#include "OneBitDisplay.h"
extern const uint8_t ucMirror[];
#include "Roboto_Black_80.h"

#define epd_height 150
#define epd_width 250
#define epd_buffer_size (epd_height*epd_width)/8

RAM uint8_t epd_buffer[epd_buffer_size];
RAM uint8_t epd_temp[epd_buffer_size]; // for OneBitDisplay to draw into
RAM OBDISP obd; // virtual display structure

#define     EPD_POWER_ON()     gpio_write(EPD_ENABLE,0)

#define     EPD_POWER_OFF()    gpio_write(EPD_ENABLE,1)

#define EPD_ENABLE_WRITE_CMD()   gpio_write(EPD_DC,0)
#define EPD_ENABLE_WRITE_DATA()  gpio_write(EPD_DC,1)

#define EPD_IS_BUSY()            (!gpio_read(EPD_BUSY))

void EPD_DLY_LP(unsigned int ms)
{
	cpu_sleep_wakeup(SUSPEND_MODE, PM_WAKEUP_TIMER, ClockTime()+(ms-40)*1000*sys_tick_per_us);
	WaitMs(40);
}

void EPD_SPI_Write(unsigned char value)
{
	unsigned char i;

	WaitUs(10);
	for(i=0; i<8; i++)
	{
	gpio_write(EPD_CLK,0);
	if(value & 0x80){
	gpio_write(EPD_MOSI,1);
	}
	else{
	gpio_write(EPD_MOSI,0);
	}
	value = (value << 1);
	gpio_write(EPD_CLK,1);
	}
}

void EPD_WriteCmd(unsigned char cmd)
{
    gpio_write(EPD_CS,0);
    EPD_ENABLE_WRITE_CMD();
    EPD_SPI_Write(cmd);
    gpio_write(EPD_CS,1);
}

void EPD_WriteData(unsigned char data)
{
    gpio_write(EPD_CS,0);
    EPD_ENABLE_WRITE_DATA();
    EPD_SPI_Write(data);
    gpio_write(EPD_CS,1);
}

void EPD_CheckStatus(void)
{
    while (EPD_IS_BUSY()) {
    }
}

void EPD_LoadImage(unsigned char *image, int size)
{
    int i;
    EPD_WriteCmd(0x10);
    for (i = 0; i < size; i++) {
        EPD_WriteData(0xff);
    }

    EPD_WriteCmd(0x13);
    for (i = 0; i < size; i++) {
        EPD_WriteData(image[i]);
    }
    WaitMs(2);
}

void init_epd(){
    //reset pin configuration
	gpio_set_func(EPD_RESET,AS_GPIO);
    gpio_set_output_en(EPD_RESET, 1);
    gpio_set_input_en(EPD_RESET, 0);
    gpio_write(EPD_RESET, 0);
    //BS1 pin configuration
    //EPD power supply control pin configuration
    gpio_set_func(EPD_ENABLE,AS_GPIO);
    gpio_set_output_en(EPD_ENABLE, 1);
    gpio_set_input_en(EPD_ENABLE, 0);
    gpio_write(EPD_ENABLE, 1);
    //EPD Data/Command control pin configuration
    gpio_set_func(EPD_DC,AS_GPIO);
    gpio_set_output_en(EPD_DC, 1);
    gpio_set_input_en(EPD_DC, 0);
    gpio_write(EPD_DC, 0);
    //EPD Busy Status pin configuratioin
    gpio_set_func(EPD_BUSY,AS_GPIO);
    gpio_set_output_en(EPD_BUSY, 0);
    gpio_set_input_en(EPD_BUSY, 1);
    gpio_setup_up_down_resistor(EPD_BUSY,PM_PIN_PULLUP_1M);

    //cs pin configuration
    gpio_set_func(EPD_CS,AS_GPIO);
    gpio_set_output_en(EPD_CS, 1);
    gpio_set_input_en(EPD_CS, 0);
    gpio_write(EPD_CS, 1);
    //clk pin configuration
    gpio_set_func(EPD_CLK,AS_GPIO);
    gpio_set_output_en(EPD_CLK, 1);
    gpio_set_input_en(EPD_CLK, 0);
    gpio_write(EPD_CLK, 0);
    //DO pin configuration
    gpio_set_func(EPD_MOSI,AS_GPIO);
    gpio_set_output_en(EPD_MOSI, 1);
    gpio_set_input_en(EPD_MOSI, 0);
    gpio_write(EPD_MOSI, 1);
}

void deinit_epd(void)
{
   // GPIO_AllTurnOff();
	gpio_set_func(EPD_RESET,AS_GPIO);
	gpio_setup_up_down_resistor(EPD_RESET,PM_PIN_PULLUP_10K);
    gpio_set_output_en(EPD_RESET, 0);
    gpio_set_input_en(EPD_RESET, 1);
    gpio_setup_up_down_resistor(EPD_RESET,PM_PIN_PULLUP_10K);
    //EPD power supply control pin configuration
    gpio_set_func(EPD_ENABLE,AS_GPIO);
    gpio_setup_up_down_resistor(EPD_ENABLE,PM_PIN_PULLDOWN_100K);
    gpio_set_output_en(EPD_ENABLE, 0);
    gpio_set_input_en(EPD_ENABLE, 1);
    gpio_setup_up_down_resistor(EPD_ENABLE,PM_PIN_PULLDOWN_100K);
    //EPD Data/Command control pin configuration
    gpio_set_func(EPD_DC,AS_GPIO);
    gpio_set_output_en(EPD_DC, 0);
    gpio_set_input_en(EPD_DC, 1);
    gpio_setup_up_down_resistor(EPD_DC,PM_PIN_PULLUP_1M);
    //EPD Busy Status pin configuratioin
    gpio_set_func(EPD_BUSY,AS_GPIO);
    gpio_set_output_en(EPD_BUSY, 0);
    gpio_set_input_en(EPD_BUSY, 1);
    gpio_setup_up_down_resistor(EPD_BUSY,PM_PIN_PULLDOWN_100K); //turn off pull-up resistor
    //Turn off all SPI IOs
    gpio_set_func(EPD_CS,AS_GPIO);
    gpio_set_output_en(EPD_CS, 0);
    gpio_set_input_en(EPD_CS, 1);
    gpio_setup_up_down_resistor(EPD_CS,PM_PIN_PULLUP_1M);
    gpio_set_func(EPD_CLK,AS_GPIO);
    gpio_set_output_en(EPD_CLK, 0);
    gpio_set_input_en(EPD_CLK, 1);
    gpio_setup_up_down_resistor(EPD_CLK,PM_PIN_PULLDOWN_100K);
    gpio_set_func(EPD_MOSI,AS_GPIO);
    gpio_set_output_en(EPD_MOSI, 0);
    gpio_set_input_en(EPD_MOSI, 1);
    gpio_setup_up_down_resistor(EPD_MOSI,PM_PIN_PULLDOWN_100K);
}

void EPD_Display(unsigned char *image, int size)
{	
    init_epd();
	//system power
    EPD_POWER_ON();
    //Reset the EPD driver IC
    gpio_write(EPD_RESET,0);
    EPD_DLY_LP(1000);
    gpio_write(EPD_RESET,1);
    EPD_DLY_LP(1000);
    //Booster soft start
    EPD_WriteCmd(0x06);
    EPD_WriteData(0x17);
    EPD_WriteData(0x17);
    EPD_WriteData(0x17);
    //power on
    EPD_WriteCmd(0x04);

    //check BUSY pin
    EPD_CheckStatus();

    //panel setting
    EPD_WriteCmd(0x00);
    EPD_WriteData(0x1f);
    EPD_WriteData(0x0f);

    //resolution setting
    EPD_WriteCmd(0x61);
    EPD_WriteData(0x80);
    EPD_WriteData(0x01);
    EPD_WriteData(0x28);

    //Vcom and data interval setting
    EPD_WriteCmd(0X50);
    EPD_WriteData(0x97);

    //load image data to EPD
    EPD_LoadImage(image, size);

    //trigger display refresh
    EPD_WriteCmd(0x12);
    EPD_DLY_LP(100);
    //check BUSY pin
    EPD_CheckStatus();

    //Vcom and data interval setting
    EPD_WriteCmd(0X50);
    EPD_WriteData(0xf7);

    //power off
    EPD_WriteCmd(0X02);

    //deep sleep
    EPD_WriteCmd(0X07);
    EPD_WriteData(0xa5);

    EPD_POWER_OFF();
    deinit_epd();
}

void FixBuffer(uint8_t *pSrc, uint8_t *pDst)
{
	int x, y;
	uint8_t *s, *d;
	for (y=0; y<16; y++) { // byte rows
		d = &pDst[y];
		s = &pSrc[y * 250];
		for (x=0; x<250; x++) {
			d[x*16] = ~ucMirror[s[249-x]]; // invert and flip
		} // for x
	} // for y
}
void epd_display(){
	obdCreateVirtualDisplay(&obd, 250, 122, epd_temp);
	obdFill(&obd, 0, 0); // fill with white
	obdWriteStringCustom(&obd, (GFXfont *)&Roboto_Black_80, 0, 60, (char *)"Roboto", 1);
	obdWriteStringCustom(&obd, (GFXfont *)&Roboto_Black_80, 0, 120, (char *)"12:31pm", 1);
	FixBuffer(epd_temp, epd_buffer);
	EPD_Display(epd_buffer, epd_buffer_size);
}

void epd_display_char(uint8_t data){
    int i;
    for(i=0;i<epd_buffer_size;i++){
        epd_buffer[i]=data;
    }
	EPD_Display(epd_buffer, epd_buffer_size);
}

void epd_clear(){
	memset(epd_buffer,0x00,epd_buffer_size);
}
