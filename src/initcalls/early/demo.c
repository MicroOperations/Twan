#include <generated/autoconf.h>
#if CONFIG_DEMO_EARLY_INITCALLS

#include <kernel/kapi.h>
#include <subsys/debug/kdbg/kdbg.h>

static __early_initcall void demo(void)
{
    kdbg("Hello World!\n");
}

REGISTER_EARLY_INITCALL(demo, demo, 1);

#endif