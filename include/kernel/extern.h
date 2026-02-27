#ifndef _EXTERN_H_
#define _EXTERN_H_

#include <kernel/isr/isr_index.h>
#include <lib/x86_index.h>

extern char __bss_start[];
extern char __bss_end[];

extern char __kernel_start[];
extern char __kernel_end[];

extern struct descriptor_table64 gdtr64;
extern struct descriptor_table64 idtr;

/* kernel page tables */
extern pml4e_t pml4[512];

/* kernel image */
extern pdpte_t pdpt[512]; // first mapped pdpt
extern pde_huge_t pd_kernel[512]; // kernel 
extern pte_t pt_kernel_start[512]; // kernel start with 4KB hole

extern struct idt_descriptor64 idt[256];

/* ap data */
extern char __wakeup[];
extern char ap_rsp[8]; // set this to a stack ptr for the ap
extern char __wakeup_end[];

extern u64 __wakeup_blob_size;
extern char __wakeup_blob_save_area[];

/* isr entry point addresses */
extern u64 isr_entry_table[NUM_VECTORS];

#endif