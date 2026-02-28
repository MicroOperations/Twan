#include <kernel/kapi.h>
#include <kernel/isr/isr_dispatcher.h>
#include <kernel/isr/base_isrs.h>
#include <kernel/apic/apic.h>
#include <lib/libtwanvisor/libvcalls.h>
#include <subsys/debug/kdbg/kdbg.h>
#include <errno.h>
#include <std.h>

extern void __emulate_interrupt(u64 rsp, u8 vector, u64 errcode);

int __map_irq(bool enable, u32 processor_id, u32 irq, u8 vector, 
             bool trig_explicit, bool trig)
{
    if (!cpu_valid(processor_id))
        return -EINVAL;

    return __ioapic_config_irq(!enable, lapic_id_of(processor_id), irq, vector,
                               trig_explicit, trig);
}

int map_irq(bool enable, u32 processor_id, u32 irq, u8 vector)
{
    return __map_irq(enable, processor_id, irq, vector, false, false);
}

int map_irq_trig(bool enable, u32 processor_id, u32 irq, u8 vector, bool trig)
{
    return __map_irq(enable, processor_id, irq, vector, true, trig);   
}

int register_isr(u32 processor_id, u8 vector, isr_func_t func)
{
    struct per_cpu *cpu_data = cpu_data(processor_id);
    if (!cpu_data)
        return -EINVAL;

    void *expected = NULL;
    return atomic_ptr_cmpxchg(&cpu_data->isr_table[vector], &expected, func) ?
           0 : -EALREADY;
}

int unregister_isr(u32 processor_id, u8 vector)
{
    struct per_cpu *cpu_data = cpu_data(processor_id);
    if (!cpu_data)
        return -EINVAL;
   
    atomic_ptr_set(&cpu_data->isr_table[vector], NULL);
    return 0;
}

int __ipi_run_func(u32 processor_id, ipi_func_t func, u64 arg, bool wait)
{
    if (!cpu_valid(processor_id))
        return -EINVAL;

    u32 this_id = this_processor_id();

    if (processor_id == this_id)
        return -EINVAL;

    struct per_cpu *cpu = cpu_data(processor_id);
    if (!cpu_active(cpu->flags))
        return -EINVAL;

    __ipi_wait(processor_id);

    struct twan_kernel *kernel = twan();

    struct ipi_data *data = &kernel->ipi_table[processor_id][this_id];

    data->func = func;
    data->arg = arg;
    data->wait = wait;

    __ipi_assert(data);

#if CONFIG_SUBSYS_TWANVISOR

    if (kernel->flags.fields.twanvisor_on != 0) {

        tv_vipi(processor_id, VIPI_DM_EXTERNAL, IPI_CMD_VECTOR, false);

    } else {
        lapic_send_ipi(lapic_id_of(processor_id), DM_NORMAL, 
                        LAPIC_DEST_PHYSICAL, SINGLE_TARGET, IPI_CMD_VECTOR);
    }

#else

    lapic_send_ipi(lapic_id_of(processor_id), DM_NORMAL, LAPIC_DEST_PHYSICAL,
                   SINGLE_TARGET, IPI_CMD_VECTOR);

#endif

    if (wait)
        __ipi_wait(processor_id);

    return 0;
}

int ipi_run_func(u32 processor_id, ipi_func_t func, u64 arg, bool wait)
{
    struct per_cpu *this_cpu = this_cpu_data();

    KBUG_ON(this_cpu->handling_isr);
    KBUG_ON(!is_interrupts_enabled());

    if (!cpu_valid(processor_id))
        return -EINVAL;

    if (processor_id == this_processor_id())
        return -EINVAL;

    struct per_cpu *cpu = cpu_data(processor_id);
    if (!cpu_active(cpu->flags))
        return -EINVAL;

    if (this_cpu->flags.fields.sched_init == 0)
        return __ipi_run_func(processor_id, func, arg, wait);
    
    current_task_disable_preemption();
    int ret = __ipi_run_func(processor_id, func, arg, wait);
    current_task_enable_preemption();

    return ret;
}

void ipi_run_func_others(ipi_func_t func, u64 arg, bool wait)
{
    u32 processor_id = this_processor_id();

    u32 num = num_cpus();
    for (u32 i = 0; i < num; i++) {

        struct per_cpu *cpu = cpu_data(i);
        
        if (i != processor_id && cpu_enabled(cpu->flags))
            ipi_run_func(i, func, arg, wait);
    }
}

void emulate_self_ipi(ipi_func_t func, u64 arg)
{
    u64 flags = read_flags_and_disable_interrupts();

    struct per_cpu *this_cpu = this_cpu_data();

    u32 processor_id = this_processor_id();
    struct ipi_data *data = &twan()->ipi_table[processor_id][processor_id];

    data->func = func;
    data->arg = arg;
    atomic32_set(&data->signal, IPI_LOCKED);

    u64 rsp = (u64)&this_cpu->int_stack_stub[sizeof(this_cpu->int_stack_stub)];
    
    __emulate_interrupt(rsp, SELF_IPI_CMD_VECTOR, 0);

    write_flags(flags);
}

void dead_local(void)
{
    disable_interrupts();
    halt_loop();
}

void dead_global(void)
{
    if (twan()->flags.fields.multicore_initialized != 0)
        ipi_run_func_others((ipi_func_t)dead_local, 0, false);

    dead_local();
}