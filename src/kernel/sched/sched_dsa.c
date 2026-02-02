#include <include/kernel/sched/sched_dsa.h>
#include <include/kernel/kapi.h>

void __sched_init(struct sched_priorityq *sched_priorityq)
{
    mcslock_isr_init(&sched_priorityq->lock);

#if SCHED_AGING
    __sched_reset_age(sched_priorityq);
#endif
}

struct sched_priorityq_bin *__sched_priorityq_get_bin(
    struct sched_priorityq *sched_priorityq, u8 *next_priority, 
    u8 *next_criticality)
{
    u8 criticality = sched_priorityq->criticality_level;
    struct sched_priorityq_bin *bin = &sched_priorityq->bins[criticality];

    int priority = priorityq_awaiting_priority(&bin->priorityq);
    if (priority != -1) {
        *next_priority = priority;
        *next_criticality = criticality;
        return bin;
    }

    if (criticality == 0)
        return NULL;

    /* fallback to considering all tasks */
    criticality = 0;
    bin = &sched_priorityq->bins[criticality];
    priority = priorityq_awaiting_priority(&bin->priorityq);
    
    if (priority == -1)
        return NULL;

    *next_priority = priority;
    *next_criticality = criticality;
    return bin;
}

void __sched_priorityq_push(struct sched_priorityq *sched_priorityq,
                            struct task *task)
{
    u8 priority = task->metadata.priority;
    u8 criticality = task->metadata.criticality;

    for (u32 i = 0; i <= criticality; i++) {

        priorityq_push(&sched_priorityq->bins[i].priorityq, &task->nodes[i], 
                       priority, PUSHBACK);
    }
}

void __sched_priorityq_dequeue(struct task *task)
{
    u8 priority = task->metadata.priority;
    u8 criticality = task->metadata.criticality;
    u32 queue_id = task->metadata.queue_id;

    struct sched_priorityq *sched_priorityq = 
        queue_to_sched_priorityq(queue_id);

    for (u32 i = 0; i <= criticality; i++) {

        struct priorityq *priorityq = &sched_priorityq->bins[i].priorityq;
        priorityq_dequeue(priorityq, &task->nodes[i], priority);
    }
}

struct task *__sched_priorityq_pop(struct sched_priorityq *sched_priorityq)
{
    u8 priority;
    u8 criticality;
    struct sched_priorityq_bin *bin = 
        __sched_priorityq_get_bin(sched_priorityq, &priority, &criticality);

    if (!bin)
        return NULL;

    struct list_double *node = priorityq_peekfront(&bin->priorityq);
    struct task *task = node_to_task(node, criticality);

    __sched_priorityq_dequeue(task);
    return task;
}

#if SCHED_AGING

void __sched_reset_age(struct sched_priorityq *sched_priorityq)
{
    bmp256_set_all(&sched_priorityq->aging_bitmap);
    sched_priorityq->iteration = 0;
}

void __sched_age(struct sched_priorityq *sched_priorityq, u8 last_priority)
{
    struct bmp256 *aging_bitmap = &sched_priorityq->aging_bitmap;
    u32 iteration = sched_priorityq->iteration;

    /* clear ages above the priority of the most recent task to allow for 
       quicker aging */
    bmp256_unset_all_above(aging_bitmap, last_priority);

    int priority = bmp256_fls(aging_bitmap);
    
    /* each priorities max iteration count can be calculcated by 
       (priority * 2) + 1 */
    if (priority == -1 || priority == 0) {

        __sched_reset_age(sched_priorityq);

    } else if (iteration == ((u32)priority * 2)) {

        bmp256_unset(&sched_priorityq->aging_bitmap, priority);
        sched_priorityq->iteration = 0;

    } else {
        sched_priorityq->iteration++;
    }
}

struct task *__sched_priorityq_pop_age(struct sched_priorityq *sched_priorityq)
{
    u8 priority;
    u8 criticality;
    struct sched_priorityq_bin *bin = 
        __sched_priorityq_get_bin(sched_priorityq, &priority, &criticality);

    if (!bin)
        return NULL;
        
    struct bmp256 priority_bitmap = bin->priorityq.priority_bitmap;

    bmp256_and(&bin->priorityq.priority_bitmap, &sched_priorityq->aging_bitmap);
    int next_priority = priorityq_awaiting_priority(&bin->priorityq);

    if (next_priority == -1) {

        __sched_reset_age(sched_priorityq);
        bin->priorityq.priority_bitmap = priority_bitmap;

        next_priority = priorityq_awaiting_priority(&bin->priorityq);
        if (next_priority == -1)
            return NULL;
    }

    struct task *task = __sched_priorityq_pop(sched_priorityq);

    if (!bmp256_test(&bin->priorityq.priority_bitmap, next_priority))
        bmp256_unset(&priority_bitmap, next_priority);

    __sched_age(sched_priorityq, next_priority);
    bin->priorityq.priority_bitmap = priority_bitmap;
    return task;
}

#endif

int __sched_priorityq_highest_priority(struct sched_priorityq *sched_priorityq)
{
    u8 criticality = sched_priorityq->criticality_level;

    struct sched_priorityq_bin *bin = &sched_priorityq->bins[criticality];
    int ret = priorityq_awaiting_priority(&bin->priorityq);

    /* fallback to considering all tasks */
    if (ret == -1 && criticality > 0) {

        bin = &sched_priorityq->bins[0];
        ret = priorityq_awaiting_priority(&bin->priorityq);
    }

    return ret;
}

bool __sched_priorityq_is_queued(struct task *task)
{
    u32 queue_id = task->metadata.queue_id;

    struct sched_priorityq *sched_priorityq = 
        queue_to_sched_priorityq(queue_id);

    /* critical 0 should be linked if its queued no matter what */
    struct priorityq *priorityq = &sched_priorityq->bins[0].priorityq;

    return priorityq_is_queued(priorityq, &task->nodes[0], 
                               task->metadata.priority);
}

int sched_priorityq_highest_priority(struct sched_priorityq *sched_priorityq)
{
    struct mcsnode sched_node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&sched_priorityq->lock, &sched_node);
    int ret = __sched_priorityq_highest_priority(sched_priorityq);
    mcs_unlock_isr_restore(&sched_priorityq->lock, &sched_node);

    return ret;
}