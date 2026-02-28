#include <initcalls/late_initcalls_conf.h>
#if LATE_TIME_HPET

#include <kernel/kapi.h>
#include <subsys/time/counter.h>
#include <subsys/time/sleep.h>
#include <subsys/time/timeout.h>
#include <kernel/mem/mmu/paging.h>
#include <kernel/apic/apic.h>
#include <kernel/sched/waitq.h>
#include <kernel/acpi_api/acpi_api.h>
#include <subsys/debug/kdbg/kdbg.h>
#include <subsys/debug/kdbg/kdyn_assert.h>
#include <lib/x86_index.h>

/* configs */

#define HPET_VECTOR 251
#define HPET_IRQ_DEST_PROCESSOR_ID 0
#define HPET_TIMER_PERIOD_MS 1

STATIC_ASSERT(HPET_IRQ_DEST_PROCESSOR_ID < NUM_CPUS);

/* macros & unions */

#define HPET_CAPABILITIES_OFFSET 0
#define HPET_GCR_OFFSET 0x10
#define HPET_GISR_OFFSET 0x20
#define HPET_MCVR_OFFSET 0xf0
#define HPET_TIMER0_CONFIG_OFFSET 0x100
#define HPET_TIMER0_COMPARATOR_OFFSET 0x108
#define HPET_TIMER0_FSB_OFFSET 0x110

#define timer_config_offset(timer_no) \
    (HPET_TIMER0_CONFIG_OFFSET + (timer_no * 32))

#define timer_comparator_offset(timer_no) \
    (HPET_TIMER0_COMPARATOR_OFFSET + (timer_no * 32))

#define timer_fsb_offset(timer_no) \
    (HPET_TIMER0_FSB_OFFSET + (timer_no * 32))

typedef union 
{
    u64 val;
    struct 
    {
        u64 rev_id : 8;
        u64 num_timers_cap : 5;
        u64 count_size_cap : 1;
        u64 reserved0 : 1;
        u64 leg_route_cap : 1;
        u64 vendor_id : 16;
        u64 counter_clk_period : 32;
    } fields;
} hpet_capabilities_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 enable_cnf : 1;
        u64 leg_rt_cnf : 1;
        u64 reserved0 : 62;
    } fields;
} hpet_gcr_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 tn_int_sts : 32;
        u64 reserved0 : 32;
    } fields;
} hpet_gisr_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 reserved0 : 1;
        u64 tn_int_type_cnf : 1;
        u64 tn_int_enb_cnf : 1;
        u64 tn_type_cnf : 1;
        u64 tn_per_int_cap : 1;
        u64 tn_size_cap : 1;
        u64 tn_val_set_cnf : 1;
        u64 reserved1 : 1;
        u64 tn_32mode_cnf : 1;
        u64 tn_int_route_cnf : 5;
        u64 tn_fsb_en_cnf : 1;
        u64 tn_fsb_del_cap : 1;
        u64 reserved2 : 16;
        u64 tn_int_route_cap : 32;
    } fields;
} hpet_timer_config_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 tn_fsb_int_val : 32;
        u64 tn_fsb_int_addr : 32;
    } fields;
} hpet_fsb_t;

/* generic */

static volatile u8 *mmio;

static u64 hpet_counter_period_fs_v;
static u64 hpet_counter_frequency_hz_v;

#if !CONFIG_SUBSYS_TWANVISOR && (CONFIG_SUBSYS_SLEEP || CONFIG_SUBSYS_TIMEOUT)

static u64 hpet_timer_period_fs_v;
static u64 hpet_timer_frequency_hz_v;

#endif

static u64 hpet_read(u32 offset)
{
    return *(u64 *)(mmio + offset);
}

static void hpet_write(u32 offset, u64 val)
{
    *(u64 *)(mmio + offset) = val;
}

/* counter */

