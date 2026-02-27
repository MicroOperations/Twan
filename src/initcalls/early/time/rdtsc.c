#include <initcalls/early_initcalls_conf.h>
#if EARLY_TIME_RDTSC

#include <kernel/kapi.h>
#include <subsys/time/counter.h>
#include <lib/x86_index.h>

static u64 period_fs;
static u64 frequency_hz;

static u64 rdtsc_read_counter(void)
{
    return __rdtsc64();
}

static u64 rdtsc_counter_period_fs(void)
{
    return period_fs;
}

static u64 rdtsc_counter_frequency_hz(void)
{
    return frequency_hz;
}

static struct counter_interface counter_interface = {
    .read_counter_func = rdtsc_read_counter,
    .counter_period_fs_func = rdtsc_counter_period_fs,
    .counter_frequency_hz_func = rdtsc_counter_frequency_hz
};

static inline bool is_invariant_tsc_supported(void)
{
    u32 regs[4] = {CPUID_EXTENDED_FUNCTION1, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    extended_function1_d_t edx = {.val = regs[3]};

    return edx.fields.tsc_invariant != 0;
}

static inline u64 get_tsc_frequency(void)
{
    u32 regs[4] = {CPUID_TSC_CORE_CRYSTAL, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    if (regs[0] != 0 && regs[1] != 0 && regs[2] != 0)
        return regs[2] * (regs[1] / regs[0]);

    u32 regs2[4] = {CPUID_CORE_FREQUENCY, 0, 0, 0};
    __cpuid(&regs2[0], &regs2[1], &regs2[2], &regs2[3]);

    core_frequency_a_t eax = {.val = regs2[0]};
    u64 frequency_mhz = eax.fields.processor_base_freq_mhz;

    return frequency_mhz * 1000000;
}

static __early_initcall void rdtsc_init(void)
{
    if (!is_invariant_tsc_supported())
        return;

    frequency_hz = get_tsc_frequency();

    if (frequency_hz == 0)
        return;

    period_fs = FEMTOSECOND / frequency_hz;

    counter_init(&counter_interface);
}

REGISTER_EARLY_INITCALL(rdtsc, rdtsc_init, EARLY_TIME_RDTSC_ORDER);

#endif