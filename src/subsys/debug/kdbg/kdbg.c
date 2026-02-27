#include <subsys/debug/kdbg/kdbg.h>
#include <errno.h>
#include <kernel/kapi.h>
#include <lib/twanprintf.h>
#include <lib/libtwanvisor/libvcalls.h>

static struct kdbg kdbg_global;

int kdbg_init(struct kdbg_interface *interface) 
{
    if (!interface)
        return -EINVAL;

    if (kdbg_global.initialized)
        return -EALREADY;

    kdbg_global.interface = interface;
    mcslock_isr_init(&kdbg_global.lock);
    kdbg_global.initialized = true;

    return 0;
}

bool is_kdbg_initialized(void)
{
    return kdbg_global.initialized;
}

void __kdbg(const char *str)
{
    if (kdbg_global.initialized && kdbg_global.interface && 
        kdbg_global.interface->__kdbg_func) {

        INDIRECT_BRANCH_SAFE(kdbg_global.interface->__kdbg_func(str));
    }
}

void kdbg(const char *str)
{
#if TWANVISOR_ON

    if (twan()->flags.fields.twanvisor_on != 0) {
        tv_vkdbg(str);
        return;
    }

#endif

    if (kdbg_global.initialized) {

        struct mcsnode node = INITIALIZE_MCSNODE();

        mcs_lock_isr_save(&kdbg_global.lock, &node);
        __kdbg(str);
        mcs_unlock_isr_restore(&kdbg_global.lock, &node);
    }
}

int __kdbgf(const char *fmt, ...)
{
    char buf[BUF_SIZE];

    va_list args;
    va_start(args, fmt);
    int len = twan_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    __kdbg(buf);
    return len;
}

int kdbgf(const char *fmt, ...)
{
    char buf[BUF_SIZE];

    va_list args;
    va_start(args, fmt);
    int len = twan_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    kdbg(buf);
    return len;
}