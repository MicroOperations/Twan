#ifndef _TWANVISOR_H_
#define _TWANVISOR_H_

#include <include/subsys/twanvisor/vsched/vsync.h>
#include <include/subsys/twanvisor/vsched/vsched.h>
#include <include/subsys/twanvisor/vsched/vsched_yield.h>
#include <include/subsys/twanvisor/vdbg/vdbg.h>
#include <include/kernel/kernel.h>
#include <include/lib/dsa/bmp512.h>

#define VROOT_PARTITION_CRITICALITY VSCHED_MAX_CRITICALITY
#define VSCHED_LAPIC_DCR DIV_16

#define VINT_STACK_SIZE 2048
#define VSAFE_STACK_SIZE 2048

#define VKERNEL_CS_IDX 1
#define VKERNEL_DS_IDX 2
#define VKERNEL_TSS_IDX 3

#define VKERNEL_CS (VKERNEL_CS_IDX << 3)
#define VKERNEL_DS (VKERNEL_DS_IDX << 3)
#define VKERNEL_TR (VKERNEL_TSS_IDX << 3)

#define VKERNEL_CS_DESC 0x00af9a000000ffff
#define VKERNEL_DS_DESC 0x00af92000000ffff

#define VIST_NMI 1
#define VIST_DF 2
#define VIST_MCE 3
#define VIST_NORMAL 4

struct vipi_call
{
    ipi_func_t func;
    u64 arg;
};

struct vipi_data
{
    bool is_self_ipi;
    bool dead;
    struct vipi_call self_ipi;

    struct 
    {
        struct dq vcpu_sender_dq;
        struct mcslock_isr lock;

        /* drain can be used without lock */
        u8 drain;
    } vcpus;
};

struct vper_cpu_arch
{
    struct vmxon_region vmxon;
} __packed;

SIZE_ASSERT(struct vper_cpu_arch, sizeof(struct vmxon_region));

typedef union 
{
    u32 val;
    struct 
    {
        u32 procbased_ctls2 : 1;
        u32 exit_ctls2 : 1;
        u32 wbinvd_exiting : 1;
        u32 unrestricted_guest : 1;
        u32 ept : 1;
        u32 vpid : 1;
        u32 ept_ve : 1;
        u32 ept_pml5 : 1;
        u32 ept_pml4 : 1;
        u32 ept_2mb : 1;
        u32 ept_1gb : 1;
        u32 ept_wb : 1;
        u32 ept_uc : 1;
        u32 ept_accessed_dirty : 1;
        u32 invvpid_single : 1;
        u32 pt_use_gpa : 1;
        u32 reserved0 : 16;
    } fields;
} vper_cpu_arch_support_t;

struct vper_cpu_arch_flags
{
    u64 tsc_period_fs;
    u64 tsc_frequency_hz;

    u64 vmx_preempt_period_fs;
    u64 vmx_preempt_frequency_hz;

    u64 lapic_period_fs;
    u64 lapic_frequency_hz;
    vper_cpu_arch_support_t support;

    u32 revision_id;
    u32 ia32_vmx_pinbased_ctls;
    u32 ia32_vmx_procbased_ctls;
    u32 ia32_vmx_exit_ctls;
    u32 ia32_vmx_entry_ctls;
};

struct vshield
{
    u64 recovery_rip;
    union 
    {
        u32 val;
        struct
        {
            u32 exception_vector : 8;
            u32 len : 8;
            u32 recover_by_length : 1;
            u32 faulted : 1;
            u32 active : 1;
            u32 reserved0 : 13;
        } fields;
    } ext;
};

typedef union 
{
    u64 val;
    struct 
    {
        u64 its_no : 1;
        u64 ibpb : 1;
        u64 should_flush_l1d_vmentry : 1;
        u64 verw_clears_fb: 1;
        u64 sr_bios_done : 1;
        u64 virtual_enum : 1;
        u64 prmrr_activated : 1;
        u64 reserved0 : 56;

        /* sticky bit set when the sched timer is armed, unset when the sched
           timer fires, should be touched only with interrupts disabled to 
           prevent races */
        u64 sched_timer_armed : 1;
    } fields;

} vper_cpu_sec_flags_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 mce : 1;
        u32 umip : 1;
        u32 la57 : 1;
        u32 smx : 1;
        u32 fsgsbase : 1;
        u32 pcid : 1;
        u32 xsave : 1;
        u32 kl : 1;
        u32 smep : 1;
        u32 smap : 1;
        u32 pke : 1;
        u32 cet : 1;
        u32 pks : 1;
        u32 uintr : 1;
        u32 lass : 1;
        u32 lam : 1;
        u32 fred : 1;
        u32 reserved0 : 15;
    } fields;
} vper_cpu_feature_flags_t;

