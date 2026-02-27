#ifndef _COUNTER_H_
#define _COUNTER_H_

#include <std.h>

typedef u64 (*read_counter_t)(void);
typedef u64 (*counter_period_fs_t)(void);
typedef u64 (*counter_frequency_hz_t)(void);

struct counter_interface 
{
    read_counter_t read_counter_func;
    counter_period_fs_t counter_period_fs_func;
    counter_frequency_hz_t counter_frequency_hz_func;
};

struct counter
{
    struct counter_interface *interface;
    bool initialized;
};

int counter_init(struct counter_interface *interface);
bool is_counter_initialized(void);

u64 read_counter(void);
u64 counter_period_fs(void);
u64 counter_frequency_hz(void);

#endif