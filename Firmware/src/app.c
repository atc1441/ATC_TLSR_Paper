#include <stdint.h>
#include "tl_common.h"
#include "app.h"
#include "main.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_common.h"

#include "battery.h"
#include "ble.h"
#include "flash.h"

RAM uint32_t last_delay = 0xFFFF0000, last_adv_delay = 0xFFFF0000, last_battery_delay = 0xFFFF0000;
RAM bool last_smiley;
int16_t temp = 0;
uint16_t humi = 0;
RAM uint8_t adv_count = 0;
RAM uint8_t meas_count = 254;
RAM int16_t last_temp;
RAM uint16_t last_humi;
RAM uint8_t battery_level;
RAM uint16_t battery_mv;
RAM bool show_batt_or_humi;

// Settings
extern settings_struct settings;

void user_init_normal(void)
{                            // this will get executed one time after power up
    random_generator_init(); // must
    init_ble();
    init_flash();
    battery_mv = get_battery_mv();
    battery_level = get_battery_level(battery_mv);
    epd_display();
}

_attribute_ram_code_ void user_init_deepRetn(void)
{ // after sleep this will get executed
    blc_ll_initBasicMCU();
    rf_set_power_level_index(RF_POWER_P3p01dBm);
    blc_ll_recoverDeepRetention();
}

_attribute_ram_code_ void main_loop()
{
    set_led_color(1);
    blt_sdk_main_loop();

    if (epd_state_handler()) // if epd_update is ongoing make a gpio wakeup to put it so sleep as fast as possible
    {
        cpu_set_gpio_wakeup(EPD_BUSY, 1, 1);
        bls_pm_setWakeupSource(PM_WAKEUP_PAD);
    }
    deinit_nfc();
    deinit_led();
    //gpio_shutdown(GPIO_ALL);
    blt_pm_proc();
}
