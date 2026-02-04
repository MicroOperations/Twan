#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/subsys/twanvisor/vportal/vexit.h>
#include <include/subsys/twanvisor/vportal/venter.h>
#include <include/subsys/twanvisor/vportal/vrecovery.h>
#include <include/subsys/twanvisor/vemulate/vcalls.h>
#include <include/subsys/twanvisor/vemulate/vemulate_utils.h>
#include <include/subsys/twanvisor/vemulate/vinfo.h>
#include <include/subsys/twanvisor/visr/vshield.h>
#include <include/subsys/twanvisor/visr/visr_dispatcher.h>

static u32 msr_blocklist[] = {

    /* not emulating rdtscp/rdpid */
    IA32_TSC_AUX,

    /* not emulating xsave */
    IA32_XSS,

    /* not emulating user rdmsr/wrmsr, ia32_barrier shares a cpuid flag with
       urdmsr/uwrmsr, so trap it just incase */
    IA32_USER_MSR_CTL,
    IA32_BARRIER,

    /* not emulating user interrupts */
    IA32_UINTR_RR,
    IA32_UINTR_HANDLER,
    IA32_UINTR_STACKADJUST,
    IA32_UINTR_MISC,
    IA32_UINTR_PD,
    IA32_UINTR_TT,

    /* not emulating uintr timer */
    IA32_UINTR_TIMER,

    /* currently not supporting fred */
    IA32_FRED_CONFIG,
    IA32_FRED_RSP0,
    IA32_FRED_RSP1,
    IA32_FRED_RSP2,
    IA32_FRED_RSP3,
    IA32_FRED_STKLVLS,
    IA32_FRED_SSP1,
    IA32_FRED_SSP2,
    IA32_FRED_SSP3,

    /* disallowing umonitor/umwait */
    IA32_UMWAIT_CONTROL,

    /* not emulating an apic (guests should use our vintc) */
    IA32_XAPIC_DISABLE_STATUS,

    /* hide sr bios done */
    IA32_SR_BIOS_DONE,

    /* hiding monitor/mwait as it can be easily abused */
    IA32_MONITOR_FILTER_SIZE,

    /* hide vmx msr's since we currently dont support nested virt */
    IA32_VMX_BASIC,
    IA32_VMX_PINBASED_CTLS,
    IA32_VMX_PROCBASED_CTLS,
    IA32_VMX_EXIT_CTLS,
    IA32_VMX_ENTRY_CTLS,
    IA32_VMX_MISC,
    IA32_VMX_CR0_FIXED0,
    IA32_VMX_CR0_FIXED1,
    IA32_VMX_CR4_FIXED0,
    IA32_VMX_CR4_FIXED1,
    IA32_VMX_VMCS_ENUM,
    IA32_VMX_PROCBASED_CTLS2,
    IA32_VMX_EPT_VPID_CAP,
    IA32_VMX_TRUE_PINBASED_CTLS,
    IA32_VMX_TRUE_PROCBASED_CTLS,
    IA32_VMX_TRUE_EXIT_CTLS,
    IA32_VMX_TRUE_ENTRY_CTLS,
    IA32_VMX_VMFUNC,
    IA32_VMX_PROCBASED_CTLS3,
    IA32_VMX_EXIT_CTLS2,

    /* trap apic msr's either way (x2apic currently not supported) */
    IA32_APIC_BASE
};

static __unroll_loops bool vis_msr_blocked(u32 msr)
{
    for (u32 i = 0; i < ARRAY_LEN(msr_blocklist); i++) {
        if (msr_blocklist[i] == msr)
            return true;
    }

    return false;
} 

static void vexit_nop(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
}

static void vexit_nop_advance(__unused struct vregs *vregs)
{   
    vcurrent_vcpu_enable_preemption();
    queue_advance_guest();
}

static void vexit_failure_recover(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    vfailure_recover();
}

static void vexit_gp0(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    queue_inject_gp0();
}

static void vexit_ud(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();
    queue_inject_ud();
}

static void vexit_exception(__unused struct vregs *vregs)
{
    /* nmis should currently be routed as normal */

    vectored_events_info_t info = {
        .val = vmread32(VMCS_RO_VMEXIT_INTERRUPT_INFO)
    };

    if (info.fields.nmi_unblocking != 0) {

        guest_interruptibility_state_t state = {
            .val = vmread32(VMCS_GUEST_INTERRUPTIBILITY_STATE)
        };

        state.fields.nmi_blocking = 0;
        __vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, state.val);
    }

    interrupt_type_t int_type = info.fields.vectored_event_type;
    u8 vector = info.fields.vector;

    switch (vector) {
        
        case DEBUG_EXCEPTION:
            queue_inject_db(int_type);
            break;

        case ALIGNMENT_CHECK:
            
            VBUG_ON(info.fields.errcode_delivered == 0);
            VBUG_ON(vmread(VMCS_RO_IDT_VECTORING_ERROR_CODE) != 0);

            queue_inject_ac0();
            break;

        default:
            VBUG_ON(true);
            break;
    }
    
    vcurrent_vcpu_enable_preemption();
}

