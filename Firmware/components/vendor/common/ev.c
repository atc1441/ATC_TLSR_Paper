
#include "ev.h"
static ev_loop_ctrl_t ev_loop, *loop = &ev_loop;
// will be called in every main loop
void ev_on_poll(ev_poll_e e, ev_poll_callback_t cb){
	loop->poll[e].valid = 1;
	loop->poll[e].cb = cb;
}
/*	
// no need to use unregister by now, use ev_disable_poll instead
void ev_unon_poll(ev_poll_e e){
	loop->poll[e].valid = 0;
}
*/
void ev_enable_poll(ev_poll_e e){
	loop->poll[e].valid = 1;
}

void ev_disable_poll(ev_poll_e e){
	loop->poll[e].valid = 0;
}

/* Process poll */
void ev_poll() {
	int i;
	for(i = 0; i < EV_POLL_MAX; ++i){
		if(loop->poll[i].valid){
#if(__LOG_RT_ENABLE__)
			if(i < TR_T_POLL_E - TR_T_POLL_0){
				LOG_TICK((TR_T_POLL_0 + i), loop->poll[i].cb());
			}else
#endif
			loop->poll[i].cb();
		}
	}
}
void ev_on_event(ev_event_e e, ev_event_t *ev){
	assert(ev->next == 0);	// !! not allow to add different e with the same ev
	LL_PREPEND((loop->events[e]), ev);
}

void ev_unon_event(ev_event_e e, ev_event_t *ev){
	ev_event_t *head = loop->events[e];
	LL_DELETE(head, ev);
	loop->events[e] = head;
	ev->next = 0;			// !! must,  allow to re-add
}

void ev_emit_event(ev_event_e e, void *data){
	STATIC_ASSERT_POW2(EV_FIRED_EVENT_MAX);
	STATIC_ASSERT(EV_FIRED_EVENT_MAX == EV_FIRED_EVENT_MAX_MASK + 1);
	assert(!irq_is_in_handler());

	u8 r = irq_disable();
	
	int c = (loop->fired_index + loop->fired_count) & EV_FIRED_EVENT_MAX_MASK;
	loop->fired_queue[c].e = (int)e;
	loop->fired_queue[c].data = data;
	++loop->fired_count;

	irq_restore(r);
}

static inline void ev_call_callbacks(int e, void *data){
	ev_event_t *ev;
	ev_event_t *head = loop->events[e];
    for(ev = head; ev; ev = ev->next){
		ev->cb(data);
	}
}
void ev_emit_event_syn(ev_event_e e, void *data){
	//u8 r = irq_disable();
#if(__LOG_RT_ENABLE__)
	if(e < TR_T_EVENT_E - TR_T_EVENT_0){
		LOG_TICK(TR_T_EVENT_0 + e, ev_call_callbacks((int)e, data));
	}else
#endif
	ev_call_callbacks((int)e, data);
	//irq_restore(r);
}

void ev_clear_event(void){
	loop->fired_count = 0;
}

void ev_process_event(){
	int i;
	int fired_count = loop->fired_count;	//  cache count, because p->cb will emit new events, will change loop->fired_count
	int end_index = loop->fired_index + fired_count;
	for(i = loop->fired_index; i < end_index; ++i){

		int j = i & EV_FIRED_EVENT_MAX_MASK;

		ev_fired_event_t *fe = &loop->fired_queue[j];
#if(__LOG_RT_ENABLE__)
		if(fe->e < TR_T_EVENT_E - TR_T_EVENT_0){
			LOG_TICK((TR_T_EVENT_0 + fe->e), ev_call_callbacks(fe->e, fe->data));					// may emit new event
		}else
#endif
		ev_call_callbacks(fe->e, fe->data);
	}
	u8 r = irq_disable();
	loop->fired_count -= fired_count;		// loop->fired_count may bring up race condition, if ev_emit_event is called in irq_handler 
	irq_restore(r);
	loop->fired_index = (loop->fired_index + fired_count) & EV_FIRED_EVENT_MAX_MASK;
}

// calulate how much time elapse till the timer fired
static u32 inline ev_cal_timer_distant(u32 t, u32 now){
	if((u32)(now - t) < EV_TIMER_SAFE_MARGIN)
		return 0;
	else
		return (u32)(t - now);
}

// to tell which fired first
// return < 0,  e1 fired first
// return == ,  e1 and e2 fired the same time
// return > 0,  e2 fired first
static int ev_cmp_timer(ev_time_event_t *e1, ev_time_event_t *e2, u32 now){
	return (int)(ev_cal_timer_distant(e1->t, now) - ev_cal_timer_distant(e2->t, now));
}

void ev_reset_timer(ev_time_event_t *e){
	e->t = clock_time() + e->interval;	// becare of overflow
}

