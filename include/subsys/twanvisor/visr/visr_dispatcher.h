#ifndef _VISR_DISPATCHER_H_
#define _VISR_DISPATCHER_H_

#include <subsys/twanvisor/visr/visr_index.h>

void vacknowledge_interrupt(u8 vector);

void vpanic_exception(struct interrupt_info *stack_trace);

void vdispatch_interrupt(u8 vector);
void __visr_dispatcher(struct interrupt_info *stack_trace);

void vexit_ext_dispatcher(u8 vector);

#endif