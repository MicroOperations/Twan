#ifndef _VRECOVERY_H_
#define _VRECOVERY_H_

#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/vsched/vpartition.h>

void vdispatcher_dequeue(struct vper_cpu *target, struct vcpu *vcpu);
void vipi_ack_dequeue_all(struct vcpu *vcpu);

void vdo_finalize_teardown(u8 vid);
void vfinalize_teardown_ipi(u64 vid);
void vfinalize_teardown(u8 vid);

void vfailure_recover(void);
void vfailure_recover_running(void);

int vteardown(u8 vid);

void vdo_transitioning_recover(struct vcpu *vcpu);
void vtransitioning_recover_ipi(__unused u64 arg);

void vtransitioning_recover(void);

#endif