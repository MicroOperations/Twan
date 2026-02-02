#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/subsys/twanvisor/vmain.h>
#include <include/subsys/twanvisor/vsched/vpartition.h>
#include <include/subsys/twanvisor/vsched/vsched_mcs.h>
#include <include/subsys/twanvisor/vemulate/vemulate_utils.h>
#include <include/subsys/twanvisor/vemulate/verror.h>
#include <include/subsys/twanvisor/vemulate/vtrap.h>
#include <include/subsys/twanvisor/visr/visr_index.h>
#include <include/subsys/twanvisor/vportal/vexit.h>
#include <include/subsys/time/counter.h>
#include <include/kernel/kapi.h>
#include <include/kernel/mem/mmu/paging.h>
#include <include/kernel/extern.h>
#include <include/kernel/apic/apic.h>

extern void __virtualise_core(u64 vprocessor_id);

extern void __vexit(void);

static struct vcpu root_vcpus[NUM_CPUS];
static struct vpartition root = {
    .root = true,
    .vcpus = root_vcpus,
    .vcpu_count = ARRAY_LEN(root_vcpus),

    .num_terminated = INITIALIZE_ATOMIC32(0),
};

static struct vtwan_kernel vkernel = {
    .this = &vkernel,
    .root_num_vcpus = ARRAY_LEN(root_vcpus),
};

void lapic_reconfig(struct vper_cpu *vthis_cpu)
{
    /* reconfigure NMI's as regular interrupts as handling nmi's has too much
       overhead and bullshit associated with it */

     u32 feature_bits_regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&feature_bits_regs[0], &feature_bits_regs[1], 
            &feature_bits_regs[2], &feature_bits_regs[3]);

    u32 ptm_regs[4] = {CPUID_PTM, 0, 0, 0};
    __cpuid(&ptm_regs[0], &ptm_regs[1], &ptm_regs[2], &ptm_regs[3]);

    u32 arch_perfmon_regs[4] = {CPUID_ARCH_PERFMON, 0, 0, 0};
    __cpuid(&arch_perfmon_regs[0], &arch_perfmon_regs[1], 
            &arch_perfmon_regs[2], &arch_perfmon_regs[3]);

    feature_bits_d_t feature_bits_d = {.val = feature_bits_regs[3]};
    ptm_a_t ptm_a = {.val = ptm_regs[0]};
    arch_perfmon_a_t arch_perfmon_a = {.val = arch_perfmon_regs[0]};

    lapic_lint_t lint0 = {
        .val = lapic_read(LAPIC_LINT0_OFFSET)
    };

    lapic_lint_t lint1 = {
        .val = lapic_read(LAPIC_LINT1_OFFSET)
    };

    if (lint0.fields.delivery_mode == DM_NMI) {

        lint0.fields.delivery_mode = DM_NORMAL;
        lint0.fields.vector = NMI;
        lapic_write(LAPIC_LINT0_OFFSET, lint0.val);
        vthis_cpu->vcache.trap_cache.fields.lint0 = 1;
    }

    if (lint1.fields.delivery_mode == DM_NMI) {

        lint1.fields.delivery_mode = DM_NORMAL;
        lint1.fields.vector = NMI;
        lapic_write(LAPIC_LINT1_OFFSET, lint1.val);
        vthis_cpu->vcache.trap_cache.fields.lint1 = 1;
    }
    
    if (feature_bits_d.fields.mca != 0) {

        ia32_mcg_cap_t mcg_cap = {.val = __rdmsrl(IA32_MCG_CAP)};

        if (mcg_cap.fields.mcp_cmci_present != 0) {
            
            lapic_cmci_t cmci = {
                .val = lapic_read(LAPIC_CMCI_OFFSET)
            };

            if (cmci.fields.delivery_mode == DM_NMI) {

                cmci.fields.delivery_mode = DM_NORMAL;
                cmci.fields.vector = NMI;
                lapic_write(LAPIC_CMCI_OFFSET, cmci.val);
                vthis_cpu->vcache.trap_cache.fields.cmci = 1;
            }
        }
    }

    /* genuinely forgot how to check support for the lapic tsr, i think we're to
       check if thermal control msr's and the digital thermal sensor are 
       present, if im wrong someone make a PR  */
    if (feature_bits_d.fields.acpi != 0 && ptm_a.fields.dts != 0) {
        
        lapic_tsr_t tsr = {
            .val = lapic_read(LAPIC_TSR_OFFSET)
        };

        if (tsr.fields.delivery_mode == DM_NMI) {
        
            tsr.fields.delivery_mode = DM_NORMAL;
            tsr.fields.vector = NMI;
            lapic_write(LAPIC_TSR_OFFSET, tsr.val);
            vthis_cpu->vcache.trap_cache.fields.tsr = 1;
        }
    }

    /* also forgot how to check support for the lapic pmcr, if this is wrong
       someone can make a PR */
    if (arch_perfmon_a.fields.version_id > 0) {    

        lapic_pmcr_t pmcr = {
            .val = lapic_read(LAPIC_PMCR_OFFSET)
        };

        if (pmcr.fields.delivery_mode == DM_NMI) {
        
            pmcr.fields.delivery_mode = DM_NORMAL;
            pmcr.fields.vector = NMI;
            lapic_write(LAPIC_PMCR_OFFSET, pmcr.val);
            vthis_cpu->vcache.trap_cache.fields.pmcr = 1;
        }
    }
}