void ev_set_timer(ev_time_event_t *e, int t_us){
	//assert(t_us < (CLOCK_MAX_US - EV_TIMER_SAFE_MARGIN_US));	// about 143000 in 30M clock, about 2 minutes
	e->interval = t_us * CLOCK_SYS_CLOCK_1US;
	ev_reset_timer(e);
}

static int inline ev_is_timer_expired(ev_time_event_t *e, u32 now){
	return ((u32)(now - e->t) < EV_TIMER_SAFE_MARGIN);
}

int ev_timer_expired(ev_time_event_t *e){
	u32 now = clock_time();
	return ev_is_timer_expired(e, now);
}

static ev_time_event_t *ev_search_nearest_timer()
{
    ev_time_event_t *te = loop->timer_head;
    ev_time_event_t *nearest = 0;
	u32 now = clock_time();

    while(te) {
		if(!nearest || ev_cmp_timer(te, nearest, now) < 0){
			nearest = te;
		}
        te = te->next;
    }
    return nearest;
}

u8 ev_timer_exist(const ev_time_event_t * e){
	ev_time_event_t *te = loop->timer_head;
	while(te){
	    if (te == e)
	        return 1;
	    te = te->next;
	}
	return 0;
}

#if(__LOG_RT_ENABLE__)
int	ev_ttl_timer_id = 0;			//  for realtime VCD log only
#endif
// !!!  	if the callback return < 0,  delete the timer
// 		if the callback return 0,  keep the original interval
// 		if the callback return > 0,  set new interval
void ev_start_timer(ev_time_event_t * e){
	// Reserve  4 second margin in case some event run too long
	// that is even a task run nearly 4 second, 
	// the timers will be fired correctly after then.
	u8 r = irq_disable();
	
	u32 now = clock_time();

	u32 t = now + e->interval;	// becare of overflow
    
	// add to timer list
	ev_time_event_t * out;
	LL_EXIST(loop->timer_head, e, out);
	if(out){
	    out->t = t;
	}else{
		e->t = t;
		LL_PREPEND(loop->timer_head, e);
	}
	
	// check if e will be the nearest timer
	// "Use sorted list or skiplist to improve performance"
	if(!loop->timer_nearest || ev_cmp_timer(e, loop->timer_nearest, now) < 0){
		loop->timer_nearest = e;
	}
#if(__LOG_RT_ENABLE__)
	// start from 1,  0 is reserved to indicate that has not been added
	if(0 == e->id){
		e->id = (++ev_ttl_timer_id);
		log_data(TR_24_TIMERS_ADDR + e->id, (u32)e);	// to show relation of id and the address
	}
#endif	
	irq_restore(r);
}

void ev_on_timer(ev_time_event_t * e, u32 t_us){
	//assert(t_us < (CLOCK_MAX_US - EV_TIMER_SAFE_MARGIN_US));	// about 143000 in 30M clock, about 2 minutes
    e->interval = t_us * CLOCK_SYS_CLOCK_1US;
	ev_start_timer(e);
}

void ev_unon_timer(ev_time_event_t * e){
	if(e==NULL){
		return ;
	}
    LL_DELETE(loop->timer_head, e);
}

/* Process time events */
static void ev_process_timer(){
	u32 now = clock_time();
	if(!loop->timer_nearest || !ev_is_timer_expired(loop->timer_nearest, now))
		return;
	
	ev_time_event_t *te = loop->timer_head;
	ev_time_event_t *prev_head = te;
	while(te){
		if(ev_is_timer_expired(te, now)){
			int t =0;

#if(__LOG_RT_ENABLE__)
			if(te->id < TR_T_TIMER_E){
				LOG_TICK(te->id, t = te->cb(te->data));
			}else
#endif
			#if 0
			t = te->cb(te->data);
			#else
			te->cb(te->data);
			if(te->mode == MIBLE_TIMER_SINGLE_SHOT){
				t = -1;
			}else if (te->mode == MIBLE_TIMER_REPEATED){
				t = 0;
			}
			#endif 
			if(t < 0){
				//ev_unon_timer(te);		// delete timer
			}else if(0 == t){
				te->t = now + te->interval;	// becare of overflow
			}else{
				te->interval = t * CLOCK_SYS_CLOCK_1US;
				te->t = now + te->interval;	// becare of overflow
			}
			if(prev_head != loop->timer_head){
				// restart the whole from timer_head.  because the head could be changed within the loop
				te = loop->timer_head;
				prev_head = te;
			}else{
				te = te->next;
			}
		}else{
			te = te->next;
		}
	}
	// recalculate the nearest timer
	loop->timer_nearest = ev_search_nearest_timer();
	
}

void ev_main(void){
	ev_process_timer();
	//ev_poll();
	//ev_process_event();
}

