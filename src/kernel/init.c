#include <include/kernel/init.h>
#include <include/kernel/mem/mmu/paging.h>
#include <include/elf.h>
#include <include/multiboot2.h>
#include <include/subsys/debug/kdbg/kdbg.h>
#include <include/kernel/acpi_api/acpi_api.h>
#include <include/kernel/extern.h>
#include <include/errno.h>
#include <include/subsys/sync/spinlock.h>
#include <include/kernel/kernel.h>
#include <include/lib/x86_index.h>
#include <include/kernel/mem/mmu/paging_conf.h>
#include <include/subsys/mem/pma.h>
#include <include/elf.h>

u32 mxcsr_mask64(void)
{
    struct fxsave64 fxsave __aligned(16) = {0};
    __fxsave(&fxsave);
    return fxsave.mxcsr_mask;
}

int init_range(u64 start, size_t size)
{
    if (start < (u64)__kernel_end) {

        u64 new_start = (u64)__kernel_end;
        u64 cutoff = new_start - start;

        if (cutoff >= size)
            return 0;

        size -= cutoff;
        start = new_start;
    }

    /* handle wraparound */

    u64 end = start + size;
    if (end < (u64)__kernel_end) {

        u64 new_end = UINT64_MAX;
        u64 cutoff = new_end - end;

        if (cutoff >= size)
            return 0;

        size -= cutoff;
    }

    return pma_init_range(start, size);
}

int parse_multiboot(u64 multiboot_info_phys)
{
    struct twan_kernel *kernel = twan();

    /* map the multiboot structure in from its physical address ^~_~^ */
    void *mb_info = __map_phys_pg_local(multiboot_info_phys);
    if (!mb_info) {
        __early_kerr("failed to map multiboot\n");
        return -ENOMEM;
    }

    u8 *mb_info_ptr = mb_info;
    u32 mb_info_size = *(u32 *)mb_info_ptr;

    kernel->mem.multiboot_info.start = (u64)multiboot_info_phys;
    kernel->mem.multiboot_info.size = mb_info_size;

    /* check if multiboot fucked up */
    if (mb_info_size > PAGE_SIZE) {
        __early_kerr("multiboot info size is fucked\n");
        return -EFAULT;
    }

    /* parse the multiboot info struct coz we have to */
    bool rsdt_located = false;

    mb_info_ptr += MULTIBOOT_INFO_ALIGN;
    struct multiboot_tag *tag = (void *)mb_info_ptr;

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {

        switch (tag->type) {

            case MULTIBOOT_TAG_TYPE_MMAP:

                struct multiboot_tag_mmap *mmap = (void *)tag;
                void *mmap_entry = &mmap[1];
                u8 *mmap_entry_ptr = mmap_entry;
                u8 *mmap_entries_end = (u8 *)tag + tag->size;

                while (mmap_entry_ptr < mmap_entries_end) {

                    struct multiboot_mmap_entry *entry = (void *)mmap_entry_ptr;
                    u64 start = entry->addr;
                    size_t size = entry->len;

                    if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                        init_range(start, size);

                    mmap_entry_ptr += mmap->entry_size;
                }

                break;
            
            case MULTIBOOT_TAG_TYPE_ACPI_OLD:
                
                struct multiboot_tag_old_acpi *old_acpi = (void *)tag;
                struct acpi_rsdp *rsdp_ptr = (void *)old_acpi->rsdp;

                kernel->acpi.rsdt_phys = rsdp_ptr->rsdt_addr;
                rsdt_located = true;
                break;

            default:
                break;
        }

        mb_info_ptr += p2align_up(tag->size, MULTIBOOT_TAG_ALIGN);
        tag = (void *)mb_info_ptr;
    }

    /* unmap the multiboot info struct */
    __unmap_phys_pg_local(mb_info);

    if (!rsdt_located) {
        __early_kerr("rsdt not in multiboot??\n");
        return -ENOENT;
    }

    return 0;
}