void lapic_reconfig_undo(struct vper_cpu *vthis_cpu)
{
    if (vthis_cpu->vcache.trap_cache.fields.lint0 != 0) {

        lapic_lint_t lint0 = {
            .val = lapic_read(LAPIC_LINT0_OFFSET)
        };

        lint0.fields.delivery_mode = DM_NMI;
        lapic_write(LAPIC_LINT0_OFFSET, lint0.val);
    }

    if (vthis_cpu->vcache.trap_cache.fields.lint1 != 0) {

        lapic_lint_t lint1 = {
            .val = lapic_read(LAPIC_LINT1_OFFSET)
        };

        lint1.fields.delivery_mode = DM_NMI;
        lapic_write(LAPIC_LINT1_OFFSET, lint1.val);
    }

    if (vthis_cpu->vcache.trap_cache.fields.cmci != 0) {

        lapic_cmci_t cmci = {
            .val = lapic_read(LAPIC_CMCI_OFFSET)
        };

        cmci.fields.delivery_mode = DM_NMI;
        lapic_write(LAPIC_CMCI_OFFSET, cmci.val);   
    }

    if (vthis_cpu->vcache.trap_cache.fields.tsr != 0) {
        
        lapic_tsr_t tsr = {
            .val = lapic_read(LAPIC_TSR_OFFSET)
        };

        tsr.fields.delivery_mode = DM_NMI;
        lapic_write(LAPIC_TSR_OFFSET, tsr.val);
    }

    if (vthis_cpu->vcache.trap_cache.fields.pmcr != 0) {
        
        lapic_pmcr_t pmcr = {
            .val = lapic_read(LAPIC_PMCR_OFFSET)
        };

        pmcr.fields.delivery_mode = DM_NMI;
        lapic_write(LAPIC_PMCR_OFFSET, pmcr.val);
    }
}

