#ifndef _TLB_H_
#define _TLB_H_

#include <include/std.h>
#include <include/arch.h>

struct tlb_range
{
    u64 first;
    u64 last;
};  

void flush_tlb_local(void);
void flush_tlb_page_local(u64 addr);
void flush_tlb_range_local(u64 first, u64 last);

void flush_tlb_ipi(__unused u64 arg);
void flush_tlb_page_ipi(u64 addr);
void flush_tlb_range_ipi(u64 arg);

void flush_tlb_global(bool wait);
void flush_tlb_page_global(u64 addr, bool wait);
void flush_tlb_range_global(u64 first, u64 last);

#endif