static void vexit_ext_intr(__unused struct vregs *vregs)
{
    vectored_events_info_t info = {
        .val = vmread32(VMCS_RO_VMEXIT_INTERRUPT_INFO)
    };

    if (info.fields.nmi_unblocking != 0) {

        guest_interruptibility_state_t state = {
            .val = vmread32(VMCS_GUEST_INTERRUPTIBILITY_STATE)
        };

        state.fields.nmi_blocking = 0;
        __vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, state.val);
    }
    
    VBUG_ON(info.fields.vectored_event_type != INTERRUPT_TYPE_EXTERNAL);
    
    vexit_ext_dispatcher(info.fields.vector);
    
    vcurrent_vcpu_enable_preemption();
}

static void vexit_nmi_window(__unused struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    struct vcpu *current = vcurrent_vcpu();
    out_nmi(current);
}

static void vexit_cpuid(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u32 eax = vregs->regs.rax & 0xffffffff;
    u32 ebx = vregs->regs.rbx & 0xffffffff;
    u32 ecx = vregs->regs.rcx & 0xffffffff;
    u32 edx = vregs->regs.rdx & 0xffffffff;
    
    u32 leaf = eax;
    u32 subleaf = ecx;

    if (leaf >= VCPUID_BASE && leaf <= VCPUID_MAX_LEAF) {
        vinfo_dispatcher(vregs);
        queue_advance_guest();
        return;
    }

    __cpuid(&eax, &ebx, &ecx, &edx);

    switch (leaf) {

        case CPUID_FEATURE_BITS:

            feature_bits_b_t feature_bits_b = {.val = ebx};
            feature_bits_c_t feature_bits_c = {.val = ecx};
            feature_bits_d_t feature_bits_d = {.val = edx};

            feature_bits_b.fields.max_addressible_ids = 0;
            feature_bits_b.fields.lapic_id = 0;

            feature_bits_d.fields.mce = 0;
            feature_bits_d.fields.apic = 0;
            feature_bits_d.fields.htt = 0;

            feature_bits_c.fields.monitor_mwait = 0;
            feature_bits_c.fields.vmx = 0;
            feature_bits_c.fields.smx = 0;
            feature_bits_c.fields.xsave = 0;
            feature_bits_c.fields.osxsave = 0;
            feature_bits_c.fields.x2apic = 0;
            feature_bits_c.fields.hypervisor = 1;

            ebx = feature_bits_b.val;
            ecx = feature_bits_c.val;
            edx = feature_bits_d.val;
            break;

        case 4:

            eax &= 0x3fff;
            break;

        case CPUID_EXTENDED_FEATURES:

            switch (subleaf) {

                case 0:

                    extended_features0_c_t extended_features0_c = {.val = ecx};
                    extended_features0_d_t extended_features0_d = {.val = edx};

                    extended_features0_c.fields.waitpkg = 0;
                    extended_features0_c.fields.rdpid = 0;
                    
                    extended_features0_d.fields.uintr = 0;
                    extended_features0_d.fields.pconfig = 0;
                    extended_features0_d.fields.hybrid = 0;

                    ecx = extended_features0_c.val;
                    edx = extended_features0_d.val;
                    break;

                case 1:

                    extended_features1_a_t extended_features1_a = {.val = eax};
                    extended_features1_d_t extended_features1_d = {.val = edx};

                    extended_features1_a.fields.msrlist = 0;
                    extended_features1_a.fields.fred = 0;
                    extended_features1_a.fields.nmi_src = 0;
                    extended_features1_a.fields.cachewarp_mitigation = 0;

                    extended_features1_d.fields.uintr_timer = 0;
                    extended_features1_d.fields.user_msr = 0;
                    extended_features1_d.fields.uiret_uif_from_flags = 0;
                    extended_features1_d.fields.mwait = 0;

                    eax = extended_features1_a.val;
                    edx = extended_features1_d.val;
                    break;

                case 2:

                    extended_features2_d_t extended_features2_d = {.val = edx};
                    
                    extended_features2_d.fields.monitor_mitigation_no = 0;

                    edx = extended_features2_d.val;
                    break;

                default:
                    break;
            }

            break;

        case CPUID_EXTENDED_SIG:
        
            extended_sig_d_t extended_sig_d = {.val = edx};
            
            extended_sig_d.fields.rdtscp = 0;
            edx = extended_sig_d.val;
            break;

        case 5:
        case 0x0b:
        case 0x0d:
        case 0x1f:

            eax = 0;
            ebx = 0;
            ecx = 0;
            edx = 0;
            break;
            
        default:
            break;
    }

    vregs->regs.rax = eax;
    vregs->regs.rbx = ebx;
    vregs->regs.rcx = ecx;
    vregs->regs.rdx = edx;

    queue_advance_guest();
}

