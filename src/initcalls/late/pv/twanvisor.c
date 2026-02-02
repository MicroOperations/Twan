#include <include/initcalls/late_initcalls_conf.h>
#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/lib/libtwanvisor/libvinfo.h>
#include <include/lib/libtwanvisor/libvcalls.h>
#include <include/lib/libtwanvisor/libvc.h>
#include <include/subsys/twanvisor/twanvisor.h>
#include <include/kernel/kapi.h>
#include <include/subsys/time/sleep.h>
#include <include/subsys/time/timeout.h>
#include <include/subsys/watchdog/watchdog.h>

/* config */

#define PV_GP_VECTOR 249
#define PV_GP_DEST_PROCESSOR_ID 0
#define PV_GP_TIMER 1
#define PV_GP_TIMER_PERIOD_MS 1

STATIC_ASSERT(PV_GP_DEST_PROCESSOR_ID < NUM_CPUS);
STATIC_ASSERT(PV_GP_TIMER != PV_SCHED_TIMER);

STATIC_ASSERT(VNUM_VTIMERS >= 2);

/* globals */

static u32 num_vtimers;
static u64 vtimer_frequency_hz;
static u64 vtimer_period_fs;

static u64 gp_timer_frequency_hz_v;
static u64 gp_timer_period_fs_v;

static struct delta_chain gp_sleep_chain = INITIALIZE_DELTA_CHAIN();
static struct mcslock_isr gp_sleep_chain_lock = INITIALIZE_MCSLOCK_ISR();

static struct delta_chain gp_timeout_chain = INITIALIZE_DELTA_CHAIN();
static struct mcslock_isr gp_timeout_chain_lock = INITIALIZE_MCSLOCK_ISR();

/* sleep */

static void sleep_wakeup_task(struct delta_node *node)
{
    struct task *task = sleep_node_to_task(node);
    sched_push(task);
}

static void gp_sleep_ticks_ipi(u64 ticks)
{
    if (ticks == 0)
        return;

    sched_timer_disable();

    struct task *current = current_task();
    struct interrupt_info *ctx = task_ctx();
    KBUG_ON(!current);
    KBUG_ON(!ctx);

    clear_yield_request(current);
    clear_preempted_early(current);

    struct task *task = sched_pop_clean_spin();
    sched_switch_ctx(ctx, task, false);

    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&gp_sleep_chain_lock, &node);
    delta_chain_insert(&gp_sleep_chain, &current->sleep_node, ticks);
    mcs_unlock_isr_restore(&gp_sleep_chain_lock, &node);
    
    if (sched_is_timer_pending())
        set_preempted_early(task);

    sched_timer_enable();
}

static void gp_sleep_ticks(u32 ticks)
{
    emulate_self_ipi(gp_sleep_ticks_ipi, ticks);
}

static u64 gp_timer_period_fs(void)
{
    return gp_timer_period_fs_v;
}

static u64 gp_timer_frequency_hz(void)
{
    return gp_timer_frequency_hz_v;
}

static struct sleep_interface sleep_interface = {
    .sleep_ticks_func = gp_sleep_ticks,
    .sleep_period_fs_func = gp_timer_period_fs,
    .sleep_frequency_hz_func = gp_timer_frequency_hz
};

/* timeout */

static void timeout_wakeup_task(struct delta_node *node)
{
    struct task *task = sleep_node_to_task(node);
    timeout_task_dequeue_from_waitq(task, true);
}

static void gp_timeout_lock(struct mcsnode *node)
{
    mcs_lock_isr_save(&gp_timeout_chain_lock, node);
}

static void gp_timeout_unlock(struct mcsnode *node)
{
    mcs_unlock_isr_restore(&gp_timeout_chain_lock, node);
}

static void __gp_timeout_insert_task(struct task *task, u32 ticks)
{
    delta_chain_insert(&gp_timeout_chain, &task->sleep_node, ticks);
}

static bool __gp_timeout_dequeue_task(struct task *task)
{
    if (!delta_chain_is_queued(&gp_timeout_chain, &task->sleep_node))
        return false;

    delta_chain_dequeue_no_callback(&gp_timeout_chain, &task->sleep_node);
    return true;
}

static u64 gp_timeout_period_fs(void)
{
    return gp_timer_period_fs_v;
}

static u64 gp_timeout_frequency_hz(void)
{
    return gp_timer_frequency_hz_v;
}

static struct timeout_interface timeout_interface = {
    .timeout_lock_func = gp_timeout_lock,
    .timeout_unlock_func = gp_timeout_unlock,

    .__timeout_insert_task_func = __gp_timeout_insert_task,
    .__timeout_dequeue_task_func = __gp_timeout_dequeue_task,
    .timeout_period_fs_func = gp_timeout_period_fs,
    .timeout_frequency_hz_func = gp_timeout_frequency_hz
};

/* isrs */

static int gp_isr(void)
{
    enable_interrupts();

    struct mcsnode sleep_node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&gp_sleep_chain_lock, &sleep_node);
    delta_chain_tick(&gp_sleep_chain, sleep_wakeup_task);
    mcs_unlock_isr_restore(&gp_sleep_chain_lock, &sleep_node);

    struct mcsnode timeout_node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&gp_timeout_chain_lock, &timeout_node);
    delta_chain_tick(&gp_timeout_chain, timeout_wakeup_task);
    mcs_unlock_isr_restore(&gp_timeout_chain_lock, &timeout_node);

    return ISR_DONE;
}

/* init */

static void enumerate_vtimer_info(void)
{
    struct per_cpu *this_cpu = this_cpu_data();

    num_vtimers = this_cpu->num_vtimers;
    vtimer_frequency_hz = this_cpu->vtimer_frequency_hz;
    vtimer_period_fs = this_cpu->vtimer_period_fs;
}

static __late_initcall void pv_twanvisor_init(void)
{
    if (twan()->flags.fields.twanvisor_on == 0)
        return;

    enumerate_vtimer_info();

    if (KBUG_ON(num_vtimers < 3))
        return;

    u64 gp_ticks = ms_to_ticks(PV_GP_TIMER_PERIOD_MS, vtimer_frequency_hz);
   
    if (KBUG_ON(gp_ticks == 0))
        return;

    gp_timer_frequency_hz_v = vtimer_frequency_hz / gp_ticks;
    gp_timer_period_fs_v = vtimer_period_fs * gp_ticks;

    if (register_isr(PV_GP_DEST_PROCESSOR_ID, PV_GP_VECTOR, gp_isr) == 0) {

        KBUG_ON(tv_varm_timer_on_cpu(PV_GP_DEST_PROCESSOR_ID, PV_GP_VECTOR, 
                                     PV_GP_TIMER, false, gp_ticks, true) < 0);

        bool sleep_failed = sleep_init(&sleep_interface) < 0;
        bool timeout_failed = timeout_init(&timeout_interface) < 0;

        if (sleep_failed && timeout_failed) {
            tv_vdisarm_timer_on_cpu(PV_GP_DEST_PROCESSOR_ID, PV_GP_TIMER);
            unregister_isr(PV_GP_DEST_PROCESSOR_ID, PV_GP_VECTOR);
        }
    }
}

REGISTER_LATE_INITCALL(pv_twanvisor_init, pv_twanvisor_init,
                       LATE_PV_TWANVISOR_ORDER);

#endif