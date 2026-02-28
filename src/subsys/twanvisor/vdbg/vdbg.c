#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vdbg/vdbg.h>
#include <subsys/debug/kdbg/kdbg.h>
#include <kernel/kernel.h>
#include <lib/twanprintf.h>

static struct mcslock_isr vdbg_lock;

void vdbg(const char *str)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vdbg_lock, &node);
    __kdbg(str);
    vmcs_unlock_isr_restore(&vdbg_lock, &node);
}

int vdbgf(const char *fmt, ...)
{
    char buf[BUF_SIZE];

    va_list args;
    va_start(args, fmt);
    int len = twan_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    vdbg(buf);
    return len;
}

#endif