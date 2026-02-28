#include <initcalls/late_initcalls_conf.h>
#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR
#include <subsys/twanvisor/vmain.h>
#include <kernel/kapi.h>
#include <lib/libtwanvisor/libvinfo.h>

static __late_initcall void cokernel_twanvisor_init(void)
{
    __start_twanvisor();
    kdbgf("twanvisor initialized? : %s\n", tv_detect() ? "yes" : "no");
}

REGISTER_LATE_INITCALL(cokernel_twanvisor_init, cokernel_twanvisor_init,
                      LATE_COKERNEL_TWANVISOR_ORDER);

#endif