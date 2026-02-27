#include <generated/autoconf.h>
#if CONFIG_DEMO_PV_TWANVISOR

#include <kernel/kapi.h>
#include <subsys/mem/pma.h>
#include <lib/libtwanvisor/libvcalls.h>
#include <lib/libtwanvisor/libvc.h>
#include <lib/x86_index.h>
#include <subsys/twanvisor/twanvisor.h>
#include <subsys/mem/vma.h>

extern char pv_twanvisor_demo_guest_start[];
extern char pv_shared_buf_start[64];
extern char _guest_start[];
extern char pv_twanvisor_demo_guest_end[];

#define TERMINATE_ISR_VECTOR 69
#define TERMINATE_ISR_PROCESSOR_ID 0

#define ROOT_TIMESLICE_MS 10
#define GUEST_TIMESLICE_MS 10

static struct vcpu guest_vcpus[NUM_CPUS];

static ept_pml4e_t pml4[512] __aligned(4096);
static ept_pdpte_t pdpt[512] __aligned(4096);
static ept_pde_t pd[512] __aligned(4096);
static ept_pte_t pt[512] __aligned(4096);

static u64 pml4_phys = virt_to_phys_static(pml4);

static struct vpartition guest = {
    .vcpus = guest_vcpus,
    .vcpu_count = NUM_CPUS,

    .terminate_notification = {
        .vector = TERMINATE_ISR_VECTOR,
        .processor_id = TERMINATE_ISR_PROCESSOR_ID
    }
};

static u8 guest_mem[4096] __aligned(4096);

static void guest_init(void)
{
    u32 phys_id = 0;
    u32 ncpus = num_cpus();
    for (u32 i = 0; i < ncpus; i++) {

        struct per_cpu *cpu = cpu_data(i);
        if (!cpu_enabled(cpu->flags))
            continue;

        u32 ticks = ms_to_ticks(GUEST_TIMESLICE_MS, 
                                cpu->vsched_timer_frequency_hz);

        struct vcpu *vcpu = &guest_vcpus[phys_id];

        vcpu->vlaunch.cs.val = 0;
        vcpu->vlaunch.rip = 0;
        
        vcpu->vboot_state = VBOOT_READY;
        vcpu->vsched_metadata.time_slice_ticks = ticks;
        vcpu->vqueue_id = vprocessor_to_vqueue_id(phys_id);

        vcpu->arch.io_bitmap_a_phys = 
            virt_to_phys_static(vcpu->arch.io_bitmap_a);

        vcpu->arch.io_bitmap_b_phys = 
            virt_to_phys_static(vcpu->arch.io_bitmap_b);

        vcpu->arch.msr_bitmap_phys = 
            virt_to_phys_static(vcpu->arch.msr_bitmap);

        vcpu->arch.ve_info_area_phys = 
            virt_to_phys_static(&vcpu->arch.ve_info_area);

        vcpu->arch.vmcs_phys = 
            virt_to_phys_static(&vcpu->arch.vmcs);

        phys_id++;
    }

    u32 num_physical_processors = twan()->num_physical_processors;
    KBUG_ON(phys_id != num_physical_processors);

    guest.vcpu_count = num_physical_processors;
    
    guest.arch.ept_state.fields.ept_base_phys = pml4_phys >> 12;
    guest.arch.ept_state.fields.ept_enabled = 1;
    guest.arch.ept_caching_policy = EPT_WB;

    pml4[0].fields.read = 1;
    pml4[0].fields.write = 1;
    pml4[0].fields.execute = 1;
    pml4[0].fields.pdpt_phys = virt_to_phys_static(pdpt) >> 12;

    pdpt[0].fields.read = 1;
    pdpt[0].fields.write = 1;
    pdpt[0].fields.execute = 1;
    pdpt[0].fields.pd_phys = virt_to_phys_static(pd) >> 12;


    pd[0].fields.read = 1;
    pd[0].fields.write = 1;
    pd[0].fields.execute = 1;
    pd[0].fields.pt_phys = virt_to_phys_static(pt) >> 12;

    pt[0].fields.read = 1;
    pt[0].fields.write = 1;
    pt[0].fields.execute = 1;
    pt[0].fields.memtype = EPT_WB;
    pt[0].fields.pfn = virt_to_phys_static(guest_mem) >> 12;

    size_t size = pv_twanvisor_demo_guest_end - pv_twanvisor_demo_guest_start;
    memcpy(guest_mem, pv_twanvisor_demo_guest_start, size);
}

static int terminate_isr(void)
{
    kdbg("guest terminated!\n");
    return ISR_DONE;
}

static __driver_init void pv_twanvisor_demo_init(void)
{
    if (twan()->flags.fields.twanvisor_on == 0) {
        kdbg("twanvisor not initialized :(\n");
        return;
    }

    guest_init();

    int err = register_isr(TERMINATE_ISR_PROCESSOR_ID, TERMINATE_ISR_VECTOR,
                           terminate_isr);
    if (err < 0) {
        kdbgf("failed to register isr %d\n", err);
        return;
    }

    err = tv_vcreate_partition(&guest);
    if (err < 0) {
        kdbgf("failed to create partition %d\n", err);
        unregister_isr(TERMINATE_ISR_PROCESSOR_ID, TERMINATE_ISR_VECTOR);
        return;
    }

    int vid = err;

    kdbgf("guest created! vid: %d\n", vid);

    u8 root_vid = twan()->flags.fields.vid;

    u32 num_cpus = num_cpus();
    for (u32 i = 0; i < num_cpus; i++) {

        struct per_cpu *cpu = cpu_data(i);
        if (cpu_enabled(cpu->flags)) {

            u32 ticks = ms_to_ticks(ROOT_TIMESLICE_MS,
                                    cpu->vsched_timer_frequency_hz);
                                    
            tv_valter_vcpu_timeslice(root_vid, i, ticks);
        }
    }

    kdbg("root vcpus are now preemptive!\n");

    spin_until(guest_mem[0] != 0);

    char buf[sizeof(pv_shared_buf_start)] = {0};
    memcpy(buf, guest_mem + 1, sizeof(pv_shared_buf_start) - 1);

    kdbgf("%s", buf);

    err = tv_vdestroy_partition(vid);
    if (err < 0)
        kdbgf("failed to destroy partition %d\n", err);
}

REGISTER_DRIVER(pv_twanvisor_demo, pv_twanvisor_demo_init);

#endif