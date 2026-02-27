#include <subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <subsys/twanvisor/vportal/vsecc.h>
#include <subsys/twanvisor/twanvisor.h>
#include <generated/autoconf.h>

void vexit_mitigations(void)
{
#if CONFIG_MITIGATION_BTI

    if (vthis_cpu_data()->sec_flags.fields.ibpb)
        ibpb_stub();

#endif

#if CONFIG_MITIGATION_MDS

    if (vthis_cpu_data()->sec_flags.fields.verw_clears_fb)
        verw_fb_clear_stub();

#endif
}

void ventry_mitigations(void)
{
#if CONFIG_MITIGATION_BTI

    if (vthis_cpu_data()->sec_flags.fields.ibpb)
        ibpb_stub();

#endif

#if CONFIG_MITIGATION_MDS

    if (vthis_cpu_data()->sec_flags.fields.verw_clears_fb)
        verw_fb_clear_stub();

#endif

#if CONFIG_MITIGATION_L1TF

    if (vthis_cpu_data()->sec_flags.fields.should_flush_l1d_vmentry)
        l1d_flush_stub();

#endif
}

#endif