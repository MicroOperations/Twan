#include <initcalls/late_initcalls_conf.h>
#include <generated/autoconf.h>

#if LATE_TIME_CMOS && CONFIG_SUBSYS_CLOCK

#include <kernel/kapi.h>
#include <subsys/time/clock.h>
#include <subsys/twanvisor/vconf.h>
#include <kernel/mem/mmu/paging.h>
#include <kernel/acpi_api/acpi_api.h>
#include <lib/libtwanvisor/libvcalls.h>
#include <lib/libtwanvisor/libvc.h>

/* configs */

#define CMOS_VECTOR 250
#define CMOS_IRQ_DEST_PROCESSOR_ID 0

STATIC_ASSERT(CMOS_IRQ_DEST_PROCESSOR_ID < NUM_CPUS);

#define CMOS_IRQ 8

/* chore: need to change this every year */
#define CMOS_DEFAULT_YEAR 2025ULL

/* macros & unions */

#define CMOS_DEFAULT_CENTURY (CMOS_DEFAULT_YEAR / 100)

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

typedef enum 
{
    CMOS_SECONDS,
    CMOS_SECONDS_ALARM,
    CMOS_MINUTES,
    CMOS_MINUTES_ALARM,
    CMOS_HOURS,
    CMOS_HOURS_ALARM,
    CMOS_DAY_OF_WEEK,
    CMOS_DAY_OF_MONTH,
    CMOS_MONTH,
    CMOS_YEAR,
    CMOS_A,
    CMOS_B,
    CMOS_C,
    CMOS_D,
} cmos_rtc_regs_t; 

typedef union
{
    u8 val;
    struct 
    {
        u8 rate_selection_freq : 4;
        u8 time_freq : 3;
        u8 update_in_progress : 1;
    } fields;
} cmos_a_t;

typedef union
{
    u8 val;
    struct 
    {
        u8 daylight_savings_time : 1;
        u8 hour24_mode : 1;
        u8 data_mode : 1;
        u8 enable_square_wave : 1;
        u8 enable_interrupt : 1;
        u8 alarm_interrupt_enable : 1;
        u8 periodic_interrupt : 1;
        u8 clock_update_cycle : 1;
    } fields;
} cmos_b_t;

typedef union
{
    u8 val;
    struct 
    {
        u8 reserved0 : 4;
        u8 uf : 1;
        u8 af : 1;
        u8 pf : 1;
        u8 irqf : 1;
    } fields;
} cmos_c_t;

typedef union
{
    u8 val;
    struct 
    {
        u8 reserved0 : 7;
        u8 cmos_battery_power_alive : 1;
    } fields;
} cmos_d_t;

#define CMOS_NMI_BIT 7
#define CMOS_NMI_MASK (1 << CMOS_NMI_BIT)

#define cmos_wait_for_update() spin_until(!cmos_uip())

/* generic */

static u8 cmos_read(u8 reg)
{
    outb(CMOS_ADDRESS, CMOS_NMI_MASK | reg);
    return inb(CMOS_DATA);
}

static void cmos_write(u8 reg, u8 val)
{
    outb(CMOS_ADDRESS, CMOS_NMI_MASK | reg);
    outb(CMOS_DATA, val);
}

static bool cmos_uip(void)
{
    cmos_a_t cmos_a = {.val = cmos_read(CMOS_A)};
    return cmos_a.fields.update_in_progress;
}

/* cmos */

static u8 cmos_century_reg = 0;

static struct mcslock_isr alarm_lock = INITIALIZE_MCSLOCK_ISR();
static clock_alarm_callback_func_t alarm_callback = NULL;

static void cmos_clock_read(struct clock_time *clock_time)
{
    u8 last_sec;
    u8 last_min;
    u8 last_hour;
    u8 last_day_of_month;
    u8 last_month;
    u32 last_year;
    u32 last_century; 

    u8 sec;
    u8 min;
    u8 hour;
    u8 day_of_month;
    u8 month;
    u32 year;
    u32 century; 

    do {
        cmos_wait_for_update();

        last_sec = cmos_read(CMOS_SECONDS);
        last_min = cmos_read(CMOS_MINUTES);
        last_hour = cmos_read(CMOS_HOURS);
        last_day_of_month = cmos_read(CMOS_DAY_OF_MONTH);
        last_month = cmos_read(CMOS_MONTH);
        last_year = cmos_read(CMOS_YEAR);
        last_century = cmos_century_reg != 0 ? cmos_read(cmos_century_reg) : 
                                               CMOS_DEFAULT_CENTURY;

        cmos_wait_for_update();

        sec = cmos_read(CMOS_SECONDS);
        min = cmos_read(CMOS_MINUTES);
        hour = cmos_read(CMOS_HOURS);
        day_of_month = cmos_read(CMOS_DAY_OF_MONTH);
        month = cmos_read(CMOS_MONTH);
        year = cmos_read(CMOS_YEAR);
        century = cmos_century_reg != 0 ? cmos_read(cmos_century_reg) :
                                          CMOS_DEFAULT_CENTURY;

    } while (last_sec != sec || last_min != min || last_hour != hour ||
             last_day_of_month != day_of_month || last_month != month ||
             last_year != year || last_century != century);

    clock_time->seconds = sec;
    clock_time->minutes = min;
    clock_time->hours = hour;
    clock_time->day_of_month = day_of_month;
    clock_time->month = month;
    clock_time->year = (century * 100) + year;
}