int parse_madt(struct acpi_rsdt *rsdt_ptr, u32 lapic_id, u32 thread_id, 
               u32 core_id, u32 pkg_id)
{
    struct twan_kernel *kernel = twan();

    struct acpi_madt *madt = 
        __early_get_acpi_table(rsdt_ptr, ACPI_MADT_SIGNATURE);

    if (!madt) {
        __early_kerr("failed to find madt\n");
        return -ENOENT;
    }

    int err = 0;

    if (madt->hdr.length > PAGE_SIZE) {
        err = -EFAULT;
        __early_kerr("madt too large\n");
        goto done;
    }

    u8 *madt_entry = (u8 *)madt->entries;
    size_t madt_entries_size = madt->hdr.length - sizeof(struct acpi_madt);
    u8 *madt_entries_end = madt_entry + madt_entries_size;

    u64 lapic_mmio_phys = madt->local_interrupt_controller_address;

    u32 num_ioapics = 0;

    u32 num_cpus = 0;
    u32 num_capable_cpus = 0;
    u32 num_enabled_cpus = 0;
    u32 bsp_index = 0;
    bool madt_bsp_found = false;

    while (madt_entry < madt_entries_end) {

        struct acpi_entry_hdr *entry_hdr = (void *)madt_entry;
        if (madt_entry + entry_hdr->length > madt_entries_end)
            break;
        
        switch (entry_hdr->type) {

            case ACPI_MADT_ENTRY_TYPE_LAPIC:
                
                struct acpi_madt_lapic *lapic_entry = (void *)madt_entry;

                if (num_cpus == NUM_CPUS) {
                    err = -EFAULT;
                    __early_kerr("too many cpus, likely bad config\n");
                    goto done;
                }

                cpu_flags_t flags = {.val = lapic_entry->flags};

                if (cpu_enabled(flags))
                    num_enabled_cpus++;
                    
                else if (cpu_capable(flags)) 
                    num_capable_cpus++;
                
                struct per_cpu *cpu = &kernel->cpu.per_cpu_data[num_cpus];

                cpu->handling_isr = false;
                cpu->intl = -1;

                cpu->current_task = NULL;
                cpu->task_ctx = NULL;

                cpu->this = cpu;
                
                cpu->processor_id = num_cpus;
                cpu->lapic_id = lapic_entry->id;
                cpu->acpi_uid = lapic_entry->uid;
                cpu->flags.val = flags.val;

                if (lapic_id == lapic_entry->id) {

                    cpu->thread_id = thread_id;
                    cpu->core_id = core_id;
                    cpu->pkg_id = pkg_id;

                    cpu->flags.fields.mxcsr_mask = mxcsr_mask64();
                        
                    cpu->flags.fields.active = 1;
                    cpu->flags.fields.bsp = 1;
                    cpu->flags.fields.vmx = is_vmx_supported();

                    bsp_index = num_cpus;
                    madt_bsp_found = true;

                } else {

                    cpu->flags.fields.active = false;
                    cpu->flags.fields.bsp = false;
                }

                cpu->tss.ist1 = (u64)(&cpu->nmi_stack[sizeof(cpu->nmi_stack)]);
                cpu->tss.ist2 = (u64)(&cpu->df_stack[sizeof(cpu->df_stack)]);
                cpu->tss.ist3 = (u64)(&cpu->mce_stack[sizeof(cpu->mce_stack)]);

                cpu->tss.ist4 = 
                    (u64)(&cpu->int_stack_stub[sizeof(cpu->int_stack_stub)]);

                cpu->tr.fields.index = KERNEL_TSS_IDX;

                u64 tss_base = (u64)&cpu->tss;
                u64 tss_limit = sizeof(struct tss64) - 1;

                cpu->descs.descs[KERNEL_CODE_IDX].val = KERNEL_CS_DESC;
                cpu->descs.descs[KERNEL_DATA_IDX].val = KERNEL_DS_DESC;

                struct gdt_descriptor64 *tss_desc = &cpu->descs.tss_desc;

                tss_desc->descriptor_lower.fields.base_low = tss_base & 0xffff;
                
                tss_desc->descriptor_lower.fields.base_mid = 
                    (tss_base >> 16) & 0xff;

                tss_desc->descriptor_lower.fields.base_high =
                     (tss_base >> 24) & 0xff;

                tss_desc->base_upper = tss_base >> 32;

                tss_desc->descriptor_lower.fields.limit_low = 
                    tss_limit & 0xffff;

                tss_desc->descriptor_lower.fields.limit_high = tss_limit >> 16;

                tss_desc->descriptor_lower.fields.present = 1;
                tss_desc->descriptor_lower.fields.segment_type = TSS_AVAILABLE;

                cpu->gdtr.base = (u64)&cpu->descs;
                cpu->gdtr.limit = sizeof(cpu->descs) - 1;

                num_cpus++;
                break;

            case ACPI_MADT_ENTRY_TYPE_IOAPIC:

                if (num_ioapics == NUM_IOAPICS) {
                    err = -EFAULT;
                    __early_kerr("too many ioapics, likely bad config\n");
                    goto done;
                }

                struct acpi_madt_ioapic *ioapic = (void *)madt_entry;
                void *ioapic_mmio_mapped = 
                    __map_phys_pg_io_noflush(ioapic->address);

                if (!ioapic_mmio_mapped) {
                    err = -ENOMEM;
                    __early_kerr("failed to map ioapic mmio\n");
                    goto done;
                }

                kernel->acpi.ioapic[num_ioapics].ioapic_mmio = 
                    ioapic_mmio_mapped;

                kernel->acpi.ioapic[num_ioapics].ioapic_mmio_phys = 
                    ioapic->address;

                kernel->acpi.ioapic[num_ioapics].ioapic_gsi_base = 
                    ioapic->gsi_base;

                num_ioapics++;
                break;

            case ACPI_MADT_ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE:

                struct acpi_madt_lapic_address_override *lapic_override = 
                    (void *)madt_entry;

                lapic_mmio_phys = lapic_override->address;
                break;

            default:
                break;
        }

        madt_entry += entry_hdr->length;
    }

    if (num_cpus == 0 || !madt_bsp_found) {
        err = -ENOENT;
        __early_kerr("madt holds fucked up info\n");
        goto done;
    }

    void *lapic_mmio = __map_phys_pg_io_noflush(lapic_mmio_phys);
    if (!lapic_mmio) {
        err = -ENOMEM;
        __early_kerr("failed to map lapic mmio\n");
        goto done;
    }

    kernel->acpi.lapic_mmio = lapic_mmio;
    kernel->acpi.lapic_mmio_phys = lapic_mmio_phys;

    kernel->acpi.num_ioapics = num_ioapics;
    
    /* store cpu info */
    kernel->cpu.num_cpus = num_cpus;
    kernel->cpu.num_capable_cpus = num_capable_cpus;
    kernel->cpu.num_enabled_cpus = num_enabled_cpus;
    kernel->cpu.bsp = bsp_index;

    for (u32 i = 0; i < ARRAY_LEN(kernel->ipi_table); i++) {

        for (u32 j = 0; j < ARRAY_LEN(kernel->ipi_table[j]); j++)
            atomic32_set(&kernel->ipi_table[i][j].signal, IPI_UNLOCKED);
    }

done:
    __early_put_acpi_table(madt);
    return err;
}

