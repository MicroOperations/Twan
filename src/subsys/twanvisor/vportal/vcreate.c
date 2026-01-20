#include "include/lib/x86_index.h"
#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/subsys/twanvisor/vportal/vcreate.h>
#include <include/subsys/twanvisor/vportal/vexit.h>
#include <include/subsys/twanvisor/vportal/venter.h>
#include <include/subsys/twanvisor/vportal/vrecovery.h>
#include <include/subsys/twanvisor/vemulate/vemulate_utils.h>
#include <include/subsys/twanvisor/vemulate/vtrap.h>
#include <include/subsys/twanvisor/vemulate/verror.h>
#include <include/subsys/twanvisor/vsched/vsched_mcs.h>
#include <include/subsys/twanvisor/twanvisor.h>

extern void __vexit(void);
extern void __vlaunch(void);

int vcpu_precheck(struct vcpu *vcpu)
{
    u32 vprocessor_id = vqueue_to_vprocessor_id(vcpu->vqueue_id);
    if (vprocessor_id >= vnum_cpus())
        return -EINVAL;

    struct vper_cpu *vtarget_cpu = vper_cpu_data(vprocessor_id);
    vper_cpu_arch_support_t support = vtarget_cpu->arch_flags.support;

    if (support.fields.unrestricted_guest == 0 || support.fields.ept == 0)
        return -EOPNOTSUPP;

    if (vcpu->vpid.fields.enabled != 0) {

        if (support.fields.vpid == 0 || support.fields.invvpid_single == 0)
            return -EOPNOTSUPP;

        if (vcpu->vpid.fields.vpid == 0)
            return -EINVAL;
    }

    if ((vcpu->vlaunch.rip >> 32) != 0)
        return -EINVAL;

    if (vcpu->vsched_metadata.terminate)
        return -EINVAL;

    if (vcpu->vsched_metadata.criticality > VSCHED_MAX_CRITICALITY)
        return -EINVAL;

    if (vcpu->preemption_count > 0)
        return -EINVAL;

    if (vcpu->vboot_state != VBOOT_READY && vcpu->vboot_state != VBOOT_PAUSED)
        return -EINVAL;

    if ((vcpu->arch.io_bitmap_a_phys & 0xfff) != 0)
        return -EINVAL;

    if ((vcpu->arch.io_bitmap_b_phys & 0xfff) != 0)
        return -EINVAL;

    if ((vcpu->arch.msr_bitmap_phys & 0xfff) != 0)
        return -EINVAL;

    if ((vcpu->arch.vmcs_phys & 0xfff) != 0)
        return -EINVAL;

    if ((vcpu->arch.ve_info_area_phys & 0xfff) != 0)
        return -EINVAL;

    if (bmp256_ffs(&vcpu->visr_metadata.subscribed_vectors) != -1)
        return -EINVAL;

    struct bmp256 *available = &vtarget_cpu->available_vectors;
    struct bmp256 *allowed = &vcpu->visr_metadata.allowed_external_vectors;

    if (((~available->bmp[0]) & allowed->bmp[0]) != 0 || 
        ((~available->bmp[1]) & allowed->bmp[1]) != 0 ||
        ((~available->bmp[2]) & allowed->bmp[2]) != 0 ||
        ((~available->bmp[3]) & allowed->bmp[3]) != 0) {

        return -EINVAL;
    }

    return 0;
}

int vpartition_precheck(struct vpartition *vpartition)
{
    struct vtwan_kernel *vkernel = vtwan();

    if (!vpartition)
        return -EINVAL;

    if (vpartition->arch.ept_state.fields.ept_enabled == 0)
        return -EOPNOTSUPP;

    if (vpartition->root)
        return -EINVAL;

    if (vpartition->arch.ept_caching_policy == EPT_RESERVED0 ||
        vpartition->arch.ept_caching_policy == EPT_RESERVED1 ||
        vpartition->arch.ept_caching_policy >= EPT_CACHING_POLICY_GUARD) {

        return -EINVAL;
    }

    if (atomic32_read(&vpartition->num_terminated) != 0)
        return -EINVAL;

    u32 root_num_vcpus = vkernel->root_num_vcpus;
    if (vpartition->terminate_notification.processor_id >= root_num_vcpus)
        return -EINVAL;

    bool nmi = vpartition->terminate_notification.nmi;
    u8 vector = vpartition->terminate_notification.vector;
    if (nmi && vector != NMI)
        return -EINVAL;

    for (u32 i = 0; i < ARRAY_LEN(vpartition->criticality_perms); i++) {

        vcriticality_perm_t perm = vpartition->criticality_perms[i];

        if (perm.fields.write != 0 && 
            perm.fields.max_criticality_writeable > VSCHED_MAX_CRITICALITY) {

            return -EINVAL;
        }
    }

    if (vpartition->vcpu_count == 0)
        return -EINVAL;

    for (u32 i = 0; i < vpartition->vcpu_count; i++) {
        
        struct vcpu *vcpu = &vpartition->vcpus[i];
        if (!vcpu)
            return -EINVAL;

        int err = vcpu_precheck(vcpu);
        if (err < 0)
            return err;
    }

    if (bmp256_ffs(&vpartition->ipi_receivers) != -1 ||
        bmp256_ffs(&vpartition->ipi_senders) != -1 ||
        bmp256_ffs(&vpartition->tlb_shootdown_receivers) != -1 ||
        bmp256_ffs(&vpartition->tlb_shootdown_senders) != -1 ||
        bmp256_ffs(&vpartition->read_vcpu_state_receivers) != -1 ||
        bmp256_ffs(&vpartition->read_vcpu_state_senders) != -1) {

        return -EINVAL;
    }

    return 0;
}

