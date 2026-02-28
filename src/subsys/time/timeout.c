#include <subsys/time/timeout.h>
#if CONFIG_SUBSYS_TIMEOUT

#include <subsys/debug/kdbg/kdbg.h>

static struct timeout timeout_global;

int timeout_init(struct timeout_interface *interface)
{
    if (!interface)
        return -EINVAL;

    if (timeout_global.initialized)
        return -EALREADY;

    timeout_global.interface = interface;
    timeout_global.initialized = true;
    
    return 0;
}

bool is_timeout_initialized(void)
{
    return timeout_global.initialized;
}

void timeout_lock(struct mcsnode *node)
{
    if (timeout_global.initialized) {

        KBUG_ON(!timeout_global.interface);
        KBUG_ON(!timeout_global.interface->timeout_lock_func);

        return INDIRECT_BRANCH_SAFE(
                timeout_global.interface->timeout_lock_func(node));
    }
}

void timeout_unlock(struct mcsnode *node)
{
    if (timeout_global.initialized) {

        KBUG_ON(!timeout_global.interface);
        KBUG_ON(!timeout_global.interface->timeout_unlock_func);

        return INDIRECT_BRANCH_SAFE(
                timeout_global.interface->timeout_unlock_func(node));
    }
}

void __timeout_insert_task(struct task *task, u32 ticks)
{
    if (timeout_global.initialized) {

        KBUG_ON(!timeout_global.interface);
        KBUG_ON(!timeout_global.interface->__timeout_insert_task_func);

        return INDIRECT_BRANCH_SAFE(
                timeout_global.interface->__timeout_insert_task_func(
                    task, ticks));
    }
}

bool __timeout_dequeue_task(struct task *task)
{
    if (!timeout_global.initialized)
        return false;

    KBUG_ON(!timeout_global.interface);
    KBUG_ON(!timeout_global.interface->__timeout_dequeue_task_func);
    
    return INDIRECT_BRANCH_SAFE(
            timeout_global.interface->__timeout_dequeue_task_func(task));
}

u64 timeout_period_fs(void)
{
    if (!timeout_global.initialized)
        return 0;

    KBUG_ON(!timeout_global.interface);
    KBUG_ON(!timeout_global.interface->timeout_period_fs_func);
    
    return INDIRECT_BRANCH_SAFE(
            timeout_global.interface->timeout_period_fs_func());
}

u64 timeout_frequency_hz(void)
{
    if (!timeout_global.initialized)
        return 0;
    
    KBUG_ON(!timeout_global.interface);
    KBUG_ON(!timeout_global.interface->timeout_frequency_hz_func);
    
    return INDIRECT_BRANCH_SAFE(
            timeout_global.interface->timeout_frequency_hz_func());
}

void timeout_insert_task(struct task *task, u32 ticks)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    timeout_lock(&node);
    __timeout_insert_task(task, ticks);
    timeout_unlock(&node);
}

#endif