typedef union
{
    u64 val;
    struct 
    {
        u64 ia32_arch_capabilities_r : 1;
        u64 ia32_feature_control_rw : 1;
        u64 user_msr : 1;
        u64 uintr_timer : 1;
        u64 waitpkg : 1;
        u64 xapic_disable_status : 1;
        u32 tsc_aux : 1;
        u64 reserved0 : 52;
        u64 lint0 : 1;
        u64 lint1 : 1;
        u64 cmci : 1;
        u64 tsr : 1;
        u64 pmcr : 1;
    } fields;
} vper_cpu_trap_cache_t;

struct vper_cpu_cache
{
    vper_cpu_trap_cache_t trap_cache;

    u32 via32_feature_ctrl_low;
    u32 via32_feature_ctrl_high;

    u32 msr_area_max;
};

struct vper_cpu 
{
    struct vper_cpu_arch arch __aligned(4096);

    struct bmp256 available_vectors;

    bool handling_isr;
    struct interrupt_info *vcpu_ctx;
    
    struct vper_cpu *this;
    struct vper_cpu_arch_flags arch_flags;
    vper_cpu_sec_flags_t sec_flags;
    vper_cpu_feature_flags_t feature_flags;

    struct vper_cpu_cache vcache;

    char int_stack[VINT_STACK_SIZE] __aligned(16);
    char nmi_stack[VINT_STACK_SIZE] __aligned(16);
    char df_stack[VINT_STACK_SIZE] __aligned(16);
    char mce_stack[VINT_STACK_SIZE] __aligned(16);

    struct vcpu *current_vcpu;

    u32 processor_id;
    u32 vprocessor_id;
    u32 lapic_id;
    u32 acpi_uid;

    u32 thread_id;
    u32 core_id;
    u32 pkg_id;

    bool bsp;

    struct dq dispatch_vectors[NUM_VECTORS];
    struct mcslock_isr dispatch_lock;

    struct vipi_data vipi_data;
    struct vscheduler vscheduler;

    struct tss64 tss;
    selector_t tr;
    struct 
    {
        gdt_descriptor32_t descs[3];
        struct gdt_descriptor64 tss_desc;
    } descs __packed;

    struct descriptor_table64 gdtr;

    struct vshield shield;
};

struct vtwan_kernel
{
    struct vtwan_kernel *this;
    void *lapic_mmio;

    struct vper_cpu per_cpu_data[NUM_CPUS];

    u32 num_enabled_cpus;
    u32 bsp;

    u8 root_vid;
    u32 root_num_vcpus; 

    struct idt_descriptor64 idt[NUM_VECTORS];
    struct descriptor_table64 idtr;
};

#define vtwan() \
    ((struct vtwan_kernel *)__readfs64(offsetof(struct vtwan_kernel, this)))

#define vnum_cpus() \
    (__readfs32(offsetof(struct vtwan_kernel, num_enabled_cpus)))

#define vper_cpu_data(vprocessor_id) \
    (&vtwan()->per_cpu_data[(vprocessor_id)])

#define vthis_cpu_data() \
    ((struct vper_cpu *)__readgs64(offsetof(struct vper_cpu, this)))

#define vthis_vprocessor_id() \
    (__readgs32(offsetof(struct vper_cpu, vprocessor_id)))

#define vthis_vscheduler() \
    (&vthis_cpu_data()->vscheduler)

#define vcurrent_vcpu() \
    (vthis_cpu_data()->current_vcpu)

#define vscheduler_of(vcpu) \
    (&vper_cpu_data(vqueue_to_vprocessor_id((vcpu)->vqueue_id))->vscheduler)

inline bool vcurrent_vcpu_is_preemption_enabled(void)
{
    struct vcpu *current = vcurrent_vcpu();
    VBUG_ON(!current);

    return current->preemption_count == 0;
}

inline void vcurrent_vcpu_disable_preemption(void)
{
    struct vcpu *current = vcurrent_vcpu();
    VBUG_ON(!current);
    VBUG_ON(current->preemption_count == UINT32_MAX);

    current->preemption_count++;
}

inline void vcurrent_vcpu_enable_preemption_no_yield(void)
{
    struct vcpu *current = vcurrent_vcpu();
    VBUG_ON(!current);
    VBUG_ON(current->preemption_count == 0);

    current->preemption_count--;
}

inline bool vtry_answer_yield_request(void)
{
    struct vcpu *current = vcurrent_vcpu();

    u64 flags = read_flags_and_disable_interrupts();

    bool ret = current->preemption_count == 0 && 
               current->flags.fields.yield_request == 1;

    if (ret)
        vsched_yield();

    write_flags(flags);
    return ret;
}

inline void vcurrent_vcpu_enable_preemption(void)
{
    vcurrent_vcpu_enable_preemption_no_yield();
    vtry_answer_yield_request();
}

#endif