static void cmos_clock_callback_disable_alarm(void)
{
    cmos_b_t cmos_b = {.val = cmos_read(CMOS_B)};
    cmos_b.fields.alarm_interrupt_enable = 0;
    cmos_write(CMOS_B, cmos_b.val);

    alarm_callback = NULL;
}

static void cmos_clock_callback_set_alarm(clock_alarm_callback_func_t callback,
                                          u8 seconds, u8 minutes, u8 hours)
{
    cmos_b_t cmos_b = {.val = cmos_read(CMOS_B)};
    cmos_b.fields.alarm_interrupt_enable = 0;
    cmos_write(CMOS_B, cmos_b.val);

    alarm_callback = callback;

    cmos_write(CMOS_SECONDS_ALARM, seconds);
    cmos_write(CMOS_MINUTES_ALARM, minutes);
    cmos_write(CMOS_HOURS_ALARM, hours);

    cmos_b.fields.alarm_interrupt_enable = 1;
    cmos_write(CMOS_B, cmos_b.val);
}

static void cmos_clock_disable_alarm(void)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&alarm_lock, &node);
    cmos_clock_callback_disable_alarm();
    mcs_unlock_isr_restore(&alarm_lock, &node);
}

static void cmos_clock_set_alarm(clock_alarm_callback_func_t callback, 
                                 u8 seconds, u8 minutes, u8 hours)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&alarm_lock, &node);
    cmos_clock_callback_set_alarm(callback, seconds, minutes, hours);
    mcs_unlock_isr_restore(&alarm_lock, &node);
}

static struct clock_interface clock_interface = {
    .clock_read_func = cmos_clock_read,
    .clock_callback_disable_alarm_func = cmos_clock_callback_disable_alarm,
    .clock_callback_set_alarm_func = cmos_clock_callback_set_alarm,
    .clock_disable_alarm_func = cmos_clock_disable_alarm,
    .clock_set_alarm_func = cmos_clock_set_alarm
};

/* isr */

static int cmos_isr(void)
{
    cmos_c_t cmos_c = {.val = cmos_read(CMOS_C)};

    struct mcsnode node = INITIALIZE_MCSNODE();
    mcs_lock_isr_save(&alarm_lock, &node);

    if (cmos_c.fields.af != 0 && alarm_callback)
        alarm_callback();

    mcs_unlock_isr_restore(&alarm_lock, &node);
    return ISR_DONE;
}

/* init */

static void locate_century_reg(void)
{
    struct twan_kernel *kernel = twan();

    struct acpi_rsdt *rsdt = __map_phys_pg_local(kernel->acpi.rsdt_phys);
    if (!rsdt)
        return;

    struct acpi_fadt *fadt = 
        __early_get_acpi_table(rsdt, ACPI_FADT_SIGNATURE);

    if (!fadt) {
        __unmap_phys_pg_local(rsdt);
        return;
    }

    if (fadt->hdr.length <= PAGE_SIZE)
        cmos_century_reg = fadt->century;

    __early_put_acpi_table(fadt);
    __unmap_phys_pg_local(rsdt);
}

static __late_initcall void cmos_init(void)
{
    (void)cmos_read(CMOS_C);

    cmos_b_t cmos_b = {
        .fields = {
            .hour24_mode = 1,
            .data_mode = 1
        }
    };

    locate_century_reg();

    cmos_write(CMOS_B, cmos_b.val);

    if (register_isr(CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_VECTOR, cmos_isr) < 0)
        return;

    if (map_irq(true, CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_IRQ, CMOS_VECTOR) < 0) {
        unregister_isr(CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_VECTOR);
        return;
    }

#if TWANVISOR_ON

    if (twan()->flags.fields.twanvisor_on != 0) {

        if (tv_vsubscribe_on_cpu(CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_VECTOR) < 0) {
            map_irq(false, CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_IRQ, CMOS_VECTOR);
            unregister_isr(CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_VECTOR);
            return;
        } 

        if (clock_init(&clock_interface) < 0) {
            map_irq(false, CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_IRQ, CMOS_VECTOR);
            unregister_isr(CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_VECTOR);
            tv_vunsubscribe_on_cpu(CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_VECTOR);
        }

        return;
    }

#endif

    if (clock_init(&clock_interface) < 0) {
        map_irq(false, CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_IRQ, CMOS_VECTOR);
        unregister_isr(CMOS_IRQ_DEST_PROCESSOR_ID, CMOS_VECTOR);
    }
}

REGISTER_LATE_INITCALL(cmos, cmos_init, LATE_TIME_CMOS_ORDER);

#endif