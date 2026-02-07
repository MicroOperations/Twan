#ifndef _VMA_H_
#define _VMA_H_

#include <include/subsys/debug/kdbg//kdbg.h>
#include <include/std.h>
#include <include/lib/x86_index.h>

#define VMA_PARTITION_REGISTRY ".vma_partition_registry"

#define __vma_partition_registry \
    __attribute__ ((section(VMA_PARTITION_REGISTRY), used, aligned(1)))

struct vma_descriptor
{
    u64 addr;
    size_t size;
    u32 num_partitions_valid;
};

struct vma_partition
{ 
    pde_huge_t pde[512];
} __packed;
SIZE_ASSERT(struct vma_partition, 4096);

struct vma_partition_ticket
{
    const char *name;
    u64 table_phys;
    u32 count;
    struct vma_descriptor *descriptor;
};

extern struct vma_partition_ticket __vma_partition_registry_start[];
extern struct vma_partition_ticket __vma_partition_registry_end[];

#define num_registered_vma_partitions()        \
    (((u64)__vma_partition_registry_end -      \
     (u64)__vma_partition_registry_start) /    \
     sizeof(struct vma_partition_ticket))

#define DEFINE_VMA_PARTITION_TABLE(name, count) \
    struct vma_partition (name)[(count)] __aligned(4096)

#define REGISTER_VMA_PARTITION_TABLE(_name, _table_phys, _count,               \
                                     descriptor_ptr)                           \
    __vma_partition_registry                                                   \
    static const struct vma_partition_ticket vma_partition_ticket_##_name = {  \
        .name = (#_name),                                                      \
        .table_phys = (_table_phys),                                           \
        .count = (_count),                                                     \
        .descriptor = (descriptor_ptr),                                        \
    }

#define DEFINE_VMA_PARTITION(name) \
    struct vma_partition (name) __aligned(4096)

#define REGISTER_VMA_PARTITION(_name, _partition_phys, descriptor_ptr) \
    REGISTER_VMA_PARTITION_TABLE(_name, (_partition_phys), 1,        \
                                 (descriptor_ptr))

#define virt_to_phys_static(addr) ((u64)addr)

#define VMA_PARTITION_SIZE PAGE_SIZE_1GB

#define VMA_PARTITION_ENTRY_COUNT 512
#define VMA_PARTITION_TABLE_COUNT(table) (ARRAY_LEN(table))

#define vma_partition_table_get(table, idx) (&(table)[(idx)])
#define vma_pa_to_pfn(pa) ((pa) >> PAGE_SHIFT)

#define vma_partition_unmap(partition, idx) do {        \
                                                        \
    KBUG_ON(idx >= VMA_PARTITION_ENTRY_COUNT);          \
    (partition)->pde[idx].val = 0;                      \
} while (0)

#define vma_partition_remap(partition, idx, _pfn) do {  \
                                                        \
    KBUG_ON(idx >= VMA_PARTITION_ENTRY_COUNT);          \
                                                        \
    (partition)->pde[idx].val = 0;                      \
    (partition)->pde[idx].fields.rw = 1;                \
    (partition)->pde[idx].fields.present = 1;           \
    (partition)->pde[idx].fields.ps = 1;                \
    (partition)->pde[idx].fields.pfn = (_pfn);          \
} while (0)

#define vma_partition_remap_mmio(partition, idx, _pfn) do { \
                                                            \
    KBUG_ON(idx >= VMA_PARTITION_ENTRY_COUNT);              \
                                                            \
    (partition)->pde[idx].val = 0;                          \
    (partition)->pde[idx].fields.rw = 1;                    \
    (partition)->pde[idx].fields.present = 1;               \
    (partition)->pde[idx].fields.pwt = 1;                   \
    (partition)->pde[idx].fields.pcd = 1;                   \
    (partition)->pde[idx].fields.ps = 1;                    \
    (partition)->pde[idx].fields.pfn = (_pfn);              \
} while (0)

struct vma_interface
{
};

struct vma
{
    struct vma_interface *interface;
    bool initialized;
};

int vma_init(struct vma_interface *interface);

bool is_vma_initialized(void);

#endif