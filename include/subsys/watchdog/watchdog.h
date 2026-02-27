#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

#include <generated/autoconf.h>
#include <types.h>

typedef void (*watchdog_pet_func_t)(void);
typedef int (*watchdog_arm_func_t)(u32 ticks);
typedef int (*watchdog_disarm_func_t)(void);

struct watchdog_interface
{
    watchdog_pet_func_t watchdog_pet_func;
    watchdog_arm_func_t watchdog_arm_func;
    watchdog_disarm_func_t watchdog_disarm_func;
};

struct watchdog
{
    struct watchdog_interface *interface;
    bool initialized;
};

int watchdog_init(struct watchdog_interface *interface);

bool is_watchdog_initialized(void);

void watchdog_pet(void);
int watchdog_arm(u32 ticks);
int watchdog_disarm(void);

#endif