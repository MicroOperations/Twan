#ifndef _KDBG_H_
#define _KDBG_H_

#include <subsys/sync/mcslock.h>
#include <kernel/kapi.h>

typedef void (*__kdbg_t)(const char *str);

struct kdbg_interface
{
    __kdbg_t __kdbg_func;
};

struct kdbg 
{
    struct kdbg_interface *interface;
    struct mcslock_isr lock;
    bool initialized;
};

#define __early_kerr(str) \
    __kdbg("[ERROR] " str) 

#define __early_kpanic(str) do {    \
    __kdbg("[PANIC] " str);         \
    dead_local();                   \
} while (0)

#define kpanic_exec_local(code) do {                    \
    (code);                                             \
    dead_local();                                       \
} while (0)

#define kpanicf_local(fmt, ...) do {                                      \
    kdbgf("[PANIC ON CPU %u] " fmt, this_processor_id(), ##__VA_ARGS__);  \
    dead_local();                                                         \
} while (0)

#define kpanicf_global(fmt, ...) do {                                   \
    kdbgf("[GLOBAL PANIC, LOGGED BY CPU %u] " fmt, this_processor_id(), \
          ##__VA_ARGS__);                                               \
    dead_global();                                                      \
} while (0)

#if CONFIG_KDBG_PANIC_ON_BUG

#define KBUG_RAW(fmt, ...) \
    kpanicf_global("[BUG] " fmt, ##__VA_ARGS__)

#else

#define KBUG_RAW(fmt, ...) \
    kdbgf("[BUG] " fmt, ##__VA_ARGS__)

#endif

#define KBUG_ON_RAW(cond, fmt, ...)             \
({                                              \
    bool bug = false;                           \
                                                \
    if ((cond)) {                               \
        bug = true;                             \
        KBUG_RAW(fmt, ##__VA_ARGS__);           \
    }                                           \
                                                \
    bug;                                        \
})                                              \

#define KBUG_ON(cond)                                           \
    KBUG_ON_RAW((cond), "condition: %s, file: %s, line: %d\n",  \
                #cond, __FILE__, __LINE__)

int kdbg_init(struct kdbg_interface *interface);
bool is_kdbg_initialized(void);

void __kdbg(const char *str);
void kdbg(const char *str);

int __kdbgf(const char *fmt, ...);
int kdbgf(const char *fmt, ...);

#endif