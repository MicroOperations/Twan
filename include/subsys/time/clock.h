#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <kernel/isr/isr_index.h>

struct clock_time
{
    u8 seconds;
    u8 minutes;
    u8 hours;
    u8 day_of_month;
    u8 month;
    u32 year;
};

typedef void (*clock_alarm_callback_func_t)(void);

typedef void (*clock_read_t)(struct clock_time *clock_time);

typedef void (*clock_callback_disable_alarm_t)(void);

typedef void (*clock_callback_set_alarm_t)(clock_alarm_callback_func_t callback,
                                           u8 seconds, u8 minutes, u8 hours);

typedef void (*clock_disable_alarm_t)(void);

typedef void (*clock_set_alarm_t)(clock_alarm_callback_func_t callback,
                                  u8 seconds, u8 minutes, u8 hours);

struct clock_interface
{
    clock_read_t clock_read_func;
    clock_callback_disable_alarm_t clock_callback_disable_alarm_func;
    clock_callback_set_alarm_t clock_callback_set_alarm_func;
    clock_disable_alarm_t clock_disable_alarm_func;
    clock_set_alarm_t clock_set_alarm_func;
};

struct clock
{
    struct clock_interface *interface;
    bool initialized;
};

int clock_init(struct clock_interface *interface);

bool is_clock_initialized(void);

void clock_read(struct clock_time *clock_time);

void clock_callback_disable_alarm(void);
void clock_callback_set_alarm(clock_alarm_callback_func_t callback,
                              u8 seconds, u8 minutes, u8 hours);

void clock_disable_alarm(void);

void clock_set_alarm(clock_alarm_callback_func_t callback, 
                     u8 seconds, u8 minutes, u8 hours);

#endif