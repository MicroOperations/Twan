#include <subsys/mem/vma.h>
#include <kernel/static.h>
#include <lib/x86_index.h>
#include <subsys/debug/kdbg/kdbg.h>
#include <kernel/kernel.h>
#include <kernel/apic/apic.h>
#include <multiboot2.h>
#include <kernel/mem/mmu/paging.h>
#include <kernel/extern.h>
#include <elf.h>
#include <kernel/acpi_api/acpi_api.h>
#include <kernel/isr/isr_index.h>
#include <kernel/init.h>
#include <kernel/kapi.h>
#include <errno.h>
#include <kernel/sched/sched.h>
#include <subsys/time/counter.h>
#include <subsys/time/sleep.h>
#include <subsys/time/timeout.h>
#include <kernel/isr/base_isrs.h>

static __noreturn void __setup_ap(u32 lapic_id, u32 thread_id,
                                  u32 core_id, u32 pkg_id)
{
    struct twan_kernel *kernel = twan();

    struct per_cpu *per_cpu_data = kernel->cpu.per_cpu_data;
    u32 num_cpus = kernel->cpu.num_cpus;
    
    for (u32 i = 0; i < num_cpus; i++) {

        if (per_cpu_data[i].lapic_id == lapic_id) {

            struct per_cpu *cpu = &per_cpu_data[i];

            /* fill in our topology data */
            cpu->thread_id = thread_id;
            cpu->core_id = core_id;
            cpu->pkg_id = pkg_id;
    
            cpu->flags.fields.mxcsr_mask = mxcsr_mask64();    
            cpu->flags.fields.vmx = is_vmx_supported();

            /* setup our tr and gs */
            __wrmsrl(IA32_GS_BASE, (u64)cpu);

            gdt_init_local();
            register_base_isrs_local();

            /* configure the lapic */
            lapic_sync();
            disable_lapic();
            mask_lapic_lint();
            enable_lapic(SPURIOUS_INT_VECTOR);
            __config_lapic_nmis();

            /* we're initialized yayy !! */
            enable_interrupts();
            cpu->flags.fields.active = true;
            break;
        }
    }
    
    halt_loop();

    UNREACHABLE();
}

__noreturn void __start_twan(u64 multiboot_info_phys, bool is_bsp)
{        
    twan_kernel_init_local();

    u32 pkg_id;
    u32 core_id;
    u32 thread_id;
    u32 lapic_id;
    this_topology(&lapic_id, &thread_id, &core_id, &pkg_id);

    if (!is_bsp)
        __setup_ap(lapic_id, thread_id, core_id, pkg_id);

    u32 num_early_initcalls = num_registered_early_initcalls();
    for (u32 i = 0; i < num_early_initcalls; i++) 
        __early_initcall_registry_start[i].init();

    if (!is_vma_initialized())
        __early_kpanic("vma not initialized during early boot\n");

    __kdbg("twan entered\n");

    /* initialize our mapper routines */
    init_mappers();

    /* parse multiboot info coz have to */
    if (parse_multiboot(multiboot_info_phys) < 0)
        __early_kpanic("couldn't parse multiboot\n");

    /* now we gotta parse the relevant acpi tables */
    if (parse_acpi_tables(lapic_id, thread_id, core_id, pkg_id) < 0)
        __early_kpanic("failed to parse acpi tables\n");

    struct twan_kernel *kernel = twan();

    u32 bsp_index = kernel->cpu.bsp;
    __wrmsrl(IA32_GS_BASE, (u64)&kernel->cpu.per_cpu_data[bsp_index]);
    register_base_isrs_local();

    /* initialize ipi data and the gdt & idt */
    if (gdt_init_local() < 0)
        __early_kpanic("failed to initialize gdt\n");

    if (idt_init() < 0) 
        __early_kpanic("failed to initialize idt\n");

    /* do any pre wakeup global initialisation */
    lapic_sync();
    remap_mask_8259pic();

    /* pre wakeup local initialisation */
    disable_lapic();
    mask_lapic_lint();
    enable_lapic(SPURIOUS_INT_VECTOR);

    /* setup our wakeup blob */
    u8 *ap_wakeup_base = (void *)WAKEUP_ADDR;

    memcpy(__wakeup_blob_save_area, ap_wakeup_base, __wakeup_blob_size);
    memcpy(ap_wakeup_base, __wakeup, __wakeup_blob_size);

    u64 ap_rsp_offset = (u64)ap_rsp - (u64)__wakeup;
    u64 *ap_rsp_reloc = (void *)(ap_wakeup_base + ap_rsp_offset);

    u32 num_cpus = kernel->cpu.num_cpus;
    for (u32 i = 0; i < num_cpus; i++) {

        if (i == bsp_index)
            continue;

        struct per_cpu *cpu = cpu_data(i);
        volatile cpu_flags_t *flags = &cpu->flags;

        if (!cpu_enabled(*flags))
            continue;

        *ap_rsp_reloc = (u64)&cpu->wakeup_stack[sizeof(cpu->wakeup_stack)];

        /* wake it up */
        u32 ap_lapic_id = lapic_id_of(i);
        lapic_wakeup_ap(ap_lapic_id, WAKEUP_VECTOR);
        spin_until(cpu_active(*flags));
    }

   /* stuff will only race once the scheduler is initialized since ap's 
      will halt after setting their active flag */
    kernel->flags.fields.multicore_initialized = 1;

    /* cleanup from our wakeup blob */
    memcpy(ap_wakeup_base, __wakeup_blob_save_area, __wakeup_blob_size);

    /* post wakeup local initialisation */
    __config_lapic_nmis();

    /* do any post wakeup global initialisation */
    if (__ioapic_config() < 0)
        __early_kpanic("failed to configure ioapic\n");

    kernel->flags.fields.bsp_initialized = 1;
    enable_interrupts();

    u32 num_late_initcalls = num_registered_late_initcalls();
    for (u32 i = 0; i < num_late_initcalls; i++) 
        INDIRECT_BRANCH_SAFE(__late_initcall_registry_start[i].init());

    kdbgf("kernel size: %lu bytes\n", __kernel_end - __kernel_start);

    /* a counter, sleep and timeout interfaces are required beyond this point */
    if (!is_counter_initialized())
        kpanicf_global("counter not initialized pre scheduler init\n");

#if CONFIG_SUBSYS_SLEEP

    if (!is_sleep_initialized())
        kpanicf_global("sleep not initialized pre scheduler init\n");

#endif

#if CONFIG_SUBSYS_TIMEOUT

    if (!is_timeout_initialized())
        kpanicf_global("timeout not initialized pre scheduler init\n");

#endif

    /* initialize the scheduler, early boot shit is done */
    scheduler_init();

    UNREACHABLE();
}