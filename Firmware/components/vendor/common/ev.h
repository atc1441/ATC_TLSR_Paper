
#pragma once

#include "stack/ble/ll/ll.h"
#include "stack/ble/blt_config.h"
#include "vendor/common/utlist.h"
#include "./mijia_ble/mijia_ble_api/telink_sdk_mible_api.h"

enum{
	EV_FIRED_EVENT_MAX = 8
};

enum{EV_FIRED_EVENT_MAX_MASK = EV_FIRED_EVENT_MAX - 1};

typedef void (*ev_poll_callback_t)(void);
typedef int (*ev_timer_callback_t)(void *data);
typedef void (*ev_event_callback_t)(void *data);

// file event
typedef struct ev_event_t{
	ev_event_callback_t	cb;
	struct ev_event_t *next;
}ev_event_t;

typedef struct ev_fired_event_t{
	int					e;			//  occupied 8 bytes anyway, use int
	void*				data;
}ev_fired_event_t;

// timer event
typedef struct ev_time_event_t {
    mible_timer_handler 	cb;			// must put this at the beginning, easily initialized
    u32 					t;
    u32 					interval;
    void 					*data;
	mible_timer_mode 		mode;
	struct ev_time_event_t	*next;
#if(__LOG_RT_ENABLE__)
	int						id;			//  for realtime VCD log only
#endif
} ev_time_event_t;

typedef struct ev_poll_t{
	int valid;
	ev_poll_callback_t	cb;
}ev_poll_t;
#if 0
typedef struct ev_loop_ctrl_t{
    ev_event_t 				*events[EV_EVENT_MAX]; /* Registered file events and callback */
/*
	There could be dozens of registered events, may be only several fired.
	Loop a small fired array is more efficient than that loop all then check fired-flag. 
*/	
	ev_fired_event_t		fired_queue[EV_FIRED_EVENT_MAX];
	int						fired_count;
	int						fired_index;
/*
	Pool callbacks can be added and removed frequently. We could use double-linked list instead.
	But it may be less than several vaiable poll events, use array is faster.
*/	
	ev_poll_t				poll[EV_POLL_MAX];
/*
	Time events is sorted, use single linked list
*/	
    ev_time_event_t		*timer_head;
	ev_time_event_t 	*timer_nearest;		// find the nearest fired timer, 
}ev_loop_ctrl_t;
#else
typedef struct ev_loop_ctrl_t{
    ev_event_t 				*events[1]; /* Registered file events and callback */
/*
	There could be dozens of registered events, may be only several fired.
	Loop a small fired array is more efficient than that loop all then check fired-flag. 
*/	
	ev_fired_event_t		fired_queue[1];
	int						fired_count;
	int						fired_index;
/*
	Pool callbacks can be added and removed frequently. We could use double-linked list instead.
	But it may be less than several vaiable poll events, use array is faster.
*/	
	ev_poll_t				poll[1];
/*
	Time events is sorted, use single linked list
*/	
    ev_time_event_t		*timer_head;
	ev_time_event_t 	*timer_nearest;		// find the nearest fired timer, 
}ev_loop_ctrl_t;

#endif 
enum { EV_TIMER_SAFE_MARGIN_US = 40000000, EV_TIMER_SAFE_MARGIN = (EV_TIMER_SAFE_MARGIN_US * CLOCK_SYS_CLOCK_1US) };		// in us,

typedef enum{
	EV_SUSPEND_NOTIFY,
	EV_WAKEUP_NOTIFY,
	EV_KEY_PRESS,
	EV_RF_PKT_RECV,
	EV_PAIRING_START,
	EV_PAIRING_STOP,
	EV_MOUSE_EVENT,
	EV_KEYBOARD_EVENT,
#if(MOUSE_CALI_ENABLE)	
	EV_CALI_START,
	EV_CALI_STOP,
#endif	
	EV_EVENT_MAX,
}ev_event_e;

typedef enum{
	EV_POLL_MOUSE_EVENT,
	EV_POLL_KEYBOARD_EVENT,
	EV_POLL_RF_RECV,
	EV_POLL_DEVICE_PKT,
	EV_POLL_RF_CHN_HOPPING,
	EV_POLL_AUDIO_DEC,
#if(MODULE_EMI_ENABLE)
	EV_POLL_EMI_TEST,
#endif
	EV_POLL_IDLE, //  Must be the last item in ev_poll_e
	EV_POLL_MAX,
}ev_poll_e;	

// will be called in every main loop
void ev_on_poll(ev_poll_e e, ev_poll_callback_t cb);
void ev_enable_poll(ev_poll_e e);
void ev_disable_poll(ev_poll_e e);
void ev_on_event(ev_event_e e, ev_event_t *ev);
// void ev_unon_event(ev_event_e e);
void ev_emit_event(ev_event_e e, void *data);
void ev_emit_event_syn(ev_event_e e, void *data);
void ev_clear_event(void);
void ev_start_timer(ev_time_event_t * e);
void ev_on_timer(ev_time_event_t * e, u32 t_us);
void ev_unon_timer(ev_time_event_t * e);
void ev_reset_timer(ev_time_event_t *e);
void ev_set_timer(ev_time_event_t *e, int t_us);
int ev_timer_expired(ev_time_event_t *e);
void ev_main(void);


