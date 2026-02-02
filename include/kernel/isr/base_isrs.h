#ifndef _BASE_ISRS_H_
#define _BASE_ISRS_H_

#include <include/kernel/isr/isr_index.h>

/* x86 isr priority level 14 */
#define SELF_IPI_CMD_VECTOR 237
#define IPI_CMD_VECTOR 238
#define SPURIOUS_INT_VECTOR 239

/* x86 isr priority level 15 */
#define SCHED_TIMER_VECTOR 255

int ipi_cmd_isr(void);
int self_ipi_cmd_isr(void);
int spurious_int_isr(void);
int sched_timer_isr(void);

void register_base_isrs_local(void);

#endif