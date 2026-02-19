#ifndef _VISR_INDEX_H_
#define _VISR_INDEX_H_

#include <include/subsys/twanvisor/vsched/vcpu.h>
#include <include/kernel/isr/base_isrs.h>
#include <include/kernel/isr/isr_index.h>

#define VSCHED_TIMER_VECTOR 255
#define VIPI_VECTOR 254
#define VSPURIOUS_INT_VECTOR SPURIOUS_INT_VECTOR

STATIC_ASSERT(VSCHED_TIMER_VECTOR != VSPURIOUS_INT_VECTOR);
STATIC_ASSERT(VIPI_VECTOR != VSPURIOUS_INT_VECTOR);

STATIC_ASSERT(vector_to_intl(VSCHED_TIMER_VECTOR) == INTL_MAX);
STATIC_ASSERT(vector_to_intl(VIPI_VECTOR) == INTL_MAX);

void vset_available_vectors(struct bmp256 *available_vectors);
bool vis_exception_vector(u8 vector);

void vemulate_self_ipi(ipi_func_t func, u64 arg);

int vipi_async(u32 vprocessor_id);

int vipi_queue_ack(u32 vprocessor_id, struct vcpu *vcpu);
void vipi_ack(void);
int vipi_check_ack(u32 vprocessor_id, struct vcpu *vcpu);

void vhandle_buggy_lapic_isr(void);

void vipi_drain_no_yield(void);

void vdead_local(void);
void vdead_global(void);

#endif