#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vemulate/vinfo.h>
#include <subsys/twanvisor/twanvisor.h>

void vinfo_dispatcher(struct vregs *vregs)
{
    u32 leaf = vregs->regs.rax & 0xffffffff;

    switch (leaf) {

        case VCPUID_BASE:
            vinfo_base(vregs);
            break;

        case VCPUID_COMPAT:
            vinfo_compat(vregs);
            break;

        case VCPUID_ROOT_INFO:
            vinfo_root_info(vregs);
            break;

        case VCPUID_EMULATION_FEATURES:
            vinfo_emulation_features(vregs);
            break;

        case VCPUID_VPARTITION_INFO:
            vinfo_vpartition_info(vregs);
            break;

        case VCPUID_VCPU_INFO:
            vinfo_vcpu_info(vregs);
            break;

        default:
            vregs->regs.rax = 0;
            vregs->regs.rbx = 0;
            vregs->regs.rcx = 0;
            vregs->regs.rdx = 0;
            break;
    }
}

void vinfo_base(struct vregs *vregs)
{
    vregs->regs.rax = VCPUID_MAX_LEAF;
    vregs->regs.rbx = VCPUID_BASE_B;
    vregs->regs.rcx = VCPUID_BASE_C;
    vregs->regs.rdx = VCPUID_BASE_D;
}

void vinfo_compat(struct vregs *vregs)
{
    vregs->regs.rax = 0;
    vregs->regs.rbx = 0;
    vregs->regs.rcx = 0;
    vregs->regs.rdx = 0;   
}

void vinfo_root_info(struct vregs *vregs)
{
    if (!vcurrent_vcpu()->root) {
        vregs->regs.rax = 0;
        vregs->regs.rbx = 0;
        vregs->regs.rcx = 0;
        vregs->regs.rdx = 0;
        return;
    }

    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    u32 subleaf = vregs->regs.rcx & 0xffffffff;

    switch (subleaf) {

        case 0:
            vregs->regs.rax = 3;
            vregs->regs.rbx = vthis_cpu->vprocessor_id;
            vregs->regs.rcx = vnum_cpus();

            vcpuid_root_info0_d_t root_info0_d = {
                .fields = {
                    .physical_nmis_disabled = 1,
                    .route_nmis_as_normal = 1
                }
            };
            
            vregs->regs.rdx = root_info0_d.val;
            break;

        case 1:
            u64 vsched_frequency_hz = vthis_cpu->arch_flags.lapic_frequency_hz;
            u64 vsched_period_fs = vthis_cpu->arch_flags.lapic_period_fs;

            vregs->regs.rax = vsched_frequency_hz & 0xffffffff;
            vregs->regs.rbx = vsched_frequency_hz >> 32;
            vregs->regs.rcx = vsched_period_fs & 0xffffffff;
            vregs->regs.rdx = vsched_period_fs >> 32;
            break;

        case 2:
            vregs->regs.rax = vthis_cpu->available_vectors.bmp[0] & 0xffffffff;
            vregs->regs.rbx = vthis_cpu->available_vectors.bmp[0] >> 32;
            vregs->regs.rcx = vthis_cpu->available_vectors.bmp[1] & 0xffffffff;
            vregs->regs.rdx = vthis_cpu->available_vectors.bmp[1] >> 32;
            break;

        case 3:
            vregs->regs.rax = vthis_cpu->available_vectors.bmp[2] & 0xffffffff;
            vregs->regs.rbx = vthis_cpu->available_vectors.bmp[2] >> 32;
            vregs->regs.rcx = vthis_cpu->available_vectors.bmp[3] & 0xffffffff;
            vregs->regs.rdx = vthis_cpu->available_vectors.bmp[3] >> 32;
            break;    
        
        default:
            vregs->regs.rax = 0;
            vregs->regs.rbx = 0;
            vregs->regs.rcx = 0;
            vregs->regs.rdx = 0;
            break;
    }
}

void vinfo_emulation_features(struct vregs *vregs)
{
    u32 subleaf = vregs->regs.rcx & 0xffffffff;

    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    switch (subleaf) {

        case 0:
            vregs->regs.rax = 1;

            vper_cpu_trap_cache_t trap_cache = vthis_cpu->vcache.trap_cache;
            vper_cpu_arch_support_t support = vthis_cpu->arch_flags.support;

            vcpuid_emulation_features0_b_t emulation_features0_b = {
                .fields = {

                    .ia32_feature_control_unconditionally_rw = 
                        trap_cache.fields.ia32_feature_control_rw,

                    .ia32_arch_capabilities_unconditionally_r = 
                        trap_cache.fields.ia32_arch_capabilities_r,

                    .ept_violations_cause_ve = support.fields.ept_ve,
                    
                    .wbinvd_nop = support.fields.wbinvd_exiting,
                    .invd_nop = 1
                }
            };

            vcpuid_emulation_features0_c_t emulation_features0_c = {
                .fields = {
                    .num_vtimers = VNUM_VTIMERS,
                    .msr_area_max = vthis_cpu->vcache.msr_area_max
                }
            };

            vcpuid_emulation_features0_d_t emulation_features0_d = {
                .fields = {
                    .ept = support.fields.ept,
                    .ept_pml4 = support.fields.ept_pml4,
                    .ept_pml5 = support.fields.ept_pml5,
                    .ept_2mb = support.fields.ept_2mb,
                    .ept_1gb = support.fields.ept_1gb,
                    .ept_wb = support.fields.ept_wb,
                    .ept_uc = support.fields.ept_uc,
                    .ept_accessed_dirty = support.fields.ept_accessed_dirty,
                    .vpid = support.fields.vpid & support.fields.invvpid_single,
                    .unrestricted_guest = support.fields.unrestricted_guest,
                    .pt_use_gpa = support.fields.pt_use_gpa
                }
            };

            vregs->regs.rbx = emulation_features0_b.val;
            vregs->regs.rcx = emulation_features0_c.val;
            vregs->regs.rdx = emulation_features0_d.val;
            break;

        case 1:
            struct vper_cpu *vthis_cpu = vthis_cpu_data();

            u64 vtimer_frequency_hz = 
                vthis_cpu->arch_flags.vmx_preempt_frequency_hz;

            u64 vtimer_period_fs = vthis_cpu->arch_flags.vmx_preempt_period_fs;

            vregs->regs.rax = vtimer_frequency_hz & 0xffffffff;
            vregs->regs.rbx = vtimer_frequency_hz >> 32;
            vregs->regs.rcx = vtimer_period_fs & 0xffffffff;
            vregs->regs.rdx = vtimer_period_fs >> 32;
            break;

        default:
            vregs->regs.rax = 0;
            vregs->regs.rbx = 0;
            vregs->regs.rcx = 0;
            vregs->regs.rdx = 0;
            break;       
    }
}

void vinfo_vpartition_info(struct vregs *vregs)
{
    struct vcpu *current = vcurrent_vcpu();

    vregs->regs.rax = 0;
    vregs->regs.rbx = ((current->root ? 1 : 0) << 8) | (current->vid);
    vregs->regs.rcx = current->vpartition_num_cpus;
    vregs->regs.rdx = 0;
}

void vinfo_vcpu_info(struct vregs *vregs)
{
    struct vcpu *current = vcurrent_vcpu();

    vregs->regs.rax = 0;
    vregs->regs.rbx = current->processor_id;
    vregs->regs.rcx = 0;
    vregs->regs.rdx = 0;
}

#endif