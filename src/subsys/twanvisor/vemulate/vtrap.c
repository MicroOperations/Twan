#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/subsys/twanvisor/vemulate/vtrap.h>
#include <include/subsys/twanvisor/vemulate/vemulate_utils.h>

void vsetup_traps(struct vper_cpu *vthis_cpu, struct vcpu *vcpu)
{
    vper_cpu_trap_cache_t trap_cache = vthis_cpu->vcache.trap_cache;
    vper_cpu_feature_flags_t feature_flags = vthis_cpu->feature_flags;

    trap_msr(vcpu, IA32_FEATURE_CONTROL);

    if (trap_cache.fields.ia32_arch_capabilities_r != 0)
        trap_msr_read(vcpu, IA32_ARCH_CAPABILITIES);

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

    if (vthis_cpu->sec_flags.fields.sr_bios_done != 0)
        trap_msr(vcpu, IA32_SR_BIOS_DONE);

    if (vthis_cpu->arch_flags.support.fields.x2apic != 0) {
        trap_msr(vcpu, IA32_X2APIC_ID);
        trap_msr(vcpu, IA32_X2APIC_VERSION);
        trap_msr(vcpu, IA32_X2APIC_TPR);
        trap_msr(vcpu, IA32_X2APIC_PPR);
        trap_msr(vcpu, IA32_X2APIC_EOI);
        trap_msr(vcpu, IA32_X2APIC_LDR);
        trap_msr(vcpu, IA32_X2APIC_SIVR);

        trap_msr(vcpu, IA32_X2APIC_ISR0);
        trap_msr(vcpu, IA32_X2APIC_ISR1);
        trap_msr(vcpu, IA32_X2APIC_ISR2);
        trap_msr(vcpu, IA32_X2APIC_ISR3);
        trap_msr(vcpu, IA32_X2APIC_ISR4);
        trap_msr(vcpu, IA32_X2APIC_ISR5);
        trap_msr(vcpu, IA32_X2APIC_ISR6);
        trap_msr(vcpu, IA32_X2APIC_ISR7);

        trap_msr(vcpu, IA32_X2APIC_TMR0);
        trap_msr(vcpu, IA32_X2APIC_TMR1);
        trap_msr(vcpu, IA32_X2APIC_TMR2);
        trap_msr(vcpu, IA32_X2APIC_TMR3);
        trap_msr(vcpu, IA32_X2APIC_TMR4);
        trap_msr(vcpu, IA32_X2APIC_TMR5);
        trap_msr(vcpu, IA32_X2APIC_TMR6);
        trap_msr(vcpu, IA32_X2APIC_TMR7);

        trap_msr(vcpu, IA32_X2APIC_IRR0);
        trap_msr(vcpu, IA32_X2APIC_IRR1);
        trap_msr(vcpu, IA32_X2APIC_IRR2);
        trap_msr(vcpu, IA32_X2APIC_IRR3);
        trap_msr(vcpu, IA32_X2APIC_IRR4);
        trap_msr(vcpu, IA32_X2APIC_IRR5);
        trap_msr(vcpu, IA32_X2APIC_IRR6);
        trap_msr(vcpu, IA32_X2APIC_IRR7);

        trap_msr(vcpu, IA32_X2APIC_ESR);

        trap_msr(vcpu, IA32_X2APIC_LVT_CMCI);
        trap_msr(vcpu, IA32_X2APIC_ICR);

        trap_msr(vcpu, IA32_X2APIC_LVT_TIMER);
        trap_msr(vcpu, IA32_X2APIC_LVT_THERMAL);
        trap_msr(vcpu, IA32_X2APIC_LVT_PMI);
        trap_msr(vcpu, IA32_X2APIC_LVT_LINT0);
        trap_msr(vcpu, IA32_X2APIC_LVT_LINT1);
        trap_msr(vcpu, IA32_X2APIC_LVT_ERROR);
        trap_msr(vcpu, IA32_X2APIC_LVT_INIT_COUNT);
        trap_msr(vcpu, IA32_X2APIC_LVT_CUR_COUNT);

        trap_msr(vcpu, IA32_X2APIC_DIV_CONF);
        trap_msr(vcpu, IA32_X2APIC_SELF_IPI);
    }

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