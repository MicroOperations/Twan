#include <include/initcalls/early_initcalls_conf.h>
#if EARLY_MEM_VMA_PARSER

#include <include/kernel/mem/mmu/paging.h>
#include <include/kernel/kapi.h>
#include <include/subsys/mem/vma.h>
#include <include/subsys/sync//mcslock.h>
#include <include/lib/x86_index.h>
#include <include/kernel/boot.h>
#include <include/kernel/extern.h>
#include <include/generated/autoconf.h>

#define NUM_PDPTS (CONFIG_KERNEL_MAX_VMA_PARTITIONS / 512)
STATIC_ASSERT(NUM_PDPTS > 0);

#define MAX_VMA_PARTITIONS (NUM_PDPTS * 512)

struct pdpt
{
    pdpte_t pdptes[512];
};
SIZE_ASSERT(struct pdpt, 4096);

static struct pdpt intm_tables[NUM_PDPTS] __aligned(4096);
STATIC_ASSERT(ARRAY_LEN(intm_tables) <= (512 - PDPT_FREE_START_PML4_IDX));

static void vma_fill_descriptor(struct vma_descriptor *desc, u32 base_idx, 
                                u32 idx)
{
    u32 table_idx = base_idx / 512;
    u32 pdpt_idx = base_idx % 512;
    u32 pml4_idx = table_idx + PDPT_FREE_START_PML4_IDX;

    u32 num_partitions = (idx - base_idx);
    size_t size = num_partitions * PAGE_SIZE_2MB;

    va_t va = {
        .fields = {
            .pdpt = pdpt_idx,
            .pml4 = pml4_idx,
            .sign = pml4_idx >= 256 ? 0xffff : 0,
        }
    };

    desc->addr = va.val;
    desc->size = size;
    desc->num_partitions_valid = num_partitions;
}

static struct vma_interface vma_interface = {

};

static __early_initcall void vma_parser_init(void)
{
    if (vma_init(&vma_interface) < 0)
        return;

    for (u32 i = 0; i < ARRAY_LEN(intm_tables); i++) {
        u32 j = PDPT_FREE_START_PML4_IDX + i;
        pml4[j].val = 0;
        pml4[j].fields.rw = 1;
        pml4[j].fields.present = 1;
        pml4[j].fields.pdpt_phys = virt_to_phys_static(&intm_tables[i]) >> 12;
    }

    u32 intm_idx = 0;
    for (u32 i = 0; i < num_registered_vma_partitions(); i++) {

        struct vma_partition_ticket *ticket = 
            &__vma_partition_registry_start[i];

        struct vma_descriptor *desc = ticket->descriptor;

        u32 count = ticket->count;
        u32 base_idx = intm_idx;

        for (u32 j = 0; j < count; j++) {

            if (intm_idx == MAX_VMA_PARTITIONS) {

                if (j != 0)
                    vma_fill_descriptor(desc, base_idx, intm_idx);

                return;
            }

            u32 table_idx = intm_idx / 512;
            u32 pdpt_idx = intm_idx % 512;
        
            pdpte_t *pdpte = &intm_tables[table_idx].pdptes[pdpt_idx];

            pdpte->val = 0;
            pdpte->fields.rw = 1;
            pdpte->fields.present = 1;
            pdpte->fields.pd_phys = (ticket->table_phys + (j * 4096)) >> 12;

            intm_idx++;
        }

        vma_fill_descriptor(desc, base_idx, intm_idx);
    }
}

REGISTER_EARLY_INITCALL(vma_parser_init, vma_parser_init, 
                        EARLY_MEM_VMA_PARSER_ORDER);

#endif