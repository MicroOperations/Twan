#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vemulate/vtrap.h>
#include <subsys/twanvisor/vemulate/vemulate_utils.h>

void vsetup_traps(struct vper_cpu *vthis_cpu, struct vcpu *vcpu)
{
    vper_cpu_trap_cache_t trap_cache = vthis_cpu->vcache.trap_cache;
    vper_cpu_feature_flags_t feature_flags = vthis_cpu->feature_flags;

    vtrap_msr(vcpu, IA32_FEATURE_CONTROL);

    if (trap_cache.fields.ia32_arch_capabilities_r != 0)
        vtrap_msr_read(vcpu, IA32_ARCH_CAPABILITIES);

    /* trap supported msr's that we do not emulate */

    if (feature_flags.fields.xsave != 0)
        vtrap_msr(vcpu, IA32_XSS);

    if (trap_cache.fields.user_msr != 0) {
        vtrap_msr(vcpu, IA32_USER_MSR_CTL);
        vtrap_msr(vcpu, IA32_BARRIER);
    }

    if (feature_flags.fields.uintr != 0) {
        vtrap_msr(vcpu, IA32_UINTR_RR);
        vtrap_msr(vcpu, IA32_UINTR_HANDLER);
        vtrap_msr(vcpu, IA32_UINTR_STACKADJUST);
        vtrap_msr(vcpu, IA32_UINTR_MISC);
        vtrap_msr(vcpu, IA32_UINTR_PD);
        vtrap_msr(vcpu, IA32_UINTR_TT);
    }

    if (trap_cache.fields.uintr_timer != 0)
        vtrap_msr(vcpu, IA32_UINTR_TIMER);

    if (feature_flags.fields.fred != 0) {
        vtrap_msr(vcpu, IA32_FRED_CONFIG);
        vtrap_msr(vcpu, IA32_FRED_RSP0);
        vtrap_msr(vcpu, IA32_FRED_RSP1);
        vtrap_msr(vcpu, IA32_FRED_RSP2);
        vtrap_msr(vcpu, IA32_FRED_RSP3);
        vtrap_msr(vcpu, IA32_FRED_STKLVLS);
        vtrap_msr(vcpu, IA32_FRED_SSP1);
        vtrap_msr(vcpu, IA32_FRED_SSP2);
        vtrap_msr(vcpu, IA32_FRED_SSP3);
    }

    if (trap_cache.fields.waitpkg != 0)
        vtrap_msr(vcpu, IA32_UMWAIT_CONTROL);

    if (trap_cache.fields.xapic_disable_status != 0)
        vtrap_msr(vcpu, IA32_XAPIC_DISABLE_STATUS);

    if (trap_cache.fields.tsc_aux != 0)
        vtrap_msr(vcpu, IA32_TSC_AUX);

    if (vthis_cpu->sec_flags.fields.sr_bios_done != 0)
        vtrap_msr(vcpu, IA32_SR_BIOS_DONE);

    if (vthis_cpu->arch_flags.support.fields.x2apic != 0) {
        vtrap_msr(vcpu, IA32_X2APIC_ID);
        vtrap_msr(vcpu, IA32_X2APIC_VERSION);
        vtrap_msr(vcpu, IA32_X2APIC_TPR);
        vtrap_msr(vcpu, IA32_X2APIC_PPR);
        vtrap_msr(vcpu, IA32_X2APIC_EOI);
        vtrap_msr(vcpu, IA32_X2APIC_LDR);
        vtrap_msr(vcpu, IA32_X2APIC_SIVR);

        vtrap_msr(vcpu, IA32_X2APIC_ISR0);
        vtrap_msr(vcpu, IA32_X2APIC_ISR1);
        vtrap_msr(vcpu, IA32_X2APIC_ISR2);
        vtrap_msr(vcpu, IA32_X2APIC_ISR3);
        vtrap_msr(vcpu, IA32_X2APIC_ISR4);
        vtrap_msr(vcpu, IA32_X2APIC_ISR5);
        vtrap_msr(vcpu, IA32_X2APIC_ISR6);
        vtrap_msr(vcpu, IA32_X2APIC_ISR7);

        vtrap_msr(vcpu, IA32_X2APIC_TMR0);
        vtrap_msr(vcpu, IA32_X2APIC_TMR1);
        vtrap_msr(vcpu, IA32_X2APIC_TMR2);
        vtrap_msr(vcpu, IA32_X2APIC_TMR3);
        vtrap_msr(vcpu, IA32_X2APIC_TMR4);
        vtrap_msr(vcpu, IA32_X2APIC_TMR5);
        vtrap_msr(vcpu, IA32_X2APIC_TMR6);
        vtrap_msr(vcpu, IA32_X2APIC_TMR7);

        vtrap_msr(vcpu, IA32_X2APIC_IRR0);
        vtrap_msr(vcpu, IA32_X2APIC_IRR1);
        vtrap_msr(vcpu, IA32_X2APIC_IRR2);
        vtrap_msr(vcpu, IA32_X2APIC_IRR3);
        vtrap_msr(vcpu, IA32_X2APIC_IRR4);
        vtrap_msr(vcpu, IA32_X2APIC_IRR5);
        vtrap_msr(vcpu, IA32_X2APIC_IRR6);
        vtrap_msr(vcpu, IA32_X2APIC_IRR7);

        vtrap_msr(vcpu, IA32_X2APIC_ESR);

        vtrap_msr(vcpu, IA32_X2APIC_LVT_CMCI);
        vtrap_msr(vcpu, IA32_X2APIC_ICR);

        vtrap_msr(vcpu, IA32_X2APIC_LVT_TIMER);
        vtrap_msr(vcpu, IA32_X2APIC_LVT_THERMAL);
        vtrap_msr(vcpu, IA32_X2APIC_LVT_PMI);
        vtrap_msr(vcpu, IA32_X2APIC_LVT_LINT0);
        vtrap_msr(vcpu, IA32_X2APIC_LVT_LINT1);
        vtrap_msr(vcpu, IA32_X2APIC_LVT_ERROR);
        vtrap_msr(vcpu, IA32_X2APIC_LVT_INIT_COUNT);
        vtrap_msr(vcpu, IA32_X2APIC_LVT_CUR_COUNT);

        vtrap_msr(vcpu, IA32_X2APIC_DIV_CONF);
        vtrap_msr(vcpu, IA32_X2APIC_SELF_IPI);
    }

    vtrap_msr(vcpu, IA32_MONITOR_FILTER_SIZE);

    vtrap_msr(vcpu, IA32_VMX_BASIC);
    vtrap_msr(vcpu, IA32_VMX_PINBASED_CTLS);
    vtrap_msr(vcpu, IA32_VMX_PROCBASED_CTLS);
    vtrap_msr(vcpu, IA32_VMX_EXIT_CTLS);
    vtrap_msr(vcpu, IA32_VMX_ENTRY_CTLS);
    vtrap_msr(vcpu, IA32_VMX_MISC);
    vtrap_msr(vcpu, IA32_VMX_CR0_FIXED0);
    vtrap_msr(vcpu, IA32_VMX_CR0_FIXED1);
    vtrap_msr(vcpu, IA32_VMX_CR4_FIXED0);
    vtrap_msr(vcpu, IA32_VMX_CR4_FIXED1);
    vtrap_msr(vcpu, IA32_VMX_VMCS_ENUM);
    vtrap_msr(vcpu, IA32_VMX_PROCBASED_CTLS2);
    vtrap_msr(vcpu, IA32_VMX_EPT_VPID_CAP);
    vtrap_msr(vcpu, IA32_VMX_TRUE_PINBASED_CTLS);
    vtrap_msr(vcpu, IA32_VMX_TRUE_PROCBASED_CTLS);
    vtrap_msr(vcpu, IA32_VMX_TRUE_EXIT_CTLS);
    vtrap_msr(vcpu, IA32_VMX_TRUE_ENTRY_CTLS);
    vtrap_msr(vcpu, IA32_VMX_VMFUNC);
    vtrap_msr(vcpu, IA32_VMX_PROCBASED_CTLS3);
    vtrap_msr(vcpu, IA32_VMX_EXIT_CTLS2);

    /* trap apic msr's either way (currently not supporting x2apic) */
    vtrap_msr(vcpu, IA32_APIC_BASE);
}

#endif