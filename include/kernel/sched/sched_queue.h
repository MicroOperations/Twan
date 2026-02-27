#ifndef _SCHED_QUEUE_H_
#define _SCHED_QUEUE_H_

#include <kernel/sched/sched_dsa.h>

#define sched_push_on_cpu(task, processor_id) \
    sched_do_push((task), (processor_id))

#define sched_push(task) \
    (sched_do_push((task), queue_to_cpu_id((task)->metadata.queue_id)))

#define sched_pop_clean_spin() ({                       \
                                                        \
    struct task *task = NULL;                           \
    spin_until((task = sched_pop_clean()) != NULL);     \
                                                        \
    task;                                               \
})

void sched_do_push(struct task *task, u32 processor_id);

struct task *sched_pop_clean(void);
struct task *sched_pop(struct task *current);

#endif