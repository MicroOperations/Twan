#include <kernel/static.h>
#include <kernel/kernel.h>
#include <lib/x86_index.h>

static struct twan_kernel kernel = {
    .this = &kernel,
};

void twan_kernel_init_local(void)
{
    __wrmsrl(IA32_FS_BASE, (u64)&kernel);
}