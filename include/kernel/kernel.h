#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <include/types.h>
#include <include/multiboot2.h>
#include <include/lib/x86_index.h>
#include <include/kernel/boot.h>
#include <include/subsys/sync/spinlock.h>
#include <include/subsys/twanvisor/vconf.h>
#include <uacpi/acpi.h>
#include <include/kernel/sched/sched.h>
#include <include/kernel/isr/isr_index.h>
#include <include/kernel/apic/apic_conf.h>

#define BUF_SIZE 256

#define INT_STACK_SIZE 2048
#define WAKEUP_STACK_SIZE 2048
#define WORKER_STACK_SIZE 2048

#define KERNEL_TSS_IDX 3

/* TODO: these need adjusting to worst case stacksize */
STATIC_ASSERT(INT_STACK_SIZE >= (sizeof(struct regs) + 64));
STATIC_ASSERT(WAKEUP_STACK_SIZE >= 64);
STATIC_ASSERT(WORKER_STACK_SIZE >= 64);

#define IST_NMI 1
#define IST_DF 2
#define IST_MCE 3
#define IST_NORMAL 4

typedef union 
{
    u64 val;
    struct 
    {
        u64 processor_enabled : 1;
        u64 online_capable : 1;
        u64 active : 1;
        u64 bsp : 1;
        u64 sched_init : 1;
        u64 vmx : 1;
        u64 nmis_as_normal : 1;
        u64 reserved0 : 25;
        u64 mxcsr_mask : 32;
    } fields;
} cpu_flags_t;

#define cpu_capable(flags) \
    ((flags).fields.online_capable != 0)

#define cpu_enabled(flags) \
    ((flags).fields.processor_enabled != 0)

#define cpu_possible(flags) \
    (cpu_capable((flags)) || cpu_enabled((flags)))

#define cpu_active(flags) \
    ((flags).fields.active != 0)

#define vmx_supported(flags) \
    ((flags).fields.vmx != 0)

#define mxcsr_mask() \
    (this_cpu_data()->flags.fields.mxcsr_mask)

struct ipi_data
{
    ipi_func_t func;
    u64 arg;
    bool wait;
    atomic32_t signal;

    struct mcslock lock;
};

struct per_cpu
{
    bool handling_isr;
    int intl;

    struct task *current_task;
    struct interrupt_info *task_ctx;

    struct per_cpu *this;

    struct ipi_data ipi_data;

    ipi_func_t self_ipi_func;
    u64 self_ipi_arg;

    u32 processor_id;
    u32 lapic_id;
    u32 acpi_uid;
    cpu_flags_t flags;

    u32 thread_id;
    u32 core_id;
    u32 pkg_id;

#if TWANVISOR_ON

    u32 physical_processor_id;

    u8 num_vtimers;
    u64 vtimer_frequency_hz;
    u64 vtimer_period_fs;

#endif

    atomic_ptr_t isr_table[NUM_VECTORS];
    
    char wakeup_stack[WAKEUP_STACK_SIZE] __aligned(16);

    char int_stack_stub[INT_STACK_SIZE] __aligned(16);
    char nmi_stack[INT_STACK_SIZE] __aligned(16);
    char df_stack[INT_STACK_SIZE] __aligned(16);
    char mce_stack[INT_STACK_SIZE] __aligned(16);

    char int_stacks[NUM_INTLS][INT_STACK_SIZE] __aligned(16);

    struct tss64 tss;
    selector_t tr;

    struct 
    {
        gdt_descriptor32_t descs[3];
        struct gdt_descriptor64 tss_desc;
    } descs __packed;

    struct descriptor_table64 gdtr;

    char worker_stack[WORKER_STACK_SIZE] __aligned(16);
    struct task worker;
};

SIZE_ASSERT(((struct per_cpu *)0)->descs, 
            (sizeof(gdt_descriptor32_t) * 3) + sizeof(struct gdt_descriptor64));

typedef union
{
    u32 val;

    struct
    {
        u32 multicore_initialized : 1;
        u32 bsp_initialized : 1;
        u32 twanvisor_on : 1;
        u32 vid : 8;
        u32 reserved0 : 21;
    } fields;
} twan_flags_t;

struct twan_kernel
{
    struct twan_kernel *this;
    twan_flags_t flags;

#if TWANVISOR_ON
    u32 num_physical_processors;
#endif

    struct
    {
        /* multiboot physaddr */
        struct memory_range multiboot_info;

        /* kernel heap */
        u64 heap_start;
        size_t heap_size;
    } mem;

    struct
    {
        u64 rsdt_phys;

        void *lapic_mmio;
        u64 lapic_mmio_phys;

        struct
        {
            void *ioapic_mmio;
            u64 ioapic_mmio_phys;
            u32 ioapic_gsi_base;
        } ioapic[NUM_IOAPICS];
        u32 num_ioapics;
    } acpi;

    struct 
    {
        struct per_cpu per_cpu_data[NUM_CPUS];
        u32 num_cpus;
        u32 num_capable_cpus;
        u32 num_enabled_cpus;
        u32 bsp;
    } cpu;    

    struct scheduler scheduler;
};

#define twan() \
    ((struct twan_kernel *)__readfs64(offsetof(struct twan_kernel, this)))

#define scheduler() \
    (&twan()->scheduler)

#define num_cpus() \
    (twan()->cpu.num_cpus)

#define this_processor_id() \
    (__readgs32(offsetof(struct per_cpu, processor_id)))

#define this_cpu_data() \
    ((struct per_cpu *)__readgs64(offsetof(struct per_cpu, this)))

#define cpu_valid(processor_id) \
    (((u32)(processor_id)) < num_cpus())

#define cpu_data(processor_id)                                                \
    (cpu_valid(processor_id) ? &twan()->cpu.per_cpu_data[(u32)processor_id] : \
                               NULL)

#define this_lapic_id() \
    (__readgs32(offsetof(struct per_cpu, lapic_id)))

#define lapic_id_of(processor_id)                                   \
    (cpu_valid(processor_id) ?                                      \
     twan()->cpu.per_cpu_data[((u32)processor_id)].lapic_id : ~0U)

#define current_task() \
    ((struct task *)__readgs64(offsetof(struct per_cpu, current_task)))

#define task_ctx() \
    ((struct interrupt_info *)__readgs64(offsetof(struct per_cpu, task_ctx)))

#define set_current_task(task) \
    (__writegs64(offsetof(struct per_cpu, current_task), (u64)(task)))

#endif