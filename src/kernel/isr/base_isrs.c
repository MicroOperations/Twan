#include <kernel/isr/base_isrs.h>
#include <kernel/isr/isr_dispatcher.h>
#include <kernel/kapi.h>

int ipi_cmd_isr(void)
{
    ipi_handler();
    return ISR_DONE;
}

int self_ipi_cmd_isr(void)
{
    self_ipi_handler();
    return ISR_DONE;
}

int spurious_int_isr(void)
{
    return ISR_DONE;
}

int sched_timer_isr(void)
{
    struct interrupt_info *ctx = task_ctx();
    sched_preempt(ctx);

    return ISR_DONE;
}

void register_base_isrs_local(void)
{
    register_isr_local(IPI_CMD_VECTOR, ipi_cmd_isr);
    register_isr_local(SELF_IPI_CMD_VECTOR, self_ipi_cmd_isr);
    register_isr_local(SPURIOUS_INT_VECTOR, spurious_int_isr);
    register_isr_local(SCHED_TIMER_VECTOR, sched_timer_isr);
}