#ifndef _KAPI_H_
#define _KAPI_H_

#include <kernel/kernel.h>
#include <subsys/sync/spinlock.h>

/* driver registration */

#define DRIVER_REGISTRY ".driver_registry"

#define __driver_registry \
    __attribute__ ((section(DRIVER_REGISTRY), used, aligned(1)))

typedef void (*driver_init_t)(void);

struct driver
{
    const char *name;
    driver_init_t init;
};

#define __driver_init

extern struct driver __driver_registry_start[];
extern struct driver __driver_registry_end[];

#define num_registered_drivers()                                   \
    (((u64)__driver_registry_end - (u64)__driver_registry_start) / \
     sizeof(struct driver))

#define REGISTER_DRIVER(_name, _init)                                   \
    __driver_registry                                                   \
    static const struct driver driver_##_name = {                       \
        .name = (#_name),                                               \
        .init = (_init)                                                 \
    }                                                                

/* initcalls */

#define EARLY_INITCALL_REGISTRY ".early_initcall_registry"

#define __early_initcall_registry(order) \
    __attribute__ ((section(EARLY_INITCALL_REGISTRY "." #order), used))

typedef void (*early_initcall_t)(void);

struct early_initcall
{
    const char *name;
    early_initcall_t init;
};

#define __early_initcall

extern struct early_initcall __early_initcall_registry_start[];
extern struct early_initcall __early_initcall_registry_end[];

#define num_registered_early_initcalls()                \
    (((u64)__early_initcall_registry_end -              \
      (u64)__early_initcall_registry_start) /           \
     sizeof(struct early_initcall))

#define REGISTER_EARLY_INITCALL(_name, _init, order)                \
    __early_initcall_registry(order)                                \
    static const struct early_initcall early_initcall_##_name = {   \
        .name = (#_name),                                           \
        .init = (_init),                                            \
    }

#define LATE_INITCALL_REGISTRY ".late_initcall_registry"

#define __late_initcall_registry(order) \
    __attribute__ ((section(LATE_INITCALL_REGISTRY "." #order), used))

typedef void (*late_initcall_t)(void);

struct late_initcall
{
    const char *name;
    late_initcall_t init;
};

#define __late_initcall

extern struct late_initcall __late_initcall_registry_start[];
extern struct late_initcall __late_initcall_registry_end[];

#define num_registered_late_initcalls()                \
    (((u64)__late_initcall_registry_end -              \
      (u64)__late_initcall_registry_start) /           \
     sizeof(struct late_initcall))

#define REGISTER_LATE_INITCALL(_name, _init, order)                     \
    __late_initcall_registry(order)                                     \
    static const struct late_initcall late_initcall_##_name = {         \
        .name = (#_name),                                               \
        .init = (_init),                                                \
    }

/* isr's */

int __map_irq(bool enable, u32 processor_id, u32 irq, u8 vector, 
             bool trig_explicit, bool trig);
             
int map_irq(bool enable, u32 processor_id, u32 irq, u8 vector);
int map_irq_trig(bool enable, u32 processor_id, u32 irq, u8 vector, bool trig);

int register_isr(u32 processor_id, u8 vector, isr_func_t func);
int unregister_isr(u32 processor_id, u8 vector);

#define register_isr_local(vector, func) \
    register_isr(this_processor_id(), (vector), (func))

#define unregister_isr_local(vector) \
    unregister_isr(this_processor_id(), (vector))

/* ipi's */

int __ipi_run_func(u32 processor_id, ipi_func_t func, u64 arg, bool wait);

int ipi_run_func(u32 processor_id, ipi_func_t func, u64 arg, bool wait);
void ipi_run_func_others(ipi_func_t func, u64 arg, bool wait);

void emulate_self_ipi(ipi_func_t func,  u64 arg);

void dead_local(void);
void dead_global(void);

#endif