static void hpet_enable_counter(void)
{
    hpet_gcr_t gcr = {.val = hpet_read(HPET_GCR_OFFSET)};
    gcr.fields.enable_cnf = 1;
    gcr.fields.leg_rt_cnf = 0;
    hpet_write(HPET_GCR_OFFSET, gcr.val);
}

static void hpet_disable_counter(void)
{
    hpet_gcr_t gcr = {.val = hpet_read(HPET_GCR_OFFSET)};
    gcr.fields.enable_cnf = 0;
    hpet_write(HPET_GCR_OFFSET, gcr.val);
}

static void hpet_clear_counter(void)
{
    hpet_write(HPET_MCVR_OFFSET, 0);
}

static u64 hpet_read_counter(void)
{
    return hpet_read(HPET_MCVR_OFFSET);
}

static u64 hpet_counter_period_fs(void)
{
    return hpet_counter_period_fs_v;
}

static u64 hpet_counter_frequency_hz(void)
{
    return hpet_counter_frequency_hz_v;
}

static struct counter_interface counter_interface = {
    .read_counter_func = hpet_read_counter,
    .counter_period_fs_func = hpet_counter_period_fs,
    .counter_frequency_hz_func = hpet_counter_frequency_hz,
};

#if !CONFIG_SUBSYS_TWANVISOR && (CONFIG_SUBSYS_SLEEP || CONFIG_SUBSYS_TIMEOUT)

/* sleep */

#if CONFIG_SUBSYS_SLEEP

static void sleep_wakeup_task(struct delta_node *node)
{
    struct task *task = sleep_node_to_task(node);
    sched_push(task);
}

static struct delta_chain hpet_sleep_chain = INITIALIZE_DELTA_CHAIN();
static struct mcslock_isr hpet_sleep_chain_lock = INITIALIZE_MCSLOCK_ISR();

static void hpet_sleep_ticks_ipi(u64 ticks)
{
    if (ticks == 0)
        return;

    sched_timer_disable();

    struct task *current = current_task();
    struct interrupt_info *ctx = task_ctx();
    KDYNAMIC_ASSERT(current);
    KDYNAMIC_ASSERT(ctx);

    clear_yield_request(current);
    clear_preempted_early(current);

    struct task *task = sched_pop_clean_spin();
    sched_switch_ctx(ctx, task, false);

    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&hpet_sleep_chain_lock, &node);
    delta_chain_insert(&hpet_sleep_chain, &current->sleep_node, ticks);
    mcs_unlock_isr_restore(&hpet_sleep_chain_lock, &node);
    
    if (sched_is_timer_pending())
        set_preempted_early(task);

    sched_timer_enable();
}

static void hpet_sleep_ticks(u32 ticks)
{
    emulate_self_ipi(hpet_sleep_ticks_ipi, ticks);
}

static u64 hpet_timer_period_fs(void)
{
    return hpet_timer_period_fs_v;
}

static u64 hpet_timer_frequency_hz(void)
{
    return hpet_timer_frequency_hz_v;
}

static struct sleep_interface sleep_interface = {
    .sleep_ticks_func = hpet_sleep_ticks,
    .sleep_period_fs_func = hpet_timer_period_fs,
    .sleep_frequency_hz_func = hpet_timer_frequency_hz
};

#endif

/* timeout */

#if CONFIG_SUBSYS_TIMEOUT

static void timeout_wakeup_task(struct delta_node *node)
{
    struct task *task = sleep_node_to_task(node);
    timeout_task_dequeue_from_waitq(task, true);
}

static struct delta_chain hpet_timeout_chain = INITIALIZE_DELTA_CHAIN();
static struct mcslock_isr hpet_timeout_chain_lock = INITIALIZE_MCSLOCK_ISR();

static void hpet_timeout_lock(struct mcsnode *node)
{
    mcs_lock_isr_save(&hpet_timeout_chain_lock, node);
}

static void hpet_timeout_unlock(struct mcsnode *node)
{
    mcs_unlock_isr_restore(&hpet_timeout_chain_lock, node);
}