void vcpu_setup(struct vpartition *vpartition, struct vcpu *vcpu, 
                u32 processor_id)
{
    vcpu->vsched_metadata.tlb_flush_pending = false;
    vcpu->vsched_metadata.current_time_slice_ticks = 0;
    vcpu->vsched_metadata.rearm = false;
    vcpu->vsched_metadata.state = VINITIALIZING;
    vcpu->vsched_metadata.pv_spin_state = VSPIN_NONE;

    vcpu->flags.val = 0;

    vcpu->processor_id = processor_id;
    vcpu->vid = vpartition->vid;
    vcpu->vpartition_num_cpus = vpartition->vcpu_count;
    vcpu->vpartition = vpartition;
    
    memset(&vcpu->timers, 0, sizeof(vcpu->timers));
    
    for (u32 i = 0; i < ARRAY_LEN(vcpu->timers); i++)
        vcpu->timers[i].timer.fields.armed = 0;

    vcpu->timer_chain.dq.front = NULL;
    vcpu->timer_chain.dq.rear = NULL;

    memset(&vcpu->vsched_nodes, 0, sizeof(vcpu->vsched_nodes));
    memset(&vcpu->vdispatch_nodes, 0, sizeof(vcpu->vdispatch_nodes));
    memset(&vcpu->ipi_nodes, 0, sizeof(vcpu->ipi_nodes));

    vcpu->visr_pending.delivery.val = 0;
    bmp256_unset_all(&vcpu->visr_pending.pending_external_interrupts);
    mcslock_isr_init(&vcpu->visr_pending.lock);

    vcpu->voperation_queue.pending.val = 0;

    vcpu->context.rip = (u64)vcpu_entry;
    vcpu->context.rsp = (u64)&vcpu->vexit_stack[sizeof(vcpu->vexit_stack)];
}

void vpartition_setup(struct vpartition *vpartition, u8 vid)
{
    u8 root_vid = vtwan()->root_vid;
    vpartition->vid = vid;

    bmp256_set(&vpartition->ipi_senders, root_vid);
    bmp256_set(&vpartition->tlb_shootdown_senders, root_vid);
    bmp256_set(&vpartition->read_vcpu_state_senders, root_vid);

    for (u32 i = 0; i < vpartition->vcpu_count; i++)
        vcpu_setup(vpartition, &vpartition->vcpus[i], i);
}

void vpartition_push(struct vpartition *vpartition)
{
    vpartition_place(vpartition, vpartition->vid);

    /* was thinking about encapsulating the pushes onto the scheduler within 
       the critical section of the vpartition_place operation so that vcpus can
       be made available immediately, however ive settled for this so the caller
       can be made preemptible quicker */

    for (u32 i = 0; i < vpartition->vcpu_count; i++) {
        
        struct vcpu *vcpu = &vpartition->vcpus[i];

        switch (vcpu->vboot_state) {

            case VBOOT_READY:
                vsched_put(vcpu, false, NULL);
                break;

            case VBOOT_PAUSED:
                vsched_put_paused(vcpu, false, false, NULL);
                break;

            default:
                VBUG_ON(true);
                break;
        }
    }
}

