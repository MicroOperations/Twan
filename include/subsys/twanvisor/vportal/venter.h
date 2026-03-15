#ifndef _VENTER_H_
#define _VENTER_H_

#include <subsys/twanvisor/twanvisor.h>

typedef enum 
{
    VINJECT_EXCEPTION,
    VINJECT_NMI,
    VINJECT_WAITING,
    VINJECT_NONE,
} venter_inject_state_t;

void __venter(void);

#endif