static void vexit_vmcall(struct vregs *vregs)
{
    vcall_dispatcher(vregs);
    queue_advance_guest();
}

static void vexit_cr_access(struct vregs *vregs)
{
    cr_access_qualification_t qual = {
        .val = vmread32(VMCS_RO_EXIT_QUALIFICATION)
    };

    vcurrent_vcpu_enable_preemption();

    struct vcpu *current = vcurrent_vcpu();

    u32 cr = qual.fields.cr;

    switch (cr) {

        case 4:

            /* some hypervisor builds will force us to keep vmxe set in the 
               guest, guest should use cpuid to enumerate support for nested
               virtualisation anyway */

            cr4_t cr4 = {.val = vgpr_val(vregs, qual.fields.gpr)};
            cr4.fields.vmxe = 1;

            vper_cpu_feature_flags_t features = vthis_cpu_data()->feature_flags;

            int valid = 1;
            valid &= cr4.fields.mce == 0;
            valid &= features.fields.umip != 0 || cr4.fields.umip == 0;
            valid &= features.fields.la57 != 0 || cr4.fields.la57 == 0;
            valid &= cr4.fields.smxe == 0;
            valid &= features.fields.fsgsbase != 0 || cr4.fields.fsgsbase == 0;
            valid &= features.fields.pcid != 0 || cr4.fields.pcide == 0;
            valid &= cr4.fields.osxsave == 0;
            valid &= features.fields.kl != 0 || cr4.fields.kl == 0;
            valid &= features.fields.smep != 0 || cr4.fields.smep == 0;
            valid &= features.fields.smap != 0 || cr4.fields.smap == 0;
            valid &= features.fields.pke != 0 || cr4.fields.pke == 0;
            valid &= features.fields.cet != 0 || cr4.fields.cet == 0;
            valid &= features.fields.pks != 0 || cr4.fields.pks == 0;
            valid &= cr4.fields.uintr == 0;
            valid &= features.fields.lass != 0 || cr4.fields.lass == 0;
            valid &= features.fields.lam != 0 || cr4.fields.lam == 0;
            valid &= cr4.fields.fred == 0;
            valid &= cr4.fields.reserved0 == 0;
            valid &= cr4.fields.reserved1 == 0;
            valid &= cr4.fields.reserved2 == 0;
            valid &= cr4.fields.reserved3 == 0;

            if (valid == 0) {
                queue_inject_gp0();
                return;
            }

            queue_vmwrite_cr4(cr4);
            break;

        case 8:

            cr8_t val = {.val = vgpr_val(vregs, qual.fields.gpr)};

            if (val.fields.reserved0 != 0) {
                queue_inject_gp0();
                return;
            }

            vregs->cr8 = val.val;
            intl_t intl = {.val = val.val};

            set_intl(current, intl);
            break;

        default:
            break;
    }

    queue_advance_guest();
}

