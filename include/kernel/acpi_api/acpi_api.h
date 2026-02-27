#ifndef _ACPI_API_H_
#define _ACPI_API_H_

#include <uacpi/acpi.h>
#include <kernel/kernel.h>

void *__early_get_acpi_table(struct acpi_rsdt *rsdt_ptr, const char *sig);

void *__early_get_acpi_table_kernel(struct twan_kernel *kernel, 
                                    const char *sig);

void __early_put_acpi_table(void *table);

#endif