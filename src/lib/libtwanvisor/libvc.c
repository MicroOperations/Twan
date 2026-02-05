#include <include/lib/libtwanvisor/libvc.h>
#include <include/kernel/kapi.h>

void vsubscribe_ipi(u64 arg)
{
    struct vsub_ipi_arg *data = (void *)arg;
    data->ret = tv_vsubscribe_external_interrupt_vector(data->vector);
}

void vunsubscribe_ipi(u64 arg)
{
    struct vsub_ipi_arg *data = (void *)arg;
    data->ret = tv_vsubscribe_external_interrupt_vector(data->vector);
}

long tv_vsubscribe_on_cpu(u32 processor_id, u8 vector)
{
    u32 this_processor_id = this_processor_id();

    long ret;

    if (processor_id != this_processor_id) {

        struct vsub_ipi_arg arg = {
            .vector = vector
        };

        ret = ipi_run_func(processor_id, vsubscribe_ipi, (u64)&arg, true);

        if (ret == 0)
            ret = arg.ret;

    } else {
        ret = tv_vsubscribe_external_interrupt_vector(vector);
    }

    return ret;
}

long tv_vunsubscribe_on_cpu(u32 processor_id, u8 vector)
{
    u32 this_processor_id = this_processor_id();

    long ret;

    if (processor_id != this_processor_id) {

        struct vsub_ipi_arg arg = {
            .vector = vector,
        };

        ret = ipi_run_func(processor_id, vunsubscribe_ipi, (u64)&arg, true);

        if (ret == 0)
            ret = arg.ret;

    } else {
        ret = tv_vunsubscribe_external_interrupt_vector(vector);
    }

    return ret;
}

void varm_timer_ipi(u64 arg)
{
    struct varm_timer_ipi_arg *data = (void *)arg;
    data->ret = tv_varm_timern(data->vector, data->timer_n, data->ticks,
                               data->periodic, data->nmi);
}

void vdisarm_timer_ipi(u64 arg)
{
    struct vdisarm_timer_ipi_arg *data = (void *)arg;
    data->ret = tv_vdisarm_timern(data->timer_n);
}

long tv_varm_timer_on_cpu(u32 processor_id, u8 vector, u8 timer_n, u32 ticks,
                          bool periodic, bool nmi)
{
    u32 this_processor_id = this_processor_id();

    long ret;

    if (processor_id != this_processor_id) {

        struct varm_timer_ipi_arg arg = {
            .vector = vector,
            .timer_n = timer_n,
            .nmi = nmi,
            .ticks = ticks,
            .periodic = periodic
        };

        ret = ipi_run_func(processor_id, varm_timer_ipi, (u64)&arg, true);

        if (ret == 0)
            ret = arg.ret;

    } else {
        ret = tv_varm_timern(vector, timer_n, ticks, periodic, nmi);
    }

    return ret;
}

long tv_vdisarm_timer_on_cpu(u32 processor_id, u8 timer_n)
{
    u32 this_processor_id = this_processor_id();

    long ret;

    if (processor_id != this_processor_id) {

        struct vdisarm_timer_ipi_arg arg = {
            .timer_n = timer_n
        };

        ret = ipi_run_func(processor_id, vdisarm_timer_ipi, (u64)&arg, true);

        if (ret == 0)
            ret = arg.ret;

    } else {
        ret = tv_vdisarm_timern(timer_n);
    }

    return ret;
}

bool tv_vintc_is_serviced(u8 vector)
{
    return tv_vis_serviced(vector_to_intl(vector)) == 1;
}

bool tv_vintc_is_pending(u8 vector)
{
    return tv_vis_pending(vector, vector == NMI) == 1;
}

void tv_vintc_eoi(void)
{
    tv_veoi();
}

bool tv_vis_vcpu_active(u32 processor_id)
{
    long state = tv_vread_vcpu_state(processor_id);
    return state == VRUNNING || state == VTRANSITIONING;
}