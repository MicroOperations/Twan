#ifndef _VENTER_H_
#define _VENTER_H_

#include <subsys/twanvisor/twanvisor.h>

typedef enum 
{
    VINJECT_NONE,
    VINJECTED_EXCEPTION,
    VINJECTED_NMI,
    VNMI_WINDOW_EXITING,
    VINT_WINDOW_EXITING,
} venter_inject_state_t;

void __venter(void);

#endif