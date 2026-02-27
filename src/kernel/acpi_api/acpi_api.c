#include <kernel/acpi_api/acpi_api.h>
#include <kernel/mem/mmu/paging.h>

void *__early_get_acpi_table(struct acpi_rsdt *rsdt_ptr, const char *sig)
{
    u32 no_entries =
        ((rsdt_ptr->hdr.length - sizeof(struct acpi_sdt_hdr)) / 4);

    for (u32 i = 0; i < no_entries; i++) {
        
        u32 hdr_phys = rsdt_ptr->entries[i];
        struct acpi_sdt_hdr *hdr = __map_phys_pg_local(hdr_phys);

        if (strncmp(hdr->signature, sig, 4) == 0)
            return hdr;        

        __unmap_phys_pg_local(hdr);
    }

    return NULL;
}

void *__early_get_acpi_table_kernel(struct twan_kernel *kernel, 
                                    const char *sig)
{
    struct acpi_rsdt *rsdt_ptr = __map_phys_pg_local(kernel->acpi.rsdt_phys);
    if (!rsdt_ptr)
        return NULL;

    void *addr = __early_get_acpi_table(rsdt_ptr, sig);
    __unmap_phys_pg_local(rsdt_ptr);

    return addr;
}

void __early_put_acpi_table(void *table)
{
    __unmap_phys_pg_local(table);
}