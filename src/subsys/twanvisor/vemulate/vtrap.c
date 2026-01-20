#include "include/lib/x86_index.h"
#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/subsys/twanvisor/vemulate/vtrap.h>
#include <include/subsys/twanvisor/vemulate/vemulate_utils.h>

void vsetup_traps(struct vper_cpu *vthis_cpu, struct vcpu *vcpu)
{
    vper_cpu_trap_cache_t trap_cache = vthis_cpu->vcache.trap_cache;
    vper_cpu_feature_flags_t feature_flags = vthis_cpu->feature_flags;

    /* intercept whatever msr's we must */

    if (trap_cache.fields.ia32_arch_capabilities_r != 0)
        trap_msr_read(vcpu, IA32_ARCH_CAPABILITIES);

    if (trap_cache.fields.ia32_feature_control_rw != 0)
        trap_msr(vcpu, IA32_FEATURE_CONTROL);

    /* trap supported msr's that we do not emulate */

    if (feature_flags.fields.xsave != 0)
        trap_msr(vcpu, IA32_XSS);

    if (trap_cache.fields.user_msr != 0) {
        trap_msr(vcpu, IA32_USER_MSR_CTL);
        trap_msr(vcpu, IA32_BARRIER);
    }

    if (feature_flags.fields.uintr != 0) {
        trap_msr(vcpu, IA32_UINTR_RR);
        trap_msr(vcpu, IA32_UINTR_HANDLER);
        trap_msr(vcpu, IA32_UINTR_STACKADJUST);
        trap_msr(vcpu, IA32_UINTR_MISC);
        trap_msr(vcpu, IA32_UINTR_PD);
        trap_msr(vcpu, IA32_UINTR_TT);
    }

    if (trap_cache.fields.uintr_timer != 0)
        trap_msr(vcpu, IA32_UINTR_TIMER);

    if (feature_flags.fields.fred != 0) {
        trap_msr(vcpu, IA32_FRED_CONFIG);
        trap_msr(vcpu, IA32_FRED_RSP0);
        trap_msr(vcpu, IA32_FRED_RSP1);
        trap_msr(vcpu, IA32_FRED_RSP2);
        trap_msr(vcpu, IA32_FRED_RSP3);
        trap_msr(vcpu, IA32_FRED_STKLVLS);
        trap_msr(vcpu, IA32_FRED_SSP1);
        trap_msr(vcpu, IA32_FRED_SSP2);
        trap_msr(vcpu, IA32_FRED_SSP3);
    }

    if (trap_cache.fields.waitpkg != 0)
        trap_msr(vcpu, IA32_UMWAIT_CONTROL);

    if (trap_cache.fields.xapic_disable_status != 0)
        trap_msr(vcpu, IA32_XAPIC_DISABLE_STATUS);

    if (trap_cache.fields.tsc_aux != 0)
        trap_msr(vcpu, IA32_TSC_AUX);

    trap_msr(vcpu, IA32_MONITOR_FILTER_SIZE);

    trap_msr(vcpu, IA32_VMX_BASIC);
    trap_msr(vcpu, IA32_VMX_PINBASED_CTLS);
    trap_msr(vcpu, IA32_VMX_PROCBASED_CTLS);
    trap_msr(vcpu, IA32_VMX_EXIT_CTLS);
    trap_msr(vcpu, IA32_VMX_ENTRY_CTLS);
    trap_msr(vcpu, IA32_VMX_MISC);
    trap_msr(vcpu, IA32_VMX_CR0_FIXED0);
    trap_msr(vcpu, IA32_VMX_CR0_FIXED1);
    trap_msr(vcpu, IA32_VMX_CR4_FIXED0);
    trap_msr(vcpu, IA32_VMX_CR4_FIXED1);
    trap_msr(vcpu, IA32_VMX_VMCS_ENUM);
    trap_msr(vcpu, IA32_VMX_PROCBASED_CTLS2);
    trap_msr(vcpu, IA32_VMX_EPT_VPID_CAP);
    trap_msr(vcpu, IA32_VMX_TRUE_PINBASED_CTLS);
    trap_msr(vcpu, IA32_VMX_TRUE_PROCBASED_CTLS);
    trap_msr(vcpu, IA32_VMX_TRUE_EXIT_CTLS);
    trap_msr(vcpu, IA32_VMX_TRUE_ENTRY_CTLS);
    trap_msr(vcpu, IA32_VMX_VMFUNC);
    trap_msr(vcpu, IA32_VMX_PROCBASED_CTLS3);
    trap_msr(vcpu, IA32_VMX_EXIT_CTLS2);

    /* trap apic msr's either way (currently not supporting x2apic) */
    trap_msr(vcpu, IA32_APIC_BASE);
}

#endif