#ifndef _VEXIT_H_
#define _VEXIT_H_

#include <subsys/twanvisor/vsched/vcpu.h>

typedef void (*vexit_func_t)(struct vregs *vregs);

void vexit_dispatcher(struct vregs *vregs);

#endif