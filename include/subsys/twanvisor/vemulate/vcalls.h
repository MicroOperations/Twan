#ifndef _VCALLS_H_
#define _VCALLS_H_

#include <include/subsys/twanvisor/varch.h>

typedef enum
{
    VIPI_DM_EXTERNAL,
    VIPI_DM_UNPAUSE
} vipi_delivery_mode_t;

typedef enum
{
    VSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR,
    VUNSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR,

    VIPI,
    VIPI_FAR,
    VTLB_SHOOTDOWN,

    VARM_TIMERN,
    VDISARM_TIMERN,

    VALTER_VCPU_TIMESLICE,
    VALTER_VCPU_CRITICALITY,

    VIS_SERVICED,
    VIS_PENDING,

    VEOI,
    VYIELD,
    VPAUSE,

    VREAD_VCPU_STATE,
    VREAD_VCPU_STATE_FAR,

    VSET_ROUTE,
    VSET_VCPU_SUBSCRIPTION_PERM,
    VSET_CRITICALITY_PERM,

    VREAD_CRITICALITY_LEVEL,
    VWRITE_CRITICALITY_LEVEL,

    VPV_SPIN_PAUSE,
    VPV_SPIN_KICK,

    VKDBG,

    VCREATE_PARTITION,
    VDESTROY_PARTITION,
} vcall_t;

typedef long (*vcall_func_t)(struct vregs *vregs);

void vcall_dispatcher(struct vregs *vregs);

#endif