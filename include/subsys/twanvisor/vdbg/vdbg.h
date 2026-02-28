#ifndef _VDBG_H_
#define _VDBG_H_

#include <generated/autoconf.h>
#include <subsys/twanvisor/visr/visr_index.h>
#include <subsys/debug/kdbg/kdbg.h>

#define vpanicf_local(fmt, ...) do {                                        \
    vdbgf("[VPANIC ON CPU %u] " fmt, vthis_vprocessor_id(), ##__VA_ARGS__); \
    vdead_local();                                                          \
} while (0)

#define vpanicf_global(fmt, ...) do {                                       \
    vdbgf("[GLOBAL VPANIC, LOGGED BY CPU %u] " fmt, vthis_vprocessor_id(),  \
          ##__VA_ARGS__);                                                   \
    vdead_global();                                                         \
} while (0)

#if CONFIG_VDBG_PANIC_ON_BUG

#define VBUG_RAW(fmt, ...) \
    vpanicf_global("[BUG] " fmt, ##__VA_ARGS__)

#else

#define VBUG_RAW(fmt, ...) \
    vdbgf("[BUG] " fmt, ##__VA_ARGS__)

#endif

#define VBUG_ON_RAW(cond, fmt, ...)             \
({                                              \
    bool bug = false;                           \
                                                \
    if ((cond)) {                               \
        bug = true;                             \
        VBUG_RAW(fmt, ##__VA_ARGS__);           \
    }                                           \
                                                \
    bug;                                        \
})                                              \

#define VBUG_ON(cond)                                           \
    VBUG_ON_RAW((cond), "condition: %s, file: %s, line: %d\n",  \
                #cond, __FILE__, __LINE__)

void vdbg(const char *str);
int vdbgf(const char *fmt, ...);

#endif