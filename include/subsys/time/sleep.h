#ifndef _SLEEP_H_
#define _SLEEP_H_

#include <generated/autoconf.h>
#include <types.h>

typedef void (*sleep_ticks_t)(u32 ticks);
typedef u64 (*sleep_period_fs_t)(void);
typedef u64 (*sleep_frequency_hz_t)(void);

struct sleep_interface
{
    sleep_ticks_t sleep_ticks_func;
    sleep_period_fs_t sleep_period_fs_func;
    sleep_frequency_hz_t sleep_frequency_hz_func;
};

struct sleep
{
    struct sleep_interface *interface;
    bool initialized;
};

int sleep_init(struct sleep_interface *interface);
bool is_sleep_initialized(void);

void sleep_ticks(u32 ticks);
u64 sleep_period_fs(void);
u64 sleep_frequency_hz(void);

#endif