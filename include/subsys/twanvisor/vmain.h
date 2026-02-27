#ifndef _VMAIN_H_
#define _VMAIN_H_

#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/vextern.h>

void lapic_reconfig(struct vper_cpu *vthis_cpu);

void vper_cpu_flags_init(struct vper_cpu *vthis_cpu);
int vper_cpu_data_init(struct vper_cpu *vthis_cpu, u32 vprocessor_id);

int root_partition_init(void);
void root_vcpu_init(struct vper_cpu *vthis_cpu, u32 vprocessor_id);

void __do_virtualise_core(u32 vprocessor_id, u64 rip, u64 rsp, rflags_t rflags);
int __start_twanvisor(void);

#endif