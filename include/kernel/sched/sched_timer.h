#ifndef _SCHED_TIMER_H_
#define _SCHED_TIMER_H_

#include <subsys/twanvisor/vsched/vcpu.h>
#include <types.h>

#define PV_SCHED_TIMER 0
STATIC_ASSERT(VNUM_VTIMERS >= 1);

void sched_timer_init(u32 time_slice_ms);
void sched_timer_disable(void);
void sched_timer_enable(void);
bool sched_is_timer_pending(void);

#endif