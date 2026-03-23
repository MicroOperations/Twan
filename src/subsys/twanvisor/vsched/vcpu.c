#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vsched/vcpu.h>
#include <subsys/twanvisor/twanvisor.h>

bool vcurrent_vcpu_is_preemption_enabled(void)
{
    struct vcpu *current = vcurrent_vcpu();
    VDYNAMIC_ASSERT(current);

    return current->preemption_count == 0;
}

void vcurrent_vcpu_disable_preemption(void)
{
    struct vcpu *current = vcurrent_vcpu();
    VDYNAMIC_ASSERT(current);
    VDYNAMIC_ASSERT(current->preemption_count < UINT32_MAX);

    current->preemption_count++;
}

void vcurrent_vcpu_enable_preemption_no_yield(void)
{
    struct vcpu *current = vcurrent_vcpu();
    VDYNAMIC_ASSERT(current);
    VDYNAMIC_ASSERT(current->preemption_count > 0);

    current->preemption_count--;
}

bool vtry_answer_yield_request(void)
{
    struct vcpu *current = vcurrent_vcpu();

    u64 flags = read_flags_and_disable_interrupts();

    bool ret = current->preemption_count == 0 && 
               current->flags.fields.yield_request == 1;

    if (ret)
        vsched_yield();

    write_flags(flags);
    return ret;
}

void vcurrent_vcpu_enable_preemption(void)
{
    vcurrent_vcpu_enable_preemption_no_yield();
    vtry_answer_yield_request();
}

#endif