void vper_cpu_flags_init(struct vper_cpu *vthis_cpu)
{
    u32 feature_bits_regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&feature_bits_regs[0], &feature_bits_regs[1], 
            &feature_bits_regs[2], &feature_bits_regs[3]);

    feature_bits_c_t feature_bits_c = {.val = feature_bits_regs[2]};
    feature_bits_d_t feature_bits_d = {.val = feature_bits_regs[3]};

    u32 ext_regs0[4] = {CPUID_EXTENDED_FEATURES, 0, 0, 0};
    __cpuid(&ext_regs0[0], &ext_regs0[1], &ext_regs0[2], &ext_regs0[3]);

    u32 ext_regs1[4] = {CPUID_EXTENDED_FEATURES, 0, 1, 0};
    __cpuid(&ext_regs1[0], &ext_regs1[1], &ext_regs1[2], &ext_regs1[3]);

    u32 ext_regs2[4] = {CPUID_EXTENDED_FEATURES, 0, 2, 0};
    __cpuid(&ext_regs2[0], &ext_regs2[1], &ext_regs2[2], &ext_regs2[3]);

    extended_features0_b_t ext_features0_b = {.val = ext_regs0[1]};
    extended_features0_c_t ext_features0_c = {.val = ext_regs0[2]};
    extended_features0_d_t ext_features0_d = {.val = ext_regs0[3]};
    extended_features1_a_t ext_features1_a = {.val = ext_regs1[0]};
    extended_features1_d_t ext_features1_d = {.val = ext_regs1[3]};
    extended_features2_d_t extended_features2_d = {.val = ext_regs2[3]};

    /* setup ia32_spec_ctrl */
    ia32_spec_ctrl_t spec_ctrl = {0};

    if (ext_features0_d.fields.ibrs != 0) {
        spec_ctrl.fields.ibrs = CONFIG_MITIGATION_BTI;
        vthis_cpu->sec_flags.fields.ibpb = CONFIG_MITIGATION_BTI;
    }
    
    if (ext_features0_d.fields.stibp != 0)
        spec_ctrl.fields.stibp = CONFIG_MITIGATION_BTI;

    if (ext_features0_d.fields.ssbd != 0)
        spec_ctrl.fields.ssbd = CONFIG_DISABLE_SSB;

    if (extended_features2_d.fields.ipred_ctrl != 0) {
        spec_ctrl.fields.ipred_dis_s = CONFIG_DISABLE_BRANCH_PREDICTION_S;
        spec_ctrl.fields.ipred_dis_u = CONFIG_DISABLE_BRANCH_PREDICTION_U;
    }

    if (extended_features2_d.fields.rrsba_ctrl != 0) {
        spec_ctrl.fields.rrsba_dis_s = CONFIG_MITIGATION_RETBLEED_S;
        spec_ctrl.fields.rrsba_dis_u = CONFIG_MITIGATION_RETBLEED_U;
    }

    if (extended_features2_d.fields.psfd != 0)
        spec_ctrl.fields.psfd = CONFIG_DISABLE_SSTLF;

    if (extended_features2_d.fields.ddpd_u)
        spec_ctrl.fields.ddpd_u = CONFIG_DISABLE_DDP_U;

    if (extended_features2_d.fields.bhi_ctrl != 0)
        spec_ctrl.fields.bhi_dis_s = CONFIG_MITIGATION_BHI;;

    if (spec_ctrl.val != 0)
        __wrmsrl(IA32_SPEC_CTRL, spec_ctrl.val);

    /* check if the l1tf mitigation is available, ia32_arch_capabilities
       will expose whether l1tf is mitigated in hw or not */
    if (ext_features0_d.fields.l1d_flush != 0) {

        vthis_cpu->sec_flags.fields.should_flush_l1d_vmentry = 
            CONFIG_MITIGATION_L1TF;
    }

    /* check if the cachewarp mitigation is supported */
    if (ext_features1_a.fields.cachewarp_mitigation != 0)
        vthis_cpu->sec_flags.fields.sr_bios_done = 1;

    /* setup ia32_mcu_opt */
    ia32_mcu_opt_ctrl_t mcu_opt_ctrl = {0};

    if (ext_features0_d.fields.srbds_ctrl != 0)
        mcu_opt_ctrl.fields.rngds_mit_dis = 0;

    if (ext_features0_d.fields.rtm_always_abort != 0)
        mcu_opt_ctrl.fields.rtm_allow = 0;
    
    if (ext_features0_d.fields.ia32_arch_capabilities != 0) {

        ia32_arch_capabilities_t caps = {
            .val = __rdmsrl(IA32_ARCH_CAPABILITIES)
        };

        if (caps.fields.fb_clear_ctrl != 0) 
            mcu_opt_ctrl.fields.fb_clear_dis = 0;

        if (caps.fields.gds_ctrl != 0) {
            mcu_opt_ctrl.fields.gds_mit_dis = 0;
            mcu_opt_ctrl.fields.gds_mit_locked = 1;
        }

        if (caps.fields.mon_umon_mitg_support != 0) 
            mcu_opt_ctrl.fields.mon_umon_mit = 1;

        /* since we know arch capabilities is supported, check if we need to 
           apply any other policies, and trap it also to expose ITS vuln 
           state */

        if (caps.fields.ibrs_all != 0)
            vthis_cpu->sec_flags.fields.its_no = !may_be_vulnerable_to_its();

        if (caps.fields.skip_l1dfl_vmentry != 0)
            vthis_cpu->sec_flags.fields.should_flush_l1d_vmentry = 0;

        if (caps.fields.doitm != 0)
            __wrmsrl(IA32_UARCH_MISC_CTRL, 1);

        if (caps.fields.fb_clear != 0)
            vthis_cpu->sec_flags.fields.verw_clears_fb = 1;

        if (caps.fields.pbopt_supported != 0)
            __wrmsrl(IA32_PBOPT_CTRL, 1);

        vthis_cpu->vcache.trap_cache.fields.ia32_arch_capabilities_r = 1;
        
        vthis_cpu->vcache.trap_cache.fields.xapic_disable_status =
            caps.fields.xapic_disable_status;
    }

    if (mcu_opt_ctrl.val != 0)
        __wrmsrl(IA32_MCU_OPT_CTRL, mcu_opt_ctrl.val);

    vthis_cpu->feature_flags.fields.mce = feature_bits_d.fields.mce;
    vthis_cpu->feature_flags.fields.umip = ext_features0_c.fields.umip;
    vthis_cpu->feature_flags.fields.la57 = ext_features0_c.fields.la57;
    vthis_cpu->feature_flags.fields.smx = feature_bits_c.fields.smx;
    vthis_cpu->feature_flags.fields.fsgsbase = ext_features0_b.fields.fsgsbase;
    vthis_cpu->feature_flags.fields.pcid = feature_bits_c.fields.pcid;
    vthis_cpu->feature_flags.fields.xsave = feature_bits_c.fields.xsave;
    vthis_cpu->feature_flags.fields.kl = ext_features0_c.fields.kl;
    vthis_cpu->feature_flags.fields.smep = ext_features0_b.fields.smep;
    vthis_cpu->feature_flags.fields.smap = ext_features0_b.fields.smap;
    vthis_cpu->feature_flags.fields.pke = ext_features0_c.fields.pku;
    vthis_cpu->feature_flags.fields.cet = ext_features0_c.fields.cet;
    vthis_cpu->feature_flags.fields.uintr = ext_features0_d.fields.uintr;
    vthis_cpu->feature_flags.fields.lass = ext_features1_a.fields.lass;
    vthis_cpu->feature_flags.fields.lam = ext_features1_a.fields.lam;
    vthis_cpu->feature_flags.fields.fred = ext_features1_a.fields.fred;

    ia32_feature_control_t ia32_feature_control = {
        .val = __rdmsrl(IA32_FEATURE_CONTROL)
    };

    ia32_feature_control.fields.senter_global_enable = 0;
    ia32_feature_control.fields.senter_local_enables = 0;

    vthis_cpu->vcache.via32_feature_ctrl_low = 
        ia32_feature_control.val & 0xffffffff;

    vthis_cpu->vcache.via32_feature_ctrl_high = ia32_feature_control.val >> 32;    

    vthis_cpu->vcache.trap_cache.fields.ia32_feature_control_rw = 
        (ia32_feature_control.fields.senter_global_enable != 0 || 
         ia32_feature_control.fields.senter_local_enables != 0);

    vthis_cpu->vcache.trap_cache.fields.user_msr = 
        ext_features1_d.fields.user_msr;

    vthis_cpu->vcache.trap_cache.fields.uintr_timer = 
        ext_features1_d.fields.uintr_timer;

    vthis_cpu->vcache.trap_cache.fields.waitpkg = 
        ext_features0_c.fields.waitpkg;

    if (ext_features0_c.fields.rdpid == 0) {

        u32 ext_sig_regs[4] = {CPUID_EXTENDED_SIG, 0, 0, 0};
        extended_sig_d_t extended_sig_d = {.val = ext_sig_regs[3]};

        vthis_cpu->vcache.trap_cache.fields.tsc_aux = 
            extended_sig_d.fields.rdtscp;

    } else {
        vthis_cpu->vcache.trap_cache.fields.tsc_aux = 1;
    }

    bool proc2_present = ((__rdmsrl(IA32_VMX_PROCBASED_CTLS) >> 63) & 1) != 0;
    vthis_cpu->arch_flags.support.fields.procbased_ctls2 = proc2_present;
        
    vthis_cpu->arch_flags.support.fields.exit_ctls2 = 
        ((__rdmsrl(IA32_VMX_EXIT_CTLS) >> 63) & 1) != 0;

    if (proc2_present) {

        u64 proc2 = __rdmsrl(IA32_VMX_PROCBASED_CTLS2);

        bool wbinvd_exiting = ((proc2 >> 38) & 1) != 0;
        bool ug = ((proc2 >> 39) & 1) != 0;
        bool ept = ((proc2 >> 33) & 1) != 0;
        bool vpid = ((proc2 >> 37) & 1) != 0;
        bool ve = ((proc2 >> 50) & 1) != 0;
        bool pt_use_gpa = ((proc2 >> 56) & 1) != 0;

        vthis_cpu->arch_flags.support.fields.wbinvd_exiting = wbinvd_exiting;        
        vthis_cpu->arch_flags.support.fields.unrestricted_guest = ug;
        vthis_cpu->arch_flags.support.fields.ept = ept;
        vthis_cpu->arch_flags.support.fields.vpid = vpid;
        vthis_cpu->arch_flags.support.fields.ept_ve = ve;

        if (ept || vpid) {

            ia32_vmx_ept_vpid_cap_t ept_cap = {
                .val = __rdmsrl(IA32_VMX_EPT_VPID_CAP)
            };

            vthis_cpu->arch_flags.support.fields.ept_pml5 = 
                ept_cap.fields.pagewalk_len_5_supported;

            vthis_cpu->arch_flags.support.fields.ept_pml4 = 
                ept_cap.fields.pagewalk_len_4_supported;

            vthis_cpu->arch_flags.support.fields.ept_2mb = 
                ept_cap.fields.pde_2mb_page_supported;

            vthis_cpu->arch_flags.support.fields.ept_1gb = 
                ept_cap.fields.pdpte_1gb_page_supported;

            vthis_cpu->arch_flags.support.fields.ept_wb = 
                ept_cap.fields.ept_wb_supported;
            
            vthis_cpu->arch_flags.support.fields.ept_uc = 
                ept_cap.fields.ept_uc_supported;

            vthis_cpu->arch_flags.support.fields.ept_accessed_dirty = 
                ept_cap.fields.ept_accessed_dirty_supported;

            vthis_cpu->arch_flags.support.fields.invvpid_single =
                ept_cap.fields.single_ctx_invvpid_supported;

            vthis_cpu->arch_flags.support.fields.pt_use_gpa = pt_use_gpa;
        }
    }
}

