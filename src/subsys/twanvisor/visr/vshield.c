#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/visr/vshield.h>
#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/visr/visr_index.h>

int venter_shield_local(u64 *flags, u8 vector, u64 rip)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    VBUG_ON(vthis_cpu->handling_isr);

    if (!vis_exception_vector(vector))
        return -EINVAL;

    u64 _flags = read_flags_and_disable_interrupts();    
    
    vthis_cpu->shield.ext.fields.exception_vector = vector;
    vthis_cpu->shield.recovery_rip = rip;
    vthis_cpu->shield.ext.fields.faulted = 0;
    vthis_cpu->shield.ext.fields.recover_by_length = 0;
    vthis_cpu->shield.ext.fields.active = 1;

    if (flags)
        *flags = _flags;

    return 0;
}

int venter_shield_local_length(u64 *flags, u8 vector, u8 length)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    VBUG_ON(vthis_cpu->handling_isr);

    if (!vis_exception_vector(vector))
        return -EINVAL;

    u64 _flags = read_flags_and_disable_interrupts();    
    
    vthis_cpu->shield.ext.fields.exception_vector = vector;
    vthis_cpu->shield.ext.fields.faulted = 0;
    vthis_cpu->shield.ext.fields.recover_by_length = 1;
    vthis_cpu->shield.ext.fields.len = length;
    vthis_cpu->shield.ext.fields.active = 1;

    if (flags)
        *flags = _flags;

    return 0;
}

bool vexit_shield_local(u64 flags)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    VBUG_ON(vthis_cpu->handling_isr);
    
    bool ret = vthis_cpu->shield.ext.fields.faulted != 0;
    vthis_cpu->shield.ext.fields.active = 0;

    write_flags(flags);
    return ret;
}

#endif