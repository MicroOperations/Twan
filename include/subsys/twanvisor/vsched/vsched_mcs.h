#ifndef _VSCHED_MCS_H_
#define _VSCHED_MCS_H_

#include <subsys/twanvisor/vsched/vsched_conf.h>

#define VSCHED_MIN_CRITICALITY 0
#define VSCHED_MAX_CRITICALITY (VSCHED_NUM_CRITICALITIES - 1)

u8 __vsched_mcs_read_criticality_level_local(void);
int __vsched_mcs_write_criticality_level_local(u8 criticality);

u8 vsched_mcs_read_criticality_level(u32 vprocessor_id);
int vsched_mcs_write_criticality_level(u32 vprocessor_id, u8 criticality);

#endif