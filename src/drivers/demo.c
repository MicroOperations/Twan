#include <generated/autoconf.h>
#if CONFIG_DEMO_DRIVERS

#include <kernel/kapi.h>
#include <subsys/debug/kdbg/kdbg.h>

static __driver_init void demo(void)
{
    kdbg("Hello World!\n");
}

REGISTER_DRIVER(demo, demo);

#endif