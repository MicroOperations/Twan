#include <include/kernel/sched/sched_yield.h>
#include <include/kernel/kapi.h>
#include <include/kernel/sched/sched_timer.h>
#include <include/kernel/sched/sched_dsa.h>
#include <include/subsys/debug/kdbg/kdbg.h>

bool sched_try_answer_yield_request(void)
{
    KBUG_ON(this_cpu_data()->handling_isr); 

    disable_preemption();

    bool ret = is_interrupts_enabled() && 
               current_task_preemption_count() == 1 && 
               yield_requested();
              
    if (ret)
        sched_yield();

    enable_preemption();
    return ret;
}

bool sched_should_request_yield(struct task *task)
{
    bool ret = false;

    struct sched_priorityq *sched_priorityq = this_sched_priorityq();
    struct mcsnode sched_node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&sched_priorityq->lock, &sched_node);

    u8 criticality = task->metadata.criticality;
    u8 criticality_level = __sched_mcs_read_criticality_level();
    
    u8 next_criticality;
    u8 next_priority;
    if (__sched_priorityq_get_bin(sched_priorityq, &next_priority,
                                   &next_criticality)) {
#if SCHED_AGING

        ret = next_criticality >= criticality_level || 
              criticality < criticality_level;

#else

        u8 priority = task->metadata.priority;

        if (criticality >= criticality_level) {

            ret = next_criticality >= criticality_level && 
                  next_priority >= priority;

        } else {

            ret = next_criticality >= criticality_level || 
                  next_priority >= priority;   
        }

#endif
    }

    mcs_unlock_isr_restore(&sched_priorityq->lock, &sched_node);
    return ret;
}

void sched_yield_ipi(__unused u64 unused)
{   
    struct task *current = current_task();
    struct interrupt_info *ctx = task_ctx();
    KBUG_ON(!current);
    KBUG_ON(!ctx);

    clear_yield_request(current);
    clear_preempted_early(current);

    struct task *old_current = current;

    struct task *task = sched_pop(current);
    if (task) {
        sched_switch_ctx(ctx, task, true);
        current = task;
    }

    if (current != old_current) {

        sched_timer_disable();

        if (sched_is_timer_pending())
            set_preempted_early(current);
    
        sched_timer_enable();
    }
}

void sched_yield(void)
{
    KBUG_ON(this_cpu_data()->handling_isr);

    emulate_self_ipi(sched_yield_ipi, 0);
}

void sched_yield_wait_ipi(u64 _arg)
{
    struct yield_wait_arg *arg = (void *)_arg;

    struct task *current = current_task();
    struct interrupt_info *ctx = task_ctx();
    KBUG_ON(!current);
    KBUG_ON(!ctx);

    sched_timer_disable();

    clear_yield_request(current);
    clear_preempted_early(current);

    struct task *task = sched_pop_clean_spin();
    sched_switch_ctx(ctx, task, false);

    if (arg->locked) {

        if (arg->timeout) {
            
            __waitq_insert_timeout(arg->waitq, current, arg->insert_real, 
                                   arg->ticks);
                                   
            yield_wait_timeout_unlock(arg->waitq, arg->waitq_node, 
                                      arg->timeout_node);
        } else {

            __waitq_insert(arg->waitq, current, arg->insert_real);
            yield_wait_unlock(arg->waitq, arg->waitq_node);
        }

    } else { 
        if (arg->timeout) {
            waitq_insert_timeout(arg->waitq, current, arg->insert_real,
                                 arg->ticks);
        } else { 
            waitq_insert(arg->waitq, current, arg->insert_real);
        }
    }

    if (sched_is_timer_pending())
        set_preempted_early(task);

    sched_timer_enable();
}

void sched_yield_wait_and_unlock(struct waitq *waitq, bool insert_real, 
                                 struct mcsnode *waitq_node, bool timeout, 
                                 struct mcsnode *timeout_node, u64 ticks)
{
    KBUG_ON(this_cpu_data()->handling_isr);

    struct yield_wait_arg arg = {
        .waitq = waitq,
        .locked = true,
        .insert_real = insert_real,
        
        .waitq_node = waitq_node,
        
        .timeout = timeout,
        .timeout_node = timeout_node,
        .ticks = ticks
    };

    emulate_self_ipi(sched_yield_wait_ipi, (u64)&arg);
}