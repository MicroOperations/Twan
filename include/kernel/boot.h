#ifndef _BOOT_H_
#define _BOOT_H_

#include <generated/autoconf.h>

#define NUM_CPUS CONFIG_NUM_CPUS

#define KERNEL_START_ADDR (void *)(0x100000)

#define BOOT_STACK_SIZE 2048

#define PDPT_PML4_IDX 0ULL
#define PD_KERNEL_PDPT_IDX 0ULL
#define PD_HEAP_PDPT_IDX 1ULL

#define PDPT_FREE_START_PML4_IDX 1ULL

#define KERNEL_CODE_IDX 1
#define KERNEL_CODE_SELECTOR (KERNEL_CODE_IDX << 3)

#define KERNEL_DATA_IDX 2
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_IDX << 3)

#define PE_MASK (1 << 0)
#define MP_MASK  (1 << 1)
#define EM_MASK (1 << 2)
#define NE_MASK (1 << 5)
#define WP_MASK  (1 << 16)
#define NW_MASK (1 << 29)
#define CD_MASK (1 << 30)

#define DE_MASK (1 << 3)
#define PAE_MASK (1 << 5)
#define OSFXSR_MASK (1 << 9)
#define OSXMMEXCPT_MASK (1 << 10)
#define FSGSBASE_MASK (1 << 16)
#define PG_MASK  (1 << 31)

#define IA32_EFER 0xC0000080
#define LME_MASK (1 << 8)

#define WAKEUP_VECTOR 7ULL
#define WAKEUP_ADDR (WAKEUP_VECTOR << 12)

#define KERNEL_CS_DESC 0x00af9b000000ffff
#define KERNEL_DS_DESC 0x00af93000000ffff

#ifndef ASM_FILE

#include <compiler.h>

STATIC_ASSERT(NUM_CPUS > 0);

#endif

#endif