static void vexit_rdmsr(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u32 msr = vregs->regs.rcx & 0xffffffff;

    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    bool is_root = vcurrent_vcpu()->root;

    struct vper_cpu_cache *vcache = &vthis_cpu->vcache;

    switch (msr) {

        case IA32_ARCH_CAPABILITIES:

            ia32_arch_capabilities_t arch_cap = {
                .val = __rdmsrl(IA32_ARCH_CAPABILITIES)
            };

            arch_cap.fields.xapic_disable_status = 0;
            arch_cap.fields.its_no = vthis_cpu->sec_flags.fields.its_no;
            arch_cap.fields.msr_virtual_enumeration_supported = 1;

            vregs->regs.rax = arch_cap.val & 0xffffffff;
            vregs->regs.rdx = arch_cap.val >> 32;
            break;

        case IA32_VIRTUAL_ENUMERATION:

            if (vcache->trap_cache.fields.ia32_arch_capabilities_r == 0) {
                queue_inject_gp0();
                return;
            }

            vregs->regs.rax = 1;
            vregs->regs.rdx = 0;
            break;

        case IA32_VIRTUAL_MITIGATION_ENUM:

            if (vcache->trap_cache.fields.ia32_arch_capabilities_r == 0) {
                queue_inject_gp0();
                return;
            }

            vregs->regs.rax = 0;
            vregs->regs.rdx = 0;
            break;

        case IA32_VIRTUAL_MITIGATION_CTRL:

            if (vcache->trap_cache.fields.ia32_arch_capabilities_r == 0) {
                queue_inject_gp0();
                return;
            }

            vregs->regs.rax = 0;
            vregs->regs.rdx = 0;
            break;

        case IA32_FEATURE_CONTROL:

            if (vcache->trap_cache.fields.ia32_feature_control_rw == 0) {
                queue_inject_gp0();
                return;
            }

            vregs->regs.rax = vcache->via32_feature_ctrl_low;
            vregs->regs.rdx = vcache->via32_feature_ctrl_high;
            break;

        default:

            if (!is_root || vis_msr_blocked(msr)) {
                queue_inject_gp0();
                return;
            }

            u64 flags;
            venter_shield_local_length(&flags, GENERAL_PROTECTION_FAULT, 2);

            u64 val = __rdmsrl(msr);
            
            if (vexit_shield_local(flags)) {
                queue_inject_gp0();
                return;
            } 

            vregs->regs.rax = val & 0xffffffff;
            vregs->regs.rdx = val >> 32;
            break;
    }

    queue_advance_guest();
}

static void vexit_wrmsr(struct vregs *vregs)
{
    vcurrent_vcpu_enable_preemption();

    u32 msr = vregs->regs.rcx & 0xffffffff;
    u32 eax = vregs->regs.rax & 0xffffffff;
    u32 edx = vregs->regs.rdx & 0xffffffff;

    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    struct vper_cpu_cache *vcache = &vthis_cpu->vcache;

    bool is_root = vcurrent_vcpu()->root;
    
    switch (msr) {

        case IA32_VIRTUAL_MITIGATION_CTRL:

            if (vcache->trap_cache.fields.ia32_arch_capabilities_r == 0 || 
                eax != 0 || edx != 0) {

                queue_inject_gp0();
                return;
            }

            break;

        case IA32_FEATURE_CONTROL:

            if (vcache->trap_cache.fields.ia32_feature_control_rw == 0) {
                queue_inject_gp0();
                return;
            }

            if (eax != vcache->via32_feature_ctrl_low || 
                edx != vcache->via32_feature_ctrl_high) {

                queue_inject_gp0();
                return;
            }

            break;

        default:

            if (!is_root || vis_msr_blocked(msr)) {
                queue_inject_gp0();
                return;
            }

            u64 flags;
            venter_shield_local_length(&flags, GENERAL_PROTECTION_FAULT, 2);

            u64 val = ((u64)edx << 32) | eax;
            __wrmsrl(msr, val);
            
            if (vexit_shield_local(flags)) {
                queue_inject_gp0();
                return;
            }

            break;
    }

    queue_advance_guest();
    
}

static void vexit_mce_during_entry(__unused struct vregs *vregs)
{
    /* mce's currently not supported - will cause shutdown */
    
    vcurrent_vcpu_enable_preemption();   
}

static void vexit_vmx_preempt(__unused struct vregs *vregs)
{
    struct vcpu *current = vcurrent_vcpu();

    struct delta_chain *chain = &current->timer_chain;
    struct delta_node *node = delta_chain_popfront_noupdate(chain);

    struct vtimer *vtimer = delta_node_to_vtimer(node);

    bool periodic = vtimer->timer.fields.periodic != 0;
    u8 vector = vtimer->timer.fields.vector;
    bool nmi = vtimer->timer.fields.nmi != 0;
    u32 ticks = vtimer->ticks;

    if (periodic)
        delta_chain_insert(chain, node, ticks);
    else
        vtimer->timer.fields.armed = 0;

    struct delta_node *next_node = delta_chain_peekfront(chain);

    /* TODO: optimise for the case in which the next nodes have a delta of 0
             (batch setting their interrupts as pending), currently they should
             vmexit straight away */

    if (next_node) {

        __vmwrite(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, next_node->delta);

    } else {

        vmx_pinbased_ctls_t pin = {
            .val = vmread32(VMCS_CTRL_PINBASED_CONTROLS)
        };

        vmx_exit_ctls_t exit = {
            .val = vmread32(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS)
        };
                
        pin.fields.activate_vmx_preemption_timer = 0;
        exit.fields.save_vmx_preemption_timer = 0;

        __vmwrite(VMCS_CTRL_PINBASED_CONTROLS, pin.val);
        __vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, exit.val);
    }

    vcurrent_vcpu_enable_preemption();

    vemu_set_interrupt_pending(current, vector, nmi);
}

