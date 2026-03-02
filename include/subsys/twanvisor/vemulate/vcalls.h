#ifndef _VCALLS_H_
#define _VCALLS_H_

#include <subsys/twanvisor/varch.h>

typedef enum
{
    VIPI_DM_EXTERNAL,
    VIPI_DM_UNPAUSE
} vipi_delivery_mode_t;

#define VSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR 0
#define VUNSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR 1

#define VIPI 2
#define VIPI_FAR 3
#define VTLB_SHOOTDOWN 4

#define VARM_TIMERN 5
#define VDISARM_TIMERN 6

#define VALTER_VCPU_TIMESLICE 7
#define VALTER_VCPU_CRITICALITY 8

#define VIS_SERVICED 9
#define VIS_PENDING 10
#define VEOI 11

#define VYIELD 12
#define VPAUSE 13

#define VREAD_VCPU_STATE 14
#define VREAD_VCPU_STATE_FAR 15

#define VSET_ROUTE 16
#define VSET_VCPU_SUBSCRIPTION_PERM 17
#define VSET_CRITICALITY_PERM 18

#define VREAD_CRITICALITY_LEVEL 19
#define VWRITE_CRITICALITY_LEVEL 20

#define VPV_SPIN_PAUSE 21
#define VPV_SPIN_KICK 22
#define VPV_SPIN_KICK_FAR 23

#define VKDBG 24

#define VCREATE_PARTITION 25
#define VDESTROY_PARTITION 26

#define VFRAME_SET 27
#define VFRAME_UNSET 28

/* 
    vcalls follow sysv 
    
    vcall id - rax
    arg1 - rdi
    arg2 - rsi
    arg3 - rdx
    arg4 - rcx
    arg5 - r8
    arg6 - r9

    return val - rax
*/

#define vcall_id(vregs) ((vregs)->regs.rax)
#define vcall_set_retval(vregs, ret) ((vregs)->regs.rax = (ret))

#define vcall_arg1_8(vregs) ((vregs)->regs.rdi & 0xff)
#define vcall_arg1_32(vregs) ((vregs)->regs.rdi & 0xffffffff)
#define vcall_arg1_64(vregs) ((vregs)->regs.rdi & 0xffffffffffffffff)

#define vcall_arg2_8(vregs) ((vregs)->regs.rsi & 0xff)
#define vcall_arg2_32(vregs) ((vregs)->regs.rsi & 0xffffffff)
#define vcall_arg2_64(vregs) ((vregs)->regs.rsi & 0xffffffffffffffff)

#define vcall_arg3_8(vregs) ((vregs)->regs.rdx & 0xff)
#define vcall_arg3_32(vregs) ((vregs)->regs.rdx & 0xffffffff)
#define vcall_arg3_64(vregs) ((vregs)->regs.rdx & 0xffffffffffffffff)

#define vcall_arg4_8(vregs) ((vregs)->regs.rcx & 0xff)
#define vcall_arg4_32(vregs) ((vregs)->regs.rcx & 0xffffffff)
#define vcall_arg4_64(vregs) ((vregs)->regs.rcx & 0xffffffffffffffff)

#define vcall_arg5_8(vregs) ((vregs)->regs.r8 & 0xff)
#define vcall_arg5_32(vregs) ((vregs)->regs.r8 & 0xffffffff)
#define vcall_arg5_64(vregs) ((vregs)->regs.r8 & 0xffffffffffffffff)

#define vcall_arg6_8(vregs)  ((vregs)->regs.r9 & 0xff)
#define vcall_arg6_32(vregs) ((vregs)->regs.r9 & 0xffffffff)
#define vcall_arg6_64(vregs) ((vregs)->regs.r9 & 0xffffffffffffffff)

typedef long (*vcall_func_t)(struct vregs *vregs);

void vcall_dispatcher(struct vregs *vregs);

#endif