void vcpu_entry(void)
{
    struct vtwan_kernel *vkernel = vtwan();
    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    struct vcpu *current = vcurrent_vcpu();

    vper_cpu_arch_support_t support = vthis_cpu->arch_flags.support;

    disable_interrupts();

    current->flags.fields.preempted = 0;

    /* setup ctrls */

    u64 vmcs_phys = current->arch.vmcs_phys;
    u32 rev_id = vthis_cpu->arch_flags.revision_id;
    u32 ia32_vmx_pinbased_ctls = vthis_cpu->arch_flags.ia32_vmx_pinbased_ctls;
    u32 ia32_vmx_procbased_ctls = vthis_cpu->arch_flags.ia32_vmx_procbased_ctls;
    u32 ia32_vmx_exit_ctls = vthis_cpu->arch_flags.ia32_vmx_exit_ctls;
    u32 ia32_vmx_entry_ctls = vthis_cpu->arch_flags.ia32_vmx_entry_ctls;
    vpid_t vpid = current->vpid;

    current->arch.vmcs.header.fields.revision_id = rev_id;

    vmx_pinbased_ctls_t pin = {
        .fields = {
            .external_interrupt_exiting = 1,
            .nmi_exiting = 1,
            .virtual_nmis = 1,
        }
    };

    vmx_procbased_ctls_t proc = {
        .fields = {
            .mwait_exiting = 1,
            .monitor_exiting = 1,
            .cr8_store_exiting = 1,
            .use_io_bitmaps = 1,
            .use_msr_bitmaps = 1,
            .activate_secondary_controls = 1   
        }
    };
    
    vmx_exit_ctls_t exit = {
        .fields = {
            .activate_secondary_controls = 1,
            .host_address_space_size = 1,
            .acknowledge_interrupt_on_exit = 1,
            .save_debug_controls = 1,
            .save_ia32_efer = 1,
            .save_ia32_pat = 1,
            .save_ia32_perf_global_ctl = 1,
            .load_ia32_efer = 1,
            .load_ia32_pat = 1,
            .load_ia32_perf_global_ctrl = 1,
            .load_pkrs = 1,
            .load_cet_state = 1,
            .conceal_vmx_from_pt = 1
        }
    };

    vmx_entry_ctls_t entry = {
        .fields = {
            .conceal_vmx_from_pt = 1,
            .load_cet_state = 1,
            .load_debug_controls = 1,
            .load_ia32_bndcfgs = 1,
            .load_ia32_efer = 1,
            .load_ia32_lbr_ctl = 1,
            .load_ia32_pat = 1,
            .load_ia32_perf_global_ctrl = 1,
            .load_ia32_rtit_ctl = 1,
            .load_pkrs = 1,
            .load_uinv = 1,
            .load_fred = 1,
        }
    };

    if (!__vmclear(vmcs_phys) || !__vmptrld(vmcs_phys))
        vfailure_recover();

    vmwrite_adjusted(ia32_vmx_pinbased_ctls, VMCS_CTRL_PINBASED_CONTROLS, 
                    pin.val);

    vmwrite_adjusted(ia32_vmx_procbased_ctls, VMCS_CTRL_PROCBASED_CTLS, 
                    proc.val);

    vmwrite_adjusted(ia32_vmx_exit_ctls, VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, 
                    exit.val);

    vmwrite_adjusted(ia32_vmx_entry_ctls, VMCS_CTRL_VMENTRY_CONTROLS, 
                    entry.val);

    if (support.fields.procbased_ctls2 != 0) {

        vmx_procbased_ctls2_t proc2 = {
            .fields = {
                .enable_invpcid = 1,
                .conceal_vmx_from_pt = 1,
                .enable_vpid = vpid.fields.enabled,
                .wbinvd_exiting = 1, 
                .enable_ept = 1,
                .ept_violation_ve = 1,
                .unrestricted_guest = 1,
                .intel_pt_use_guest_phys_addr = 1
            }
        };

        vmwrite_adjusted(IA32_VMX_PROCBASED_CTLS2, 
                        VMCS_CTRL_PROCBASED_CTLS2,
                        proc2.val);
    }

    if (support.fields.exit_ctls2 != 0) {

        vmx_exit_ctls2_t exit2 = {
            .fields = {
                .load_fred = 1,
                .save_fred = 1
            }
        };

        vmwrite_adjusted(IA32_VMX_EXIT_CTLS2, 
                        VMCS_CTRL_SECONDARY_VMEXIT_CONTROLS, 
                        exit2.val);
    }

    /* setup host */

    u64 host_rsp = (u64)&current->vexit_stack[sizeof(current->vexit_stack)];

    cr0_t host_cr0 = __read_cr0();
    __vmwrite(VMCS_HOST_CR0, host_cr0.val);
    __vmwrite(VMCS_HOST_CR3, __read_cr3().val);
    __vmwrite(VMCS_HOST_CR4, __read_cr4().val);

    __vmwrite(VMCS_HOST_CS_SELECTOR, VKERNEL_CS);
    __vmwrite(VMCS_HOST_DS_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_SS_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_ES_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_FS_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_GS_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_TR_SELECTOR, vthis_cpu->tr.val);

    __vmwrite(VMCS_HOST_FS_BASE, (u64)&vkernel);
    __vmwrite(VMCS_HOST_GS_BASE, (u64)vthis_cpu);
    __vmwrite(VMCS_HOST_TR_BASE, (u64)&vthis_cpu->tss);
    __vmwrite(VMCS_HOST_GDTR_BASE, vthis_cpu->gdtr.base);
    __vmwrite(VMCS_HOST_IDTR_BASE, vkernel->idtr.base);
    
    __vmwrite(VMCS_HOST_RIP, (u64)__vexit);
    __vmwrite(VMCS_HOST_RSP, host_rsp);
    __vmwrite(VMCS_HOST_IA32_EFER, __rdmsrl(IA32_EFER));
    __vmwrite(VMCS_HOST_IA32_PAT, __rdmsrl(IA32_PAT));

    /* setup guest */

    cr0_t guest_cr0 = {
        .fields = {
            .ne = 1,
            .et = host_cr0.fields.et
        }
    };

    cr4_t guest_cr4 = {
        .fields = {
            .vmxe = 1
        }
    };

    cr4_t cr4_mask = {
        .fields = {
            .vmxe = 1,
            .mce = vthis_cpu->feature_flags.fields.mce,
            .smxe = vthis_cpu->feature_flags.fields.smx,
            .osxsave = vthis_cpu->feature_flags.fields.xsave,
            .uintr = vthis_cpu->feature_flags.fields.uintr,
            .fred = vthis_cpu->feature_flags.fields.fred
        }
    };

    cr4_t cr4_shadow = {
        .fields = {
            .vmxe = 1
        }
    };
    
    __vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, cr4_mask.val);
    __vmwrite(VMCS_CTRL_CR4_READ_SHADOW, cr4_shadow.val);

    __vmwrite(VMCS_GUEST_CR0, guest_cr0.val);
    __vmwrite(VMCS_GUEST_CR4, guest_cr4.val);

    __vmwrite(VMCS_GUEST_CS_SELECTOR, current->vlaunch.cs.val);

    __vmwrite(VMCS_GUEST_CS_BASE, current->vlaunch.cs.fields.index << 4);
    
    __vmwrite(VMCS_GUEST_CS_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_DS_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_SS_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_ES_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_FS_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_GS_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_TR_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_LDTR_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_GDTR_LIMIT, 0xffff);
    __vmwrite(VMCS_GUEST_IDTR_LIMIT, 0xffff);

    access_rights_t default_ar_cs = {
        .fields = {
            .segment_type = 11,
            .descriptor_type = 1,
            .p = 1,
            .g = 1
        }
    };

    access_rights_t default_ar_tr = {
        .fields = {
            .segment_type = 3,
            .p = 1,
            .g = 1
        }
    };

    access_rights_t default_ar_ldtr = {
        .fields = {
            .segment_unusable = 1
        }
    };

    access_rights_t default_ar = {
        .fields = {
            .segment_type = 3,
            .descriptor_type = 1,
            .p = 1,
            .g = 1,
        }
    };

    __vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, default_ar_cs.val);
    __vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, default_ar.val);
    __vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, default_ar.val);
    __vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, default_ar.val);
    __vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, default_ar.val);
    __vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, default_ar.val);
    __vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, default_ar_tr.val);
    __vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, default_ar_ldtr.val);

    /* architectural state */

    rflags_t rflags = {
        .fields = {
            .reserved0 = 1,
            ._if = 1
        }  
    };

    __vmwrite(VMCS_GUEST_RIP, current->vlaunch.rip);
    __vmwrite(VMCS_GUEST_RFLAGS, rflags.val);

    /* access control bitmaps */

    __vmwrite(VMCS_CTRL_IO_BITMAP_A, current->arch.io_bitmap_a_phys);
    __vmwrite(VMCS_CTRL_IO_BITMAP_B, current->arch.io_bitmap_b_phys);
    __vmwrite(VMCS_CTRL_MSR_BITMAPS, current->arch.msr_bitmap_phys);

    /* misc */

    __vmwrite(VMCS_GUEST_ACTIVITY_STATE, active);
    __vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL);

    /* epts */

    struct vpartition *vpartition = current->vpartition;

    eptp_t eptp = {
        .fields = {
            .accessed_dirty_enabled = support.fields.ept_accessed_dirty,
            .walklength = support.fields.ept_pml5 != 0 ? 4 : 3,
            .memtype = vpartition->arch.ept_caching_policy,
            .phys = vpartition->arch.ept_state.fields.ept_base_phys,
        }
    };

    __vmwrite(VMCS_CTRL_EPTP, eptp.val);

    if (support.fields.ept_ve != 0) {

        __vmwrite(VMCS_CTRL_VIRT_EXCEPTION_INFORMATION_ADDRESS, 
                  current->arch.ve_info_area_phys);
    }

    if (vpid.fields.enabled != 0) {
        __vmwrite(VMCS_CTRL_VPID, vpid.val);
        invvpid_single(vpid.val);
    }

    /* setup traps */

    vsetup_traps(vthis_cpu, current);
    
    /* boot the guest */
    
    __venter();
    __vlaunch();
}

#endif