static vexit_func_t vexit_table[] = {
    [EXIT_REASON_EXCEPTION] = vexit_exception,
    [EXIT_REASON_EXT_INTR] = vexit_ext_intr,
    [EXIT_REASON_TRIPLE_FAULT] = vexit_failure_recover,
    [EXIT_REASON_INIT] = vexit_failure_recover,
    [EXIT_REASON_SIPI] = vexit_failure_recover,
    [EXIT_REASON_INTR_WINDOW] = vexit_nop,
    [EXIT_REASON_NMI_WINDOW] = vexit_nmi_window,
    [EXIT_REASON_TASK_SWITCH] = vexit_gp0,
    [EXIT_REASON_CPUID] = vexit_cpuid,
    [EXIT_REASON_GETSEC] = vexit_ud,
    [EXIT_REASON_INVD] = vexit_nop_advance,
    [EXIT_REASON_VMCALL] = vexit_vmcall,
    [EXIT_REASON_VMCLEAR] = vexit_ud,
    [EXIT_REASON_VMLAUNCH] = vexit_ud,
    [EXIT_REASON_VMPTRLD] = vexit_ud,
    [EXIT_REASON_VMPTRST] = vexit_ud,
    [EXIT_REASON_VMREAD] = vexit_ud,
    [EXIT_REASON_VMRESUME] = vexit_ud,
    [EXIT_REASON_VMWRITE] = vexit_ud,
    [EXIT_REASON_VMXOFF] = vexit_ud,
    [EXIT_REASON_VMXON] = vexit_ud,
    [EXIT_REASON_CR_ACCESS] = vexit_cr_access,
    [EXIT_REASON_INOUT] = vexit_gp0,
    [EXIT_REASON_RDMSR] = vexit_rdmsr,
    [EXIT_REASON_WRMSR] = vexit_wrmsr,
    [EXIT_REASON_MWAIT] = vexit_ud,
    [EXIT_REASON_MONITOR] = vexit_ud,
    [EXIT_REASON_MCE_DURING_ENTRY] = vexit_mce_during_entry,
    [EXIT_REASON_EPT_FAULT] = vexit_failure_recover,
    [EXIT_REASON_EPT_MISCONFIG] = vexit_failure_recover,
    [EXIT_REASON_INVEPT] = vexit_ud,
    [EXIT_REASON_VMX_PREEMPT] = vexit_vmx_preempt,
    [EXIT_REASON_INVVPID] = vexit_ud,
    [EXIT_REASON_WBINVD] = vexit_nop_advance,
    [EXIT_REASON_XSETBV] = vexit_ud
};

void vexit_dispatcher(struct vregs *vregs)
{
    vmexit_reason_t reason = {.val = vmread(VMCS_RO_EXIT_REASON)};
    u32 basic_reason = reason.fields.basic_reason;

    if (reason.fields.vmentry_failure != 0)
        vfailure_recover();

    if (VBUG_ON(basic_reason >= ARRAY_LEN(vexit_table)))
        vfailure_recover();

    vexit_func_t func = vexit_table[basic_reason];
    if (VBUG_ON(!func))
        vfailure_recover();

    /* up to the vexit handler to enable preemption when possible, this should
       be done as soon as it no longer needs to touch vmcs fields */
    vcurrent_vcpu_disable_preemption();

    /* transition ack ipi's that occur here */
    struct vcpu *current = vcurrent_vcpu();
    struct vscheduler *vsched = vthis_vscheduler();
    
    struct mcsnode vsched_node = INITIALIZE_MCSNODE();
    
    vmcs_lock_isr_save(&vsched->lock, &vsched_node);
    current->vsched_metadata.state = VTRANSITIONING;
    vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);

    vipi_ack();

#if TWANVISOR_VIPI_DRAIN_STRICT

    if (basic_reason != EXIT_REASON_EXT_INTR)
        vipi_drain_no_yield();

#endif

    if (current->vsched_metadata.terminate) {
        vcurrent_vcpu_enable_preemption();
        vtransitioning_recover();
    }

    /* can safely be interrupted from here on */
    enable_interrupts();
    
    INDIRECT_BRANCH_SAFE(func(vregs));
    VBUG_ON(!vcurrent_vcpu_is_preemption_enabled());

    __venter();
}

#endif