#ifndef _LIBVCALLS_H_
#define _LIBVCALLS_H_

#include <include/subsys/twanvisor/vemulate/vcalls.h>
#include <include/subsys/twanvisor/vsched/vpartition.h>

inline long tv_vcall(u64 id, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5,
                     u64 arg6)
{
    long ret;

    __asm__ __volatile__ (
        "movq %[arg5], %%r8;"
        "movq %[arg6], %%r9;"
        "vmcall;"
        : "=a"(ret)
        : "a"(id), 
        "D"(arg1), 
        "S"(arg2), 
        "d"(arg3), 
        "c"(arg4), 
        [arg5]"r"(arg5), 
        [arg6]"r"(arg6)
        : "r8", "r9", "r10", "r11", "memory"  
    );

    return ret;
}

inline long tv_vsubscribe_external_interrupt_vector(u8 vector)
{
    return tv_vcall(VSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR, vector, 
                    0, 0, 0, 0, 0);
}

inline long tv_vunsubscribe_external_interrupt_vector(u8 vector)
{
    return tv_vcall(VUNSUBSCRIBE_EXTERNAL_INTERRUPT_VECTOR, vector, 
                    0, 0, 0, 0, 0);
}

inline long tv_vipi(u32 processor_id, u32 delivery_mode, u8 vector, bool nmi)
{
    return tv_vcall(VIPI, processor_id, delivery_mode, vector, nmi, 0, 0);
}

inline long tv_vipi_far(u8 vid, u32 processor_id, u32 delivery_mode, u8 vector, 
                        bool nmi)
{
    return tv_vcall(VIPI_FAR, vid, processor_id, delivery_mode, vector, nmi, 0);
}

inline long tv_vtlb_shootdown(u8 target_vid)
{
    return tv_vcall(VTLB_SHOOTDOWN, target_vid, 0, 0, 0, 0, 0);
}

inline long tv_varm_timern(u8 vector, u8 timer_n, u32 ticks, bool periodic,
                           bool nmi)
{
    return tv_vcall(VARM_TIMERN, vector, timer_n, ticks, periodic, nmi, 0);
}

inline long tv_vdisarm_timern(u8 timer_n)
{
    return tv_vcall(VDISARM_TIMERN, timer_n, 0, 0, 0, 0, 0);
}

inline long tv_valter_vcpu_timeslice(u8 vid, u32 processor_id, u32 ticks)
{
    return tv_vcall(VALTER_VCPU_TIMESLICE, vid, processor_id, ticks, 0, 0, 0);
}

inline long tv_valter_vcpu_criticality(u8 vid, u32 processor_id, u8 criticality)
{
    return tv_vcall(VALTER_VCPU_CRITICALITY, vid, processor_id, criticality, 
                    0, 0, 0);
}

inline long tv_vis_serviced(u8 intl)
{
    return tv_vcall(VIS_SERVICED, intl, 0, 0, 0, 0, 0);
}

inline long tv_vis_pending(u8 vector, bool nmi)
{
    return tv_vcall(VIS_PENDING, vector, nmi, 0, 0, 0, 0);
}

inline long tv_veoi(void)
{
    return tv_vcall(VEOI, 0, 0, 0, 0, 0, 0);
}

inline long tv_vyield(void)
{
    return tv_vcall(VYIELD, 0, 0, 0, 0, 0, 0);
}

inline long tv_vpause(void)
{
    return tv_vcall(VPAUSE, 0, 0, 0, 0, 0, 0);
}

inline long tv_vread_vcpu_state(u32 processor_id)
{
    return tv_vcall(VREAD_VCPU_STATE, processor_id, 0, 0, 0, 0, 0);
}

inline long tv_vread_vcpu_state_far(u8 vid, u32 processor_id)
{
    return tv_vcall(VREAD_VCPU_STATE_FAR, vid, processor_id, 0, 0, 0, 0);
}

inline long tv_vset_route(u8 target_vid, u8 sender_vid, u32 route_type, 
                          bool allow)
{
    return tv_vcall(VSET_ROUTE, target_vid, sender_vid, route_type, allow, 
                    0, 0);
}

inline long tv_vset_vcpu_subscription_perm(u8 vid, u32 processor_id, u8 vector,
                                           bool allow)
{
    return tv_vcall(VSET_VCPU_SUBSCRIPTION_PERM, vid, processor_id, vector, 
                    allow, 0, 0);
}

inline long tv_vset_criticality_perm(u8 vid, u32 processor_id, 
                                    vcriticality_perm_t perm)
{
    return tv_vcall(VSET_CRITICALITY_PERM, vid, processor_id, perm.val, 
                    0, 0, 0);
}

inline long tv_vread_criticality_level(int physical_processor_id)
{
    return tv_vcall(VREAD_CRITICALITY_LEVEL, physical_processor_id, 
                    0, 0, 0, 0, 0);
}

inline long tv_vwrite_criticality_level(int physical_processor_id, 
                                        u8 criticality_level)
{
    return tv_vcall(VWRITE_CRITICALITY_LEVEL, physical_processor_id, 
                    criticality_level, 0, 0, 0, 0);
}

inline long tv_vpv_spin_pause(void)
{
    return tv_vcall(VPV_SPIN_PAUSE, 0, 0, 0, 0, 0, 0);
}

inline long tv_vpv_spin_kick(u32 processor_id)
{
    return tv_vcall(VPV_SPIN_KICK, processor_id, 0, 0, 0, 0, 0);   
}

inline long tv_vpv_spin_kick_far(u8 vid, u32 processor_id)
{
    return tv_vcall(VPV_SPIN_KICK_FAR, vid, processor_id, 0, 0, 0, 0); 
}

inline long tv_vkdbg(const char *str)
{
    return tv_vcall(VKDBG, (u64)str, 0, 0, 0, 0, 0);
}

inline long tv_vcreate_partition(struct vpartition *vpartition)
{
    return tv_vcall(VCREATE_PARTITION, (u64)vpartition, 0, 0, 0, 0, 0);
}

inline long tv_vdestroy_partition(u8 vid)
{
    return tv_vcall(VDESTROY_PARTITION, vid, 0, 0, 0, 0, 0);
}

#endif