int vper_cpu_data_init(struct vper_cpu *vthis_cpu, u32 vprocessor_id)
{
    vset_available_vectors(&vthis_cpu->available_vectors);

    vthis_cpu->this = vthis_cpu;
    
    struct per_cpu *this_cpu = this_cpu_data();

    vthis_cpu->processor_id = this_processor_id();
    vthis_cpu->vprocessor_id = vprocessor_id;
    vthis_cpu->lapic_id = this_cpu->lapic_id;
    vthis_cpu->acpi_uid = this_cpu->acpi_uid;

    vthis_cpu->thread_id = this_cpu->thread_id;
    vthis_cpu->core_id = this_cpu->core_id;
    vthis_cpu->pkg_id = this_cpu->pkg_id;

    vthis_cpu->bsp = this_cpu->flags.fields.bsp;

    mcslock_isr_init(&vthis_cpu->dispatch_lock);

    mcslock_isr_init(&vthis_cpu->vipi_data.vcpus.lock);

    vthis_cpu->vscheduler.vcriticality_level = VSCHED_MIN_CRITICALITY;
    mcslock_isr_init(&vthis_cpu->vscheduler.lock);

    vthis_cpu->tss.ist1 = 
        (u64)(&vthis_cpu->nmi_stack[sizeof(vthis_cpu->nmi_stack)]);

    vthis_cpu->tss.ist2 =
        (u64)(&vthis_cpu->df_stack[sizeof(vthis_cpu->df_stack)]);

    vthis_cpu->tss.ist3 = 
        (u64)(&vthis_cpu->mce_stack[sizeof(vthis_cpu->mce_stack)]);

    vthis_cpu->tss.ist4 = 
        (u64)(&vthis_cpu->int_stack[sizeof(vthis_cpu->int_stack)]);

    vthis_cpu->tr.val = VKERNEL_TR;

    vthis_cpu->descs.descs[VKERNEL_CS_IDX].val = VKERNEL_CS_DESC;
    vthis_cpu->descs.descs[VKERNEL_DS_IDX].val = VKERNEL_DS_DESC;

    u64 tss_base = (u64)&vthis_cpu->tss;
    u64 tss_limit = sizeof(struct tss64) - 1;

    struct gdt_descriptor64 *tss_desc = &vthis_cpu->descs.tss_desc;

    tss_desc->descriptor_lower.fields.base_low = tss_base & 0xffff;
    tss_desc->descriptor_lower.fields.base_mid = (tss_base >> 16) & 0xff;
    tss_desc->descriptor_lower.fields.base_high = (tss_base >> 24) & 0xff;
    tss_desc->base_upper = tss_base >> 32;
    tss_desc->descriptor_lower.fields.limit_low = tss_limit & 0xffff;
    tss_desc->descriptor_lower.fields.limit_high = tss_limit >> 16;
    tss_desc->descriptor_lower.fields.present = 1;
    tss_desc->descriptor_lower.fields.segment_type = TSS_AVAILABLE;

    vthis_cpu->gdtr.base = (u64)&vthis_cpu->descs;
    vthis_cpu->gdtr.limit = sizeof(vthis_cpu->descs) - 1;

    if (vthis_cpu->bsp)
        vkernel.bsp = vprocessor_id;

    vper_cpu_flags_init(vthis_cpu);
    lapic_write(LAPIC_DCR_OFFSET, VSCHED_LAPIC_DCR);

    u64 tsc_frequency_hz = 0;
    u64 lapic_frequency_hz = 0;

    u32 regs[4] = {CPUID_TSC_CORE_CRYSTAL, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    if (regs[0] != 0 && regs[1] != 0 && regs[2] != 0) {
        
        tsc_frequency_hz = regs[2] * (regs[1] / regs[0]);
        lapic_frequency_hz = regs[2] / VSCHED_LAPIC_DCR;

    } else {

        u32 regs2[4] = {CPUID_CORE_FREQUENCY, 0, 0, 0};
        __cpuid(&regs2[0], &regs2[1], &regs2[2], &regs2[3]);

        core_frequency_a_t eax = {.val = regs2[0]};
        u64 frequency_mhz = eax.fields.processor_base_freq_mhz;

        if (frequency_mhz != 0) {

            tsc_frequency_hz = frequency_mhz * 1000000;

        } else if (is_counter_initialized()) {

            /* fallback will obviously have a fair bit of variance to it */

            u64 counter_frequency = counter_frequency_hz();

            if (counter_frequency > 0) { 

                u64 tsc_start = __rdtsc64();
                u64 counter_start = read_counter();

                u64 ticks = counter_frequency / 20;
                spin_until(read_counter() - counter_start >= ticks);

                u64 tsc_end = __rdtsc64();
                u64 counter_end = read_counter();

                u64 tsc_delta = tsc_end - tsc_start;
                u64 counter_delta = counter_end - counter_start;

                tsc_frequency_hz = (tsc_delta * counter_frequency) / counter_delta;
                
                struct lapic_calibration cal = calibrate_lapic_timer(
                        VSPURIOUS_INT_VECTOR, 10, VSCHED_LAPIC_DCR);

                lapic_frequency_hz = cal.lapic_frequency_hz;
            }
        }
    }

    /* their cpu is fucked */
    if (tsc_frequency_hz == 0 || lapic_frequency_hz == 0)
        return -EOPNOTSUPP;

    ia32_vmx_misc_t misc = {.val = __rdmsrl(IA32_VMX_MISC)};
    u32 shift = misc.fields.preempt_timer_tsc_relation;
    u64 vmx_preempt_frequency_hz = tsc_frequency_hz / (1 << shift);
    
    u64 tsc_period_fs = FEMTOSECOND / tsc_frequency_hz;
    u64 vmx_preempt_period_fs = FEMTOSECOND / vmx_preempt_frequency_hz;
    u64 lapic_period_fs = FEMTOSECOND / lapic_frequency_hz;

    vthis_cpu->vcache.msr_area_max = 512;

    vthis_cpu->arch_flags.tsc_period_fs = tsc_period_fs;
    vthis_cpu->arch_flags.tsc_frequency_hz = tsc_frequency_hz;

    vthis_cpu->arch_flags.vmx_preempt_period_fs = vmx_preempt_period_fs;
    vthis_cpu->arch_flags.vmx_preempt_frequency_hz = vmx_preempt_frequency_hz;

    vthis_cpu->arch_flags.lapic_period_fs = lapic_period_fs;
    vthis_cpu->arch_flags.lapic_frequency_hz = lapic_frequency_hz;

    ia32_vmx_basic_t basic = {.val = __rdmsrl(IA32_VMX_BASIC)};
    u32 ia32_vmx_pinbased_ctls;
    u32 ia32_vmx_procbased_ctls;
    u32 ia32_vmx_exit_ctls;
    u32 ia32_vmx_entry_ctls;

    if (basic.fields.defaults_to_one_clear != 0) {
        
        ia32_vmx_pinbased_ctls = IA32_VMX_TRUE_PINBASED_CTLS;
        ia32_vmx_procbased_ctls = IA32_VMX_TRUE_PROCBASED_CTLS;
        ia32_vmx_exit_ctls = IA32_VMX_TRUE_EXIT_CTLS;
        ia32_vmx_entry_ctls = IA32_VMX_TRUE_ENTRY_CTLS;

    } else {

        ia32_vmx_pinbased_ctls = IA32_VMX_PINBASED_CTLS;
        ia32_vmx_procbased_ctls = IA32_VMX_PROCBASED_CTLS;
        ia32_vmx_exit_ctls = IA32_VMX_EXIT_CTLS;
        ia32_vmx_entry_ctls = IA32_VMX_ENTRY_CTLS;
    }

    vthis_cpu->arch_flags.revision_id = basic.fields.revision_id;
    vthis_cpu->arch_flags.ia32_vmx_pinbased_ctls = ia32_vmx_pinbased_ctls;
    vthis_cpu->arch_flags.ia32_vmx_procbased_ctls = ia32_vmx_procbased_ctls;
    vthis_cpu->arch_flags.ia32_vmx_exit_ctls = ia32_vmx_exit_ctls;
    vthis_cpu->arch_flags.ia32_vmx_entry_ctls = ia32_vmx_entry_ctls;

    this_cpu->num_vtimers = VNUM_VTIMERS;
    this_cpu->vtimer_frequency_hz = vmx_preempt_frequency_hz;
    this_cpu->vtimer_period_fs = vmx_preempt_period_fs;

    this_cpu->flags.fields.nmis_as_normal = 1;

    return 0;
}

int vkernel_init(void)
{
    struct twan_kernel *kernel = twan();
    u32 num_cpus = kernel->cpu.num_cpus;

    u32 vnum_enabled_cpus = 0;
    for (u32 i = 0; i < num_cpus; i++) {

        struct per_cpu *cpu = &kernel->cpu.per_cpu_data[i];

        if (cpu_enabled(cpu->flags)) {

            if (cpu->flags.fields.vmx == 0)
                return -EOPNOTSUPP;

            struct vper_cpu *vper_cpu = 
                &vkernel.per_cpu_data[vnum_enabled_cpus];
                
            vper_cpu->processor_id = cpu->processor_id; 
            vper_cpu->bsp = cpu->flags.fields.bsp != 0;

            vnum_enabled_cpus++;
        }
    }

    vkernel.lapic_mmio = kernel->acpi.lapic_mmio;
    vkernel.num_enabled_cpus = vnum_enabled_cpus;

    for (u32 i = 0; i < ARRAY_LEN(visr_entry_table); i++) {
        u32 ist_entry;
        switch (i) {

            case NMI:
                ist_entry = VIST_NMI;
                break;

            case DOUBLE_FAULT:
                ist_entry = VIST_DF;
                break;

            case MACHINE_CHECK:
                ist_entry = VIST_MCE;
                break;

            default:
                ist_entry = VIST_NORMAL;
                break;
        }

        vkernel.idt[i].ist = ist_entry;
        vkernel.idt[i].attr.fields.gate_type = INTERRUPT_GATE64;
        vkernel.idt[i].attr.fields.present = 1;
        vkernel.idt[i].selector.val = VKERNEL_CS;
        vkernel.idt[i].offset_low = visr_entry_table[i] & 0xffff;
        vkernel.idt[i].offset_mid = (visr_entry_table[i] >> 16) & 0xffff;
        vkernel.idt[i].offset_high = visr_entry_table[i] >> 32;
    }

    vkernel.idtr.base = (u64)&vkernel.idt;
    vkernel.idtr.limit = sizeof(vkernel.idt) - 1;

    return 0;
}

int root_partition_init(void)
{
    int vid = __vpartition_id_alloc();
    if (vid < 0)
        return -EFAULT;

    /* preset all roots receivers (save us from having to setup routes for root
       upon partition creation) */
    bmp256_set_all(&root.ipi_receivers);
    bmp256_set_all(&root.tlb_shootdown_receivers);
    bmp256_set_all(&root.read_vcpu_state_receivers);

    for (u32 i = 0; i < ARRAY_LEN(root.criticality_perms); i++) {

        root.criticality_perms[i].fields.read = 1;
        root.criticality_perms[i].fields.write = 1;

        root.criticality_perms[i].fields.min_criticality_writeable = 
            VSCHED_MIN_CRITICALITY;

        root.criticality_perms[i].fields.max_criticality_writeable = 
            VSCHED_MAX_CRITICALITY;
    }

    root.vid = vid;
    __vpartition_place(&root, vid);

    vkernel.root_vid = vid;
    return 0;
}

void root_vcpu_init(struct vper_cpu *vthis_cpu, u32 vprocessor_id)
{
    struct per_cpu *this_cpu = this_cpu_data();
    struct vcpu *vcpu = &root_vcpus[vprocessor_id];

    vcpu->arch.vmcs_phys = virt_to_phys_static(&vcpu->arch.vmcs);
    vcpu->root = true;
    vcpu->vsched_metadata.criticality = VROOT_PARTITION_CRITICALITY;
    vcpu->vsched_metadata.pv_spin_state = VSPIN_NONE;
    vcpu->processor_id = this_cpu->core_id;
    vcpu->vid = root.vid;
    vcpu->vpartition_num_cpus = root.vcpu_count;
    vcpu->vpartition = &root;
    vcpu->vqueue_id = vprocessor_to_vqueue_id(vprocessor_id);
    vcpu->visr_metadata.allowed_external_vectors = vthis_cpu->available_vectors;

    vcpu->arch.io_bitmap_a_phys = virt_to_phys_static(vcpu->arch.io_bitmap_a);
    vcpu->arch.io_bitmap_b_phys = virt_to_phys_static(vcpu->arch.io_bitmap_b);
    vcpu->arch.msr_bitmap_phys = virt_to_phys_static(vcpu->arch.msr_bitmap);
}

void __do_virtualise_core(u32 vprocessor_id, u64 rip, u64 rsp, rflags_t rflags)
{
    struct per_cpu *this_cpu = this_cpu_data();

    this_cpu->physical_processor_id = vprocessor_id;

    struct vper_cpu *vthis_cpu = &vkernel.per_cpu_data[vprocessor_id];

    int ret = vper_cpu_data_init(vthis_cpu, vprocessor_id);
    if (ret < 0) {
        kdbg("init vper_cpu failed\n");
        return;
    }

    root_vcpu_init(vthis_cpu, vprocessor_id);
    
    cr0_t cr0 = __read_cr0();
    cr3_t cr3 = __read_cr3();
    cr4_t cr4 = __read_cr4();

    cr0 = adjust_cr0(cr0);
    cr4 = adjust_cr4(cr4);
    cr4.fields.vmxe = 1;

    __write_cr0(cr0);
    __write_cr4(cr4);

    u32 rev_id = vthis_cpu->arch_flags.revision_id;
    u32 ia32_vmx_pinbased_ctls = vthis_cpu->arch_flags.ia32_vmx_pinbased_ctls;
    u32 ia32_vmx_procbased_ctls = vthis_cpu->arch_flags.ia32_vmx_procbased_ctls;
    u32 ia32_vmx_exit_ctls = vthis_cpu->arch_flags.ia32_vmx_exit_ctls;
    u32 ia32_vmx_entry_ctls = vthis_cpu->arch_flags.ia32_vmx_entry_ctls;

    struct vcpu *vcpu = &root.vcpus[vprocessor_id];

    vthis_cpu->arch.vmxon.header.fields.revision_id = rev_id;
    vcpu->arch.vmcs.header.fields.revision_id = rev_id;
    
    if (!__vmxon(virt_to_phys_static(&vthis_cpu->arch.vmxon))) {
        kdbg("vmxon failed\n");
        return;
    }

    if (!__vmclear(vcpu->arch.vmcs_phys)) {
        kdbg("vmclear failed\n");
        return;
    }

    if (!__vmptrld(vcpu->arch.vmcs_phys)) {
        kdbg("vmptrld failed\n");
        return;
    }

    /* ctrls */

    vmx_exception_bitmap_t exception_bmp = {
        .fields = {
            .debug = 1,
            .alignment_check = 1
        }
    };

    vmx_pinbased_ctls_t pin = {
        .fields = {
            .external_interrupt_exiting = 1,
            .virtual_nmis = 1,
            .nmi_exiting = 1,
        }
    };

    vmx_procbased_ctls_t proc = {
        .fields = {
            .mwait_exiting = 1,
            .monitor_exiting = 1,
            .use_io_bitmaps = 1,
            .use_msr_bitmaps = 1,
            .activate_secondary_controls = 1,
            .cr8_load_exiting = 1,
        }
    };

    vmx_procbased_ctls2_t proc2 = {
        .fields = {
            .enable_invpcid = 1,
            .conceal_vmx_from_pt = 1,
            .wbinvd_exiting = 1
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

    vmx_exit_ctls2_t exit2 = {
        .fields = {
            .save_fred = 1,
            .load_fred = 1
        }
    };

    vmx_entry_ctls_t entry = {
        .fields = {
            .conceal_vmx_from_pt = 1,
            .ia32e_mode_guest = 1,
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

    __vmwrite(VMCS_CTRL_EXCEPTION_BITMAP, exception_bmp.val);

    vmwrite_adjusted(ia32_vmx_pinbased_ctls,
                     VMCS_CTRL_PINBASED_CONTROLS,
                     pin.val);

    vmwrite_adjusted(ia32_vmx_procbased_ctls, 
                    VMCS_CTRL_PROCBASED_CTLS, 
                    proc.val);

    if (vthis_cpu->arch_flags.support.fields.procbased_ctls2 != 0) {

        vmwrite_adjusted(IA32_VMX_PROCBASED_CTLS2, 
                        VMCS_CTRL_PROCBASED_CTLS2, 
                        proc2.val);
    }

    vmwrite_adjusted(ia32_vmx_exit_ctls,
                     VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
                     exit.val);

    if (vthis_cpu->arch_flags.support.fields.exit_ctls2 != 0) {

        vmwrite_adjusted(IA32_VMX_EXIT_CTLS2,
                        VMCS_CTRL_SECONDARY_VMEXIT_CONTROLS,
                        exit2.val);
    }

    vmwrite_adjusted(ia32_vmx_entry_ctls,
                     VMCS_CTRL_VMENTRY_CONTROLS,
                     entry.val);

    /* host */

    ia32_efer_t efer = {.val = __rdmsrl(IA32_EFER)};
    ia32_pat_t pat = {.val = __rdmsrl(IA32_PAT)};
    ia32_debug_ctl_t debug_ctl = {.val = __rdmsrl(IA32_DEBUG_CTL)};

    cr0_t host_cr0 = cr0;
    cr3_t host_cr3 = cr3;
    cr4_t host_cr4 = cr4;

    u64 host_rsp = (u64)&vcpu->vexit_stack[sizeof(vcpu->vexit_stack)];
    u64 host_fs_base = (u64)&vkernel;
    u64 host_gs_base = (u64)vthis_cpu;

    __vmwrite(VMCS_HOST_CR0, host_cr0.val);
    __vmwrite(VMCS_HOST_CR3, host_cr3.val);
    __vmwrite(VMCS_HOST_CR4, host_cr4.val);

    __vmwrite(VMCS_HOST_CS_SELECTOR, VKERNEL_CS);
    __vmwrite(VMCS_HOST_DS_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_SS_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_ES_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_FS_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_GS_SELECTOR, VKERNEL_DS);
    __vmwrite(VMCS_HOST_TR_SELECTOR, vthis_cpu->tr.val);

    __vmwrite(VMCS_HOST_FS_BASE, host_fs_base);
    __vmwrite(VMCS_HOST_GS_BASE, host_gs_base);
    __vmwrite(VMCS_HOST_TR_BASE, (u64)&vthis_cpu->tss);
    __vmwrite(VMCS_HOST_GDTR_BASE, vthis_cpu->gdtr.base);
    __vmwrite(VMCS_HOST_IDTR_BASE, vkernel.idtr.base);
    
    __vmwrite(VMCS_HOST_RIP, (u64)__vexit);
    __vmwrite(VMCS_HOST_RSP, host_rsp);
    __vmwrite(VMCS_HOST_IA32_EFER, efer.val);
    __vmwrite(VMCS_HOST_IA32_PAT, pat.val);

    /* guest */

    selector_t guest_cs = __read_cs();
    selector_t guest_ds = __read_ds();
    selector_t guest_ss = __read_ss();
    selector_t guest_es = __read_es();
    selector_t guest_fs = __read_fs();
    selector_t guest_gs = __read_gs();
    selector_t guest_tr = __str();
    selector_t guest_ldtr = __sldt();

    struct descriptor_table64 *guest_gdtr = &this_cpu_data()->gdtr;
    struct descriptor_table64 *guest_idtr = &idtr;

    gdt_descriptor32_t *guest_gdt = (void *)guest_gdtr->base;
        
    cr0_t guest_cr0 = cr0;
    cr3_t guest_cr3 = cr3;
    cr4_t guest_cr4 = cr4;

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
    __vmwrite(VMCS_GUEST_CR3, guest_cr3.val);
    __vmwrite(VMCS_GUEST_CR4, guest_cr4.val);

    __vmwrite(VMCS_GUEST_CS_SELECTOR, guest_cs.val);
    __vmwrite(VMCS_GUEST_DS_SELECTOR, guest_ds.val);
    __vmwrite(VMCS_GUEST_SS_SELECTOR, guest_ss.val);
    __vmwrite(VMCS_GUEST_ES_SELECTOR, guest_es.val);
    __vmwrite(VMCS_GUEST_FS_SELECTOR, guest_fs.val);
    __vmwrite(VMCS_GUEST_GS_SELECTOR, guest_gs.val);
    __vmwrite(VMCS_GUEST_TR_SELECTOR, guest_tr.val);
    __vmwrite(VMCS_GUEST_LDTR_SELECTOR, guest_ldtr.val);

    __vmwrite(VMCS_GUEST_FS_BASE, __rdmsrl(IA32_FS_BASE));
    __vmwrite(VMCS_GUEST_GS_BASE, __rdmsrl(IA32_GS_BASE));
    __vmwrite(VMCS_GUEST_TR_BASE, __segment_base(guest_tr, guest_gdt));
    __vmwrite(VMCS_GUEST_LDTR_BASE, __segment_base(guest_ldtr, guest_gdt));
    __vmwrite(VMCS_GUEST_GDTR_BASE, guest_gdtr->base);
    __vmwrite(VMCS_GUEST_IDTR_BASE, guest_idtr->base);
    
    __vmwrite(VMCS_GUEST_CS_LIMIT, __segment_limit(guest_cs));
    __vmwrite(VMCS_GUEST_DS_LIMIT, __segment_limit(guest_ds));
    __vmwrite(VMCS_GUEST_SS_LIMIT, __segment_limit(guest_ss));
    __vmwrite(VMCS_GUEST_ES_LIMIT, __segment_limit(guest_es));
    __vmwrite(VMCS_GUEST_FS_LIMIT, __segment_limit(guest_fs));
    __vmwrite(VMCS_GUEST_GS_LIMIT, __segment_limit(guest_gs));
    __vmwrite(VMCS_GUEST_TR_LIMIT, __segment_limit(guest_tr));
    __vmwrite(VMCS_GUEST_LDTR_LIMIT, __segment_limit(guest_ldtr));
    __vmwrite(VMCS_GUEST_GDTR_LIMIT, guest_gdtr->limit);
    __vmwrite(VMCS_GUEST_IDTR_LIMIT, guest_idtr->limit);
  
    __vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, __segment_ar(guest_cs).val);
    __vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, __segment_ar(guest_ds).val);
    __vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, __segment_ar(guest_ss).val);
    __vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, __segment_ar(guest_es).val);
    __vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, __segment_ar(guest_fs).val);
    __vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, __segment_ar(guest_gs).val);
    __vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, __segment_ar(guest_tr).val);
    __vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, __segment_ar(guest_ldtr).val);

    __vmwrite(VMCS_GUEST_RIP, rip);
    __vmwrite(VMCS_GUEST_RSP, rsp);
    __vmwrite(VMCS_GUEST_RFLAGS, rflags.val);
    __vmwrite(VMCS_GUEST_DR7, __read_dr7());

    __vmwrite(VMCS_GUEST_ACTIVITY_STATE, active);
    __vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL);

    __vmwrite(VMCS_GUEST_IA32_EFER, efer.val);
    __vmwrite(VMCS_GUEST_IA32_PAT, pat.val);
    __vmwrite(VMCS_GUEST_IA32_DEBUGCTL, debug_ctl.val);

    /* bitmaps */
    __vmwrite(VMCS_CTRL_IO_BITMAP_A, vcpu->arch.io_bitmap_a_phys);
    __vmwrite(VMCS_CTRL_IO_BITMAP_B, vcpu->arch.io_bitmap_b_phys);
    __vmwrite(VMCS_CTRL_MSR_BITMAPS, vcpu->arch.msr_bitmap_phys);

    /* traps */
    vsetup_traps(vthis_cpu, vcpu);

    vcpu->vsched_metadata.state = VRUNNING;
    vthis_cpu->current_vcpu = vcpu;
    
    u64 flags = read_flags_and_disable_interrupts();
    vcpu->visr_pending.delivery.fields.intl = __read_cr8().fields.tpr;

    /* reconfiguring here is kinda dirty, if vmlaunch fails, then any interrupts
       set to fire from the reconfigured lvt entries will stay pending in the 
       irr rather than fire straight away as nmi's 
       
       - this is fine aslong as we are not routing any nmi's before twanvisor
         initializes, which we should not be doing anyway */
    lapic_reconfig(vthis_cpu);

    if (!__vmlaunch()) {
        
        lapic_reconfig_undo(vthis_cpu);
        write_flags(flags);

        kdbgf("vmlaunch failed on processor %d, err %s\n", 
              this_processor_id(), read_vmcs_err());
    }
}

int __start_twanvisor(void)
{
    int ret = vkernel_init();
    if (ret < 0)
        return ret;

    vpartition_table_init();
    
    ret = root_partition_init();
    if (ret < 0)
        return ret;

    u32 bsp = 0;
    
    u32 num_enabled_cpus = vkernel.num_enabled_cpus;
    for (u32 i = 0; i < num_enabled_cpus; i++) {

        struct vper_cpu *vper_cpu = &vkernel.per_cpu_data[i];
        if (vper_cpu->bsp) {
            bsp = i;
            continue;
        }

        ipi_run_func(vper_cpu->processor_id, __virtualise_core, i, true);
    }

    __virtualise_core(bsp);

    struct twan_kernel *kernel = twan();

    kernel->num_physical_processors = num_enabled_cpus;
    kernel->flags.fields.vid = root.vid;
    kernel->flags.fields.twanvisor_on = 1;

    return ret;
}

#endif