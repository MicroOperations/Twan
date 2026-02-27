#ifndef _VCREATE_H_
#define _VCREATE_H_

#include <subsys/twanvisor/vsched/vcpu.h>
#include <subsys/twanvisor/vsched/vpartition.h>

int vcpu_precheck(struct vcpu *vcpu);
int vpartition_precheck(struct vpartition *vpartition);

void vcpu_setup(struct vpartition *vpartition, struct vcpu *vcpu, 
                u32 processor_id);

void vpartition_setup(struct vpartition *vpartition, u8 vid);
void vpartition_push(struct vpartition *vpartition);

void vcpu_entry(void);

#endif