int parse_acpi_tables(u32 lapic_id, u32 thread_id, u32 core_id, u32 pkg_id)
{
    u64 rsdt_phys = twan()->acpi.rsdt_phys;
    struct acpi_rsdt *rsdt_ptr = __map_phys_pg_local(rsdt_phys);

    if (!rsdt_ptr) {
        __early_kerr("failed to map rsdt\n");
        return -ENOMEM;
    }

    int ret = parse_madt(rsdt_ptr, lapic_id, thread_id, core_id, pkg_id);

    __unmap_phys_pg_local(rsdt_ptr);
    return ret;
}

int gdt_init_local(void)
{
    struct per_cpu *this_cpu = this_cpu_data();
    __lgdt(&this_cpu->gdtr);
    __ltr(this_cpu->tr.val);
    return 0;
}

int idt_init(void)
{
    for (u32 i = 0; i < ARRAY_LEN(isr_entry_table); i++) {
        u32 ist_entry;
        switch (i) {

            case NMI:
                ist_entry = IST_NMI;
                break;

            case DOUBLE_FAULT:
                ist_entry = IST_DF;
                break;

            case MACHINE_CHECK:
                ist_entry = IST_MCE;
                break;

            default:
                ist_entry = IST_NORMAL;
                break;
        }

        idt[i].ist = ist_entry;
        idt[i].attr.fields.gate_type = INTERRUPT_GATE64;
        idt[i].attr.fields.present = 1;
        idt[i].selector.val = KERNEL_CODE_SELECTOR;
        idt[i].offset_low = isr_entry_table[i] & 0xffff;
        idt[i].offset_mid = (isr_entry_table[i] >> 16) & 0xffff;
        idt[i].offset_high = isr_entry_table[i] >> 32;
    }

    return 0;
}