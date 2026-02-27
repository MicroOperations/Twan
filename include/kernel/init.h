#ifndef _INIT_H_
#define _INIT_H_

#include <kernel/kernel.h>

u32 mxcsr_mask64(void);

int init_range(u64 start, size_t size);

int  parse_multiboot(u64 multiboot_info_phys);
int parse_madt(struct acpi_rsdt *rsdt_ptr, u32 lapic_id, u32 thread_id, 
              u32 core_id, u32 pkg_id);

int parse_acpi_tables(u32 lapic_id, u32 thread_id, u32 core_id, u32 pkg_id);

int gdt_init_local(void);
int idt_init(void);

#endif