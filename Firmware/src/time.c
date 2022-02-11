#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "drivers/8258/flash.h"
#include "time.h"
#include "main.h"

RAM uint32_t current_unix_time;
RAM uint32_t last_clock_increase;
RAM uint32_t last_reached_period;

_attribute_ram_code_ void init_time()
{
    current_unix_time = 0;
}

_attribute_ram_code_ void handler_time()
{
    if (clock_time() - last_clock_increase >= CLOCK_16M_SYS_TIMER_CLK_1S)
    {
        last_clock_increase += CLOCK_16M_SYS_TIMER_CLK_1S;
        current_unix_time++;
    }
}

_attribute_ram_code_ uint8_t time_reached_period(uint32_t seconds)
{
    if (current_unix_time - last_reached_period >= seconds)
    {
        last_reached_period = current_unix_time;
        return 1;
    }
    return 0;
}

_attribute_ram_code_ void set_time(uint32_t time_now)
{
    current_unix_time = time_now;
}
