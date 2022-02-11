#pragma once

void init_time();
void handler_time();
uint8_t time_reached_period(uint32_t seconds);
void set_time(uint32_t time_now);