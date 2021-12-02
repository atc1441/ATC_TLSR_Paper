#include "third_party/pt/pt_misc.h"

#ifndef NULL
#define NULL 0
#endif

void pstimer_set(pstimer_t * t, uint32_t timeout_ms)
{
    t->timeout_tick = timeout_ms / t->tick_interval_ms;
    t->start_tick = t->p_curr_tick[0];
}

int pstimer_expired(pstimer_t * t, void (*handler)(void))
{
    int expired = t->start_tick + t->timeout_tick <= t->p_curr_tick[0];
    if (expired == true && handler != NULL)
        handler();
    return expired;
}