static void __hpet_timeout_insert_task(struct task *task, u32 ticks)
{
    delta_chain_insert(&hpet_timeout_chain, &task->sleep_node, ticks);
}

static bool __hpet_timeout_dequeue_task(struct task *task)
{
    if (!delta_chain_is_queued(&hpet_timeout_chain, &task->sleep_node))
        return false;

    delta_chain_dequeue_no_callback(&hpet_timeout_chain, &task->sleep_node);
    return true;
}

static u64 hpet_timeout_period_fs(void)
{
    return hpet_timer_period_fs_v;
}

static u64 hpet_timeout_frequency_hz(void)
{
    return hpet_timer_frequency_hz_v;
}

static struct timeout_interface timeout_interface = {
    .timeout_lock_func = hpet_timeout_lock,
    .timeout_unlock_func = hpet_timeout_unlock,

    .__timeout_insert_task_func = __hpet_timeout_insert_task,
    .__timeout_dequeue_task_func = __hpet_timeout_dequeue_task,
    .timeout_period_fs_func = hpet_timeout_period_fs,
    .timeout_frequency_hz_func = hpet_timeout_frequency_hz
};

#endif

/* isr */

static int hpet_isr(void)
{
    enable_interrupts();

#if CONFIG_SUBSYS_SLEEP

    struct mcsnode sleep_node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&hpet_sleep_chain_lock, &sleep_node);
    delta_chain_tick(&hpet_sleep_chain, sleep_wakeup_task);
    mcs_unlock_isr_restore(&hpet_sleep_chain_lock, &sleep_node);

#endif

#if CONFIG_SUBSYS_TIMEOUT

    struct mcsnode timeout_node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&hpet_timeout_chain_lock, &timeout_node);
    delta_chain_tick(&hpet_timeout_chain, timeout_wakeup_task);
    mcs_unlock_isr_restore(&hpet_timeout_chain_lock, &timeout_node);

#endif

    return ISR_DONE;
}

#endif

/* init */

static void hpet_disable_timer(u32 timer_no)
{
    u32 config_offset = timer_config_offset(timer_no);
    hpet_timer_config_t config = {.val = hpet_read(config_offset)};

    config.fields.tn_int_enb_cnf = 0;
    config.fields.tn_type_cnf = 0;
    config.fields.tn_fsb_en_cnf = 0;
    config.fields.tn_int_route_cnf = 0;
    config.fields.tn_int_type_cnf = 0;
    config.fields.tn_val_set_cnf = 0;

    hpet_write(config_offset, config.val);
    hpet_write(timer_comparator_offset(timer_no), -1ULL);
}

static int parse_hpet_table(void)
{
    struct twan_kernel *kernel = twan();

    struct acpi_rsdt *rsdt = __map_phys_pg_local(kernel->acpi.rsdt_phys);
    if (!rsdt)
        return -ENOMEM;

    struct acpi_hpet *hpet_table = 
        __early_get_acpi_table(rsdt, ACPI_HPET_SIGNATURE);

    if (!hpet_table) {
        __unmap_phys_pg_local(rsdt);
        return -ENOENT;
    }

    int ret = -EFAULT;
    
    if (hpet_table->hdr.length <= PAGE_SIZE) {
        
        mmio = map_phys_pg_io(hpet_table->address.address);
        if (mmio)
            ret = 0;
    } 
    
    __early_put_acpi_table(hpet_table);
    __unmap_phys_pg_local(rsdt);
    return ret;
}

