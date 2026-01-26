#ifndef _ISR_INDEX_H_
#define _ISR_INDEX_H_

#define NUM_VECTORS 256
#define NUM_RESERVED_VECTORS 32

#ifndef ASM_FILE

#include <include/lib/x86_index.h>
#include <include/arch.h>
#include <include/lib/atomic.h>

#define EXCEPTON_UNHANDLED -1
#define EXCEPTION_HANDLED 0

#define ISR_DONE 0

#define IPI_UNLOCKED 0
#define IPI_LOCKED 1
#define IPI_PAUSED 2

typedef int (*isr_func_t)(struct interrupt_info *stack_trace);
typedef void (*ipi_func_t)(struct interrupt_info *stack_trace, u64 arg);

struct ipi_data
{
    ipi_func_t func;
    u64 arg;
    bool wait;
    atomic32_t signal;
};

extern bool critical_exceptions[];
extern bool has_error_code[NUM_RESERVED_VECTORS];

extern interrupt_type_t interrupt_type_arr[NUM_RESERVED_VECTORS];
extern char *vector_to_str[NUM_RESERVED_VECTORS];

u64 get_int_stack(struct interrupt_info *ctx);

#endif

#endif