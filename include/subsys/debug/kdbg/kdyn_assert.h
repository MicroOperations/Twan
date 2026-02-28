#ifndef _KDYN_ASSERT_H_
#define _KDYN_ASSERT_H_

#include <subsys/debug/kdbg/kdbg.h>

#if CONFIG_KDBG_KDYNAMIC_ASSERT

#define KDYNAMIC_ASSERT(cond)                                           \
    ((void)KBUG_ON_RAW(!(cond),                                         \
                        "assertation failed: %s, file: %s, line: %d\n", \
                        #cond, __FILE__, __LINE__))

#else

#define KDYNAMIC_ASSERT(cond)

#endif

#endif