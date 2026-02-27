#ifndef _VSCHED_TIMER_H_
#define _VSCHED_TIMER_H_

#include <types.h>

bool vis_sched_timer_done(void);
void vsched_timer_reload(u32 ticks);
void vsched_arm_timer(u32 ticks);

#endif