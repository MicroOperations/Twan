#include <kernel/sched/sched.h>
#include <kernel/kapi.h>
#include <subsys/debug/kdbg/kdbg.h>

void sched_worker(__unused void *unused)
{
    while (1)
        sched_yield();
}

void sched_trampoline_ipi(__unused u64 unused)
{
    struct per_cpu *cpu = this_cpu_data();
    struct interrupt_info *ctx = task_ctx();

    cpu->flags.fields.sched_init = 1;

    struct task *worker = &cpu->worker;
    u64 stack_top = (u64)&cpu->worker_stack[sizeof(cpu->worker_stack)];
   
    task_init(worker, this_processor_id(), NULL, stack_top, sched_worker, 
              NULL, LOWEST_PRIORITY, 0, NULL, NULL);
   
    sched_push(worker);

    struct task *task = sched_pop_clean_spin();
    sched_set_ctx(ctx, task);

    sched_timer_init(CONFIG_KERNEL_SCHED_TIME_SLICE_MS);
}

void scheduler_init(void)
{
    struct scheduler *sched = scheduler();

    for (u32 i = 0; i < ARRAY_LEN(sched->queues); i++)
        __sched_init(&sched->queues[i]);

    ipi_run_func_others(sched_trampoline_ipi, 0, false);

    u64 num_drivers = num_registered_drivers();
    for (u32 i = 0; i < num_drivers; i++)
        INDIRECT_BRANCH_SAFE(__driver_registry_start[i].init());

    emulate_self_ipi(sched_trampoline_ipi, 0);
}

void sched_preempt(struct interrupt_info *ctx)
{
    struct task *current = current_task();

    if (is_preempted_early(current)) {
        clear_preempted_early(current);
        return;
    }

    if (!is_preemption_enabled(current)) {

        if (sched_should_request_yield(current))
            set_yield_request(current);

        return;
    }

    clear_yield_request(current);
    struct task *task = sched_pop(current);
    if (task)
        sched_switch_ctx(ctx, task, true);
}