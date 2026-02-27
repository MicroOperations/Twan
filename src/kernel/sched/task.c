#include <kernel/sched/task.h>
#include <kernel/sched/sched.h>
#include <kernel/kapi.h>
#include <kernel/kernel.h>
#include <subsys/mem/rtalloc.h>

#if SCHED_GLOBAL_QUEUE

int task_init(struct task *task, __unused u32 processor_id, void *mempool, 
               u64 stack_top, task_func_t func, void *arg, u8 priority, 
               u8 criticality, sched_put_callback_func_t put_callback_func, 
               sched_set_callback_func_t set_callback_func)

#else

int task_init(struct task *task, u32 processor_id, void *mempool, 
              u64 stack_top, task_func_t func, void *arg, u8 priority, 
              u8 criticality,sched_put_callback_func_t put_callback_func, 
              sched_set_callback_func_t set_callback_func)

#endif
{
    struct per_cpu *cpu = cpu_data(processor_id);
    if (!cpu || !cpu_active(cpu->flags) || criticality >= SCHED_NUM_CRITICALITIES)
        return -EINVAL;

    memset(task, 0, sizeof(struct task));

    task->mempool = mempool;

    task->context.rip = (u64)func;
    task->context.rsp = (u64)stack_top;
    task->context.rbp = stack_top;
    task->context.rdi = (u64)arg;
    task->context.rflags.fields._if = 1;
    task->context.rflags.fields.reserved0 = 1;

    task->context.fp_context.fcw = DEFAULT_FCW;
    task->context.fp_context.mxcsr = DEFAULT_MXCSR;    
    task->context.fp_context.mxcsr_mask = mxcsr_mask(); 

    atomic32_set(&task->preemption_count, 0);

    task->put_callback_func = put_callback_func;
    task->set_callback_func = set_callback_func;

    task->metadata.queue_id = cpu_to_queue_id(processor_id);
    task->metadata.real_priority = priority;
    task->metadata.priority = priority;
    task->metadata.real_criticality = criticality;
    task->metadata.criticality = criticality;

    return 0;
}

#if SCHED_GLOBAL_QUEUE

struct task *__task_create(__unused u32 processor_id, task_func_t func, 
                           void *arg, u8 priority, u8 criticality, 
                           sched_put_callback_func_t put_callback_func, 
                           sched_set_callback_func_t set_callback_func)

#else


struct task *__task_create(u32 processor_id, task_func_t func, void *arg, 
                           u8 priority, u8 criticality,
                           sched_put_callback_func_t put_callback_func, 
                           sched_set_callback_func_t set_callback_func)

#endif 
{
    if (criticality >= SCHED_NUM_CRITICALITIES)
        return ERR_PTR(-ENOMEM);

    struct per_cpu *cpu = cpu_data(processor_id);
    if (!cpu || !cpu_active(cpu->flags))
        return ERR_PTR(-EINVAL);

    u8 *mempool = 
        rtalloc_p2aligned(sizeof(struct task) + SCHED_TASK_STACK_SIZE, 16);

    if (!mempool)
        return ERR_PTR(-ENOMEM);

    u64 stack_top = (u64)(mempool + SCHED_TASK_STACK_SIZE);
    struct task *task = (void *)stack_top;

    u64 *stack_ptr = (u64 *)(stack_top - 8);
    *stack_ptr = (u64)current_task_destroy;

    task_init(task, processor_id, mempool, (u64)stack_ptr, func, arg, priority, 
              criticality, put_callback_func, set_callback_func);

    sched_push_on_cpu(task, processor_id);

    return task;
}

void current_task_destroy_ipi( __unused u64 unused)
{
    sched_timer_disable();

    struct task *current = current_task();
    struct interrupt_info *ctx = task_ctx();
    KBUG_ON(!current);
    KBUG_ON(!ctx);

    set_current_task(NULL);

    void *mempool = current->mempool;
    if (mempool)
        rtfree(mempool);
       
    struct task *task = sched_pop_clean_spin();
    sched_set_ctx(ctx, task);

    if (sched_is_timer_pending())
        set_preempted_early(task);

    sched_timer_enable();
}

void current_task_destroy(void)
{
    KBUG_ON(!current_task()); 
    KBUG_ON(this_cpu_data()->handling_isr); 

    emulate_self_ipi(current_task_destroy_ipi, 0);
}

u8 __current_task_priority(void)
{
    struct task *current = current_task();
    KBUG_ON(!current);
    KBUG_ON(current_task_is_preemption_enabled());

    return current->metadata.priority;
}

u8 __current_task_criticality(void)
{
    struct task *current = current_task();
    KBUG_ON(!current);
    KBUG_ON(current_task_is_preemption_enabled());

    return current->metadata.criticality;
}

void current_task_write(u8 priority, u8 criticality)
{
    struct task *current = current_task();
    KBUG_ON(!current);
    KBUG_ON(criticality > SCHED_MAX_CRITICALITY);

    u64 flags = read_flags_and_disable_interrupts();
    current->metadata.priority = priority;
    current->metadata.criticality = criticality;
    write_flags(flags);
}

int current_task_info(u8 *real_priority, u8 *priority, u8 *real_criticality, 
                      u8 *criticality)
{
    struct task *current = current_task();
    if (KBUG_ON(!current) || KBUG_ON(this_cpu_data()->handling_isr))
        return -EINVAL;    

    current_task_disable_preemption();

    *real_priority = current->metadata.real_priority;
    *priority = current->metadata.priority;
    *real_criticality = current->metadata.real_criticality;
    *criticality = current->metadata.criticality;

    current_task_enable_preemption();
    return 0;
}

u32 current_task_preemption_count(void)
{
    return preemption_count();
}

bool current_task_is_preemption_enabled(void)
{
    return current_task_preemption_count() == 0;
}

void current_task_disable_preemption(void)
{    
    disable_preemption();
}

void current_task_enable_preemption(void)
{
    enable_preemption();
    sched_try_answer_yield_request();
}