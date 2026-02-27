#include <generated/autoconf.h>
#if CONFIG_DEMO_LATE_INITCALLS

#include <kernel/kapi.h>
#include <subsys/debug/kdbg/kdbg.h>

static __late_initcall void demo(void)
{
    kdbg("Hello World!\n");
}

REGISTER_LATE_INITCALL(demo, demo, 1);

#endif