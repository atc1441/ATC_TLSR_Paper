#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "drivers/8258/flash.h"
#include "time.h"
#include "main.h"

RAM uint16_t time_trime = 5000;// The higher the number the slower the time runs!, -32,768 to 32,767 
RAM uint32_t one_second_trimmed = CLOCK_16M_SYS_TIMER_CLK_1S;
RAM uint32_t current_unix_time;
RAM uint32_t last_clock_increase;
RAM uint32_t last_reached_period[10] = {0};
RAM uint8_t has_ever_reached[10] = {0};

_attribute_ram_code_ void init_time(void)
{
    one_second_trimmed += time_trime;
    current_unix_time = 0;
}

_attribute_ram_code_ void handler_time(void)
{
    if (clock_time() - last_clock_increase >= one_second_trimmed)
    {
        last_clock_increase += one_second_trimmed;
        current_unix_time++;
    }
}

_attribute_ram_code_ uint8_t time_reached_period(timer_channel ch, uint32_t seconds)
{
    if (!has_ever_reached[ch])
    {
        has_ever_reached[ch] = 1;
        return 1;
    }
    if (current_unix_time - last_reached_period[ch] >= seconds)
    {
        last_reached_period[ch] = current_unix_time;
        return 1;
    }
    return 0;
}

_attribute_ram_code_ void set_time(uint32_t time_now)
{
    current_unix_time = time_now;
}

_attribute_ram_code_ uint32_t get_time(void)
{
    return current_unix_time;
}
