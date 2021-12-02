#ifndef __PT_MISC_H__
#define __PT_MISC_H__
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*** Pseudo timer ***/
typedef struct {
    uint32_t * p_curr_tick;
    uint32_t tick_interval_ms;
    uint32_t start_tick;
    uint32_t timeout_tick;
} pstimer_t;

void pstimer_set(pstimer_t * t, uint32_t timeout_ms);

int pstimer_expired(pstimer_t * t, void (*timeout_handler)(void));

#ifdef __cplusplus
}
#endif

/** @} */

#endif  /* __PT_MISC_H__ */ 