static __late_initcall void hpet_init(void)
{   
    if (parse_hpet_table() < 0)
        return;

    hpet_capabilities_t caps = {
        .val = hpet_read(HPET_CAPABILITIES_OFFSET)
    }; 

    u32 num_timers = caps.fields.num_timers_cap + 1;
    hpet_counter_period_fs_v = caps.fields.counter_clk_period;

    if (hpet_counter_period_fs_v == 0)
        return;

    hpet_counter_frequency_hz_v = FEMTOSECOND / caps.fields.counter_clk_period;

    /* disable and clear the counter */
    hpet_disable_counter();
    hpet_clear_counter();

    for (u32 i = 0; i < num_timers; i++)
        hpet_disable_timer(i);

    /* clear any pending interrupts */
    hpet_gisr_t gisr = {.val = hpet_read(HPET_GISR_OFFSET)};
    if (gisr.fields.tn_int_sts != 0)
        hpet_write(HPET_GISR_OFFSET, gisr.val);

    /* configure the hpet counter */
    hpet_enable_counter();

    u64 start = hpet_read_counter();
    for (u32 i = 0; i < 1000; i++)
        cpu_relax();

    u64 after = hpet_read_counter();

    bool initialized = after != start;

    if (!initialized || counter_init(&counter_interface) < 0)
        hpet_disable_counter();

#if !CONFIG_SUBSYS_TWANVISOR && (CONFIG_SUBSYS_SLEEP || CONFIG_SUBSYS_TIMEOUT)

    /* setup the hpet timer0 */
    u64 timer_period_ticks = 
        ms_to_ticks(HPET_TIMER_PERIOD_MS, hpet_counter_frequency_hz_v); 

    if (timer_period_ticks == 0)
        return;

    hpet_timer_config_t config = {.val = hpet_read(HPET_TIMER0_CONFIG_OFFSET)};
    u32 irq_map = config.fields.tn_int_route_cap;

    if (config.fields.tn_per_int_cap == 0)
        return;

    if (config.fields.tn_size_cap == 0 && timer_period_ticks > UINT32_MAX)
        return;

    if (register_isr(HPET_IRQ_DEST_PROCESSOR_ID, HPET_VECTOR, hpet_isr) < 0)
        return;

    int irq;
    while ((irq = ffs32(irq_map)) != -1) {

        if (map_irq_trig(true, HPET_IRQ_DEST_PROCESSOR_ID, irq, HPET_VECTOR, 
                        EDGE_TRIGGERED) == 0) {
            break;
        }

        irq_map &= ~(1U << irq);
    }
    
    if (irq == -1) {
        unregister_isr(HPET_IRQ_DEST_PROCESSOR_ID, HPET_VECTOR);
        return;
    }

    int ret = 0;

#if CONFIG_SUBSYS_SLEEP

    ret |= sleep_init(&sleep_interface) == 0;

#endif

#if CONFIG_SUBSYS_TIMEOUT

    ret |= timeout_init(&timeout_interface) == 0;

#endif

    if (ret == 0) {

        map_irq(false, HPET_IRQ_DEST_PROCESSOR_ID, irq, HPET_VECTOR);
        unregister_isr(HPET_IRQ_DEST_PROCESSOR_ID, HPET_VECTOR);
        return;
    }

    hpet_disable_counter();

    hpet_timer_frequency_hz_v = hpet_counter_frequency_hz_v / 
                                timer_period_ticks;

    hpet_timer_period_fs_v = hpet_counter_period_fs_v * timer_period_ticks;

    config.fields.tn_int_type_cnf = 0;     
    config.fields.tn_int_enb_cnf = 0;
    config.fields.tn_type_cnf = 1;       
    config.fields.tn_val_set_cnf = 1;
    config.fields.tn_int_route_cnf = irq;
    config.fields.tn_fsb_en_cnf = 0;
    config.fields.tn_32mode_cnf = 0;
    hpet_write(HPET_TIMER0_CONFIG_OFFSET, config.val);

    u64 now = hpet_read_counter();
    hpet_write(HPET_TIMER0_COMPARATOR_OFFSET, now + timer_period_ticks);
    hpet_write(HPET_TIMER0_COMPARATOR_OFFSET, timer_period_ticks);

    config.fields.tn_int_enb_cnf = 1;
    hpet_write(HPET_TIMER0_CONFIG_OFFSET, config.val);

    hpet_enable_counter();   
    
#endif
}

REGISTER_LATE_INITCALL(hpet, hpet_init, LATE_TIME_HPET_ORDER);

#endif