#ifndef _SCHED_DSA_H_
#define _SCHED_DSA_H_

#include <include/kernel/sched/task.h>
#include <include/lib/dsa/priorityq.h>

#define SCHED_MIN_CRITICALITY 0
#define SCHED_MAX_CRITICALITY (SCHED_NUM_CRITICALITIES - 1)

struct sched_priorityq_bin
{
    struct priorityq priorityq;
};

struct sched_priorityq
{
    struct sched_priorityq_bin bins[SCHED_NUM_CRITICALITIES];
    u8 criticality_level;

#if SCHED_AGING
    struct bmp256 aging_bitmap;
    u32 iteration;
#endif

    struct mcslock_isr lock;
};

struct scheduler
{
    struct sched_priorityq queues[SCHED_NUM_QUEUES];
};

#define cpu_to_queue_id(processor_id) (SCHED_GLOBAL_QUEUE ? 0 : (processor_id))
#define queue_to_cpu_id(queue_id) (SCHED_GLOBAL_QUEUE ? 0 : (queue_id))
#define this_queue_id() (cpu_to_queue_id(this_processor_id()))

#define cpu_to_sched_priorityq(processor_id) \
    (&scheduler()->queues[cpu_to_queue_id(processor_id)])

#define queue_to_sched_priorityq(queue_id) \
    (&scheduler()->queues[queue_id])

#define this_sched_priorityq() (cpu_to_sched_priorityq(this_processor_id()))

void __sched_init(struct sched_priorityq *sched_priorityq);

struct sched_priorityq_bin *__sched_priorityq_get_bin(
    struct sched_priorityq *sched_priorityq, u8 *next_priority, 
    u8 *next_criticality);

void __sched_priorityq_push(struct sched_priorityq *sched_priorityq,
                            struct task *task);

void __sched_priorityq_dequeue(struct task *task);

struct task *__sched_priorityq_pop(struct sched_priorityq *sched_priorityq);

#if SCHED_AGING

void __sched_reset_age(struct sched_priorityq *sched_priorityq);
void __sched_age(struct sched_priorityq *sched_priorityq, u8 last_priority);
struct task *__sched_priorityq_pop_age(struct sched_priorityq *sched_priorityq);

#endif

int __sched_priorityq_highest_priority(
    struct sched_priorityq *sched_priorityq);

bool __sched_priorityq_is_queued(struct task *task);

int sched_priorityq_highest_priority(struct sched_priorityq *sched_priorityq);

#endif