#pragma once

typedef enum
{
    Timer_CH_0 = 0,
    Timer_CH_1,
    Timer_CH_2,
    Timer_CH_3,
    Timer_CH_4,
    Timer_CH_5,
    Timer_CH_6,
    Timer_CH_7,
    Timer_CH_8,
    Timer_CH_9,
} timer_channel;

void init_time(void);
void handler_time(void);
uint8_t time_reached_period(timer_channel ch, uint32_t seconds);
void set_time(uint32_t time_now);
uint32_t get_time(void);