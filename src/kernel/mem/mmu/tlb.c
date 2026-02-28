#include <kernel/mem/mmu/tlb.h>
#include <kernel/kapi.h>
#include <lib/libtwanvisor/libvcalls.h>

void flush_tlb_local(void)
{
    flush_this_tlb();
}

void flush_tlb_page_local(u64 addr)
{
    flush_this_tlb_page(addr);
}

void flush_tlb_range_local(u64 first, u64 last)
{
    flush_this_tlb_range(first, last);
}

void flush_tlb_ipi(__unused u64 arg)
{
    enable_interrupts();
    flush_tlb_local();
}

void flush_tlb_page_ipi(u64 addr)
{
    enable_interrupts();
    flush_this_tlb_page(addr);
}

void flush_tlb_range_ipi(u64 arg)
{
    enable_interrupts();
    
    struct tlb_range *range = (void *)arg;
    flush_this_tlb_range(range->first, range->last);
}

void flush_tlb_global(bool wait)
{
    struct twan_kernel *kernel = twan();

#if CONFIG_SUBSYS_TWANVISOR
    
    if (kernel->flags.fields.twanvisor_on != 0) {

        u8 vid = kernel->flags.fields.vid;
        tv_vtlb_shootdown(vid);
        return;
    }

#endif

    flush_this_tlb();

    if (kernel->flags.fields.multicore_initialized != 0)
        ipi_run_func_others(flush_tlb_ipi, 0, wait);
}

void flush_tlb_page_global(u64 addr, bool wait)
{
    flush_this_tlb_page(addr);

    if (twan()->flags.fields.multicore_initialized != 0)
        ipi_run_func_others(flush_tlb_page_ipi, addr, wait);
}

void flush_tlb_range_global(u64 first, u64 last)
{
    flush_this_tlb_range(first, last);

    if (twan()->flags.fields.multicore_initialized != 0) {

        struct tlb_range range = {
            .first = first,
            .last = last
        };

        ipi_run_func_others(flush_tlb_range_ipi, (u64)&range, true);
    }
}
