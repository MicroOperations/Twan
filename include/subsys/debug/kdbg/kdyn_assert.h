#ifndef _KDYN_ASSERT_H_
#define _KDYN_ASSERT_H_

#include <subsys/debug/kdbg/kdbg.h>

#if CONFIG_KDBG_KDYNAMIC_ASSERT

#define KDYNAMIC_ASSERT(cond) \
    ((void)KBUG_ON(!(cond)))

#else

#define KDYNAMIC_ASSERT(cond)

#endif

#endif