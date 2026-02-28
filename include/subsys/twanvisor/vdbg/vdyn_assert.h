#ifndef _VDYN_ASSERT_H_
#define _VDYN_ASSERT_H_

#include <subsys/twanvisor/vdbg/vdbg.h>

#if CONFIG_VDBG_VDYNAMIC_ASSERT

#define VDYNAMIC_ASSERT(cond) \
    ((void)VBUG_ON(!(cond)))

#else

#define VDYNAMIC_ASSERT(cond)

#endif

#endif