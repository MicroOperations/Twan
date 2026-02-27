#include <kernel/sched/sched_queue.h>
#include <kernel/kernel.h>
#include <subsys/debug/kdbg/kdbg.h>

void sched_do_push(struct task *task, u32 processor_id)
{
    u32 queue_id = cpu_to_queue_id(processor_id);
    task->metadata.queue_id = queue_id;

    struct sched_priorityq *sched_priorityq = 
        cpu_to_sched_priorityq(processor_id);

    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&sched_priorityq->lock, &node);
    __sched_priorityq_push(sched_priorityq, task);
    mcs_unlock_isr_restore(&sched_priorityq->lock, &node);
}

struct task *sched_pop_clean(void)
{
    struct task *task = NULL;

    struct sched_priorityq *sched_priorityq = this_sched_priorityq();

    struct mcsnode sched_node = INITIALIZE_MCSNODE();
    mcs_lock_isr_save(&sched_priorityq->lock, &sched_node);

#if SCHED_AGING

    task = __sched_priorityq_pop_age(sched_priorityq);

#else

    if (__sched_priorityq_highest_priority(sched_priorityq) != -1)
        task = __sched_priorityq_pop(sched_priorityq);

#endif

    mcs_unlock_isr_restore(&sched_priorityq->lock, &sched_node);

    return task;
}

struct task *sched_pop(struct task *current)
{
    struct task *new_task = NULL;
    
    struct sched_priorityq *sched_priorityq = this_sched_priorityq();

    struct mcsnode sched_node = INITIALIZE_MCSNODE();
    mcs_lock_isr_save(&sched_priorityq->lock, &sched_node);    

    u8 next_priority;
    u8 next_criticality;
    if (__sched_priorityq_get_bin(sched_priorityq, &next_priority, 
                                   &next_criticality)) {
        
        u8 criticality = current->metadata.criticality;
        u8 criticality_level = __sched_mcs_read_criticality_level();

#if SCHED_AGING

        if (next_criticality >= criticality_level) 
            new_task = __sched_priorityq_pop_age(sched_priorityq);
        else if (criticality < criticality_level)
            new_task = __sched_priorityq_pop_age(sched_priorityq);
    
#else

        u8 priority = current->metadata.priority;

        if (criticality >= criticality_level) {

            if (next_criticality >= criticality_level && 
                next_priority >= priority) {

                new_task = __sched_priorityq_pop(sched_priorityq);
            }

        } else if (next_criticality >= criticality_level || 
                   next_priority >= priority) {

            new_task = __sched_priorityq_pop(sched_priorityq);
        }

#endif
    }

    mcs_unlock_isr_restore(&sched_priorityq->lock, &sched_node);
    return new_task;
}