#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vsched/vsched_mcs.h>
#include <subsys/twanvisor/twanvisor.h>

u8 __vsched_mcs_read_criticality_level_local(void)
{
    return vthis_vscheduler()->vcriticality_level;
}

int __vsched_mcs_write_criticality_level_local(u8 criticality)
{
    if (criticality > VSCHED_MAX_CRITICALITY)
        return -EINVAL;

    vthis_vscheduler()->vcriticality_level = criticality;
    return 0;
}

u8 vsched_mcs_read_criticality_level(u32 vprocessor_id)
{
    struct vscheduler *vsched = &vper_cpu_data(vprocessor_id)->vscheduler;
    
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vsched->lock, &node);
    u8 crit = vsched->vcriticality_level;
    vmcs_unlock_isr_restore(&vsched->lock, &node);

    return crit;
}

int vsched_mcs_write_criticality_level(u32 vprocessor_id, u8 criticality)
{ 
    if (criticality > VSCHED_MAX_CRITICALITY)
        return -EINVAL;

    struct vscheduler *vsched = &vper_cpu_data(vprocessor_id)->vscheduler;
    
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vsched->lock, &node);
    vsched->vcriticality_level = criticality;
    vmcs_unlock_isr_restore(&vsched->lock, &node);

    return 0;
}

#endif