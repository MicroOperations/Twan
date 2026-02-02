#ifndef _TASK_H_
#define _TASK_H_

#include <include/lib/x86_index.h>
#include <include/std.h>
#include <include/kernel/isr/isr_index.h>
#include <include/kernel/boot.h>
#include <include/lib/dsa/flat_priorityq.h>
#include <include/lib/dsa/dq.h>
#include <include/lib/dsa/delta_chain.h>
#include <include/errno.h>
#include <include/kernel/sched/sched_conf.h>
#include <include/kernel/sched/sched_mcs.h>
#include <include/subsys/sync/mcslock.h>
#include <include/subsys/debug/kdbg/kdbg.h>

typedef void (*task_func_t)(void *arg);

typedef union 
{
    u32 val;
    struct 
    {
        u32 yield_request : 1;
        u32 preempted_early : 1;
        u32 timed_out : 1;
        u32 reserved0 : 29;
    } fields;
} task_flags_t;

typedef void (*sched_put_callback_func_t)(void);
typedef void (*sched_set_callback_func_t)(void);

struct task
{
    u8 *mempool;
    struct context context;

    struct list_double nodes[SCHED_NUM_CRITICALITIES];
    struct flat_priorityq_node waiting_node;

    struct delta_node sleep_node;

    atomic32_t preemption_count;
    task_flags_t flags;

    sched_put_callback_func_t put_callback_func; 
    sched_set_callback_func_t set_callback_func;
    
    struct
    {   
        u8 real_priority;
        u8 priority;
        u8 real_criticality;
        u8 criticality;
        u32 queue_id;

        struct waitq *waitq;
    } metadata;
};

#define node_to_task(ptr, criticality) \
    container_of((ptr), struct task, nodes[criticality])

#define waiting_node_to_task(ptr) \
    container_of((ptr), struct task, waiting_node)

#define sleep_node_to_task(ptr) \
    container_of((ptr), struct task, sleep_node)

#define is_yield_requested(task) ((task)->flags.fields.yield_request)
#define is_preempted_early(task) ((task)->flags.fields.preempted_early)
#define is_timed_out(task) ((task)->flags.fields.timed_out)
#define is_preemption_enabled(task) \
    ((u32)atomic32_read(&(task)->preemption_count) == 0)

#define set_yield_request(task) ((task)->flags.fields.yield_request = true)          
#define set_preempted_early(task) ((task)->flags.fields.preempted_early = true)
#define set_timed_out(task) ((task)->flags.fields.timed_out = true)

#define clear_yield_request(task) ((task)->flags.fields.yield_request = false)

#define clear_preempted_early(task) \
    ((task)->flags.fields.preempted_early = false)

#define clear_timed_out(task) ((task)->flags.fields.timed_out = false)

#define yield_requested() (is_yield_requested(current_task()))
#define timed_out() (is_timed_out(current_task()))

#define yield_request() (set_yield_request(current_task()))

#define preemption_count_raw() (current_task()->preemption_count)

#define preemption_count() ({                               \
                                                            \
    KBUG_ON(!current_task());                               \
    KBUG_ON(this_cpu_data()->handling_isr);                 \
                                                            \
    atomic32_read(&preemption_count_raw());                 \
})

#define disable_preemption() do {                           \
                                                            \
    KBUG_ON(!current_task());                               \
    KBUG_ON(this_cpu_data()->handling_isr);                 \
    KBUG_ON(current_task_preemption_count() == UINT32_MAX); \
                                                            \
    atomic32_inc(&preemption_count_raw());                  \
} while (0)


#define enable_preemption() do {                    \
                                                    \
    KBUG_ON(!current_task());                       \
    KBUG_ON(this_cpu_data()->handling_isr);         \
    KBUG_ON(current_task_is_preemption_enabled());  \
                                                    \
    atomic32_dec(&preemption_count_raw());          \
} while (0)

#define task_create_on_cpu_callbacks(processor_id, func, arg, priority,     \
                                     criticality, put_callback_func,        \
                                     set_callback_func)                     \
    __task_create((processor_id), (func), (arg), (priority), (criticality), \
                  (put_callback_func), (set_callback_func))

#define task_create_callbacks(func, arg, priority, criticality,             \
                              put_callback_func, set_callback_func)         \
    task_create_on_cpu_callbacks(this_processor_id(), (func), (arg),        \
                                 (priority), (criticality),    \
                                 (put_callback_func), (set_callback_func))

#define task_create_on_cpu(processor_id, func, arg, priority, criticality)  \
    task_create_on_cpu_callbacks((processor_id), (func), (arg), (priority), \
                                 (criticality), NULL, NULL)

#define task_create(func, arg, priority, criticality)        \
    task_create_on_cpu(this_processor_id(), (func), (arg), (priority),  \
                       (criticality)) 

#if SCHED_GLOBAL_QUEUE 

int task_init(struct task *task, __unused u32 processor_id, void *mempool,
              u64 stack_top, task_func_t func, void *arg, u8 priority, 
              u8 criticality, sched_put_callback_func_t put_callback_func, 
              sched_set_callback_func_t set_callback_func);

struct task *__task_create(__unused u32 processor_id, task_func_t func, 
                           void *arg, u8 priority, u8 criticality, 
                           sched_put_callback_func_t put_callback_func, 
                           sched_set_callback_func_t set_callback_func);

#else 

int task_init(struct task *task, u32 processor_id, void *mempool,
              u64 stack_top, task_func_t func, void *arg, u8 priority, 
              u8 criticality, sched_put_callback_func_t put_callback_func, 
              sched_set_callback_func_t set_callback_func);

struct task *__task_create(u32 processor_id, task_func_t func, void *arg, 
                           u8 priority, u8 criticality,
                           sched_put_callback_func_t put_callback_func, 
                           sched_set_callback_func_t set_callback_func);

#endif

void current_task_destroy_ipi(__unused u64 unused);
                              
void current_task_destroy(void);

u8 __current_task_priority(void);
u8 __current_task_criticality(void);

void current_task_write(u8 priority, u8 criticality);

int current_task_info(u8 *real_priority, u8 *priority, u8 *real_criticality, 
                      u8 *criticality);

u32 current_task_preemption_count(void);
bool current_task_is_preemption_enabled(void);
void current_task_disable_preemption(void);
void current_task_enable_preemption(void);

#endif