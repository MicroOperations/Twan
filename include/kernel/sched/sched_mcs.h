#ifndef _SCHED_MCS_H_
#define _SCHED_MCS_H_

#include <std.h>
#include <kernel/sched/sched_conf.h>

u8 __sched_mcs_read_criticality_level(void);
int __sched_mcs_write_criticality_level(u8 criticality);
u8 sched_mcs_read_criticality_level(void);
int sched_mcs_write_criticality_level(u8 criticality);
bool sched_mcs_inc_criticality_level(void);
bool sched_mcs_dec_criticality_level(void);

#endif