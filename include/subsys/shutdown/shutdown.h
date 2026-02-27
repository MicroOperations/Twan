#ifndef _SHUTDOWN_H_
#define _SHUTDOWN_H_

#include <generated/autoconf.h>
#include <types.h>

typedef void (*shutdown_system_t)(void);

struct shutdown_interface
{
    shutdown_system_t shutdown_system_func;
};

struct shutdown
{
    struct shutdown_interface *interface;
    bool initialized;
};

int shutdown_init(struct shutdown_interface *interface);
bool is_shutdown_initialized(void);

void shutdown_system(void);

#endif