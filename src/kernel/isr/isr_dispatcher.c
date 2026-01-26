#include <include/kernel/isr/isr_dispatcher.h>
#include <include/kernel/isr/isr_index.h>
#include <include/kernel/kernel.h>
#include <include/subsys/debug/kdbg/kdbg.h>
#include <include/kernel/apic/apic.h>
#include <include/kernel/sched/sched.h>
#include <include/kernel/kapi.h>
#include <include/subsys/twanvisor/vconf.h>
#include <include/subsys/watchdog/watchdog.h>
#include <include/lib/x86_index.h>
#include <include/lib/libtwanvisor/libvcalls.h>
#include <include/lib/libtwanvisor/libvc.h>

void __ipi_assert(struct ipi_data *data)
{
    atomic32_set(&data->signal, IPI_LOCKED);
}

void __ipi_ack(u32 sender_processor_id)
{
    struct twan_kernel *kernel = twan();
    u32 processor_id = this_processor_id();

    struct ipi_data *data = 
        &kernel->ipi_table[processor_id][sender_processor_id];

#if TWANVISOR_PV_IPIS

    if (kernel->flags.fields.twanvisor_on != 0) {
        
        int expected = IPI_LOCKED;
        if (!atomic32_cmpxchg(&data->signal, &expected, IPI_UNLOCKED)) {

            atomic32_set(&data->signal, IPI_UNLOCKED);
            tv_vpv_spin_kick(sender_processor_id);
        }

        return;
    }

#endif

    atomic32_set(&data->signal, IPI_UNLOCKED);
}

void __ipi_wait(u32 target_processor_id)
{
    struct twan_kernel *kernel = twan();
    u32 processor_id = this_processor_id();

    struct ipi_data *data = 
        &kernel->ipi_table[target_processor_id][processor_id];

#if TWANVISOR_PV_IPIS

    if (kernel->flags.fields.twanvisor_on != 0) {

        while (atomic32_read(&data->signal) != IPI_UNLOCKED) {

            if (!tv_vis_vcpu_active(target_processor_id)) {

                int expected = IPI_LOCKED;
                if (atomic32_cmpxchg(&data->signal, &expected, IPI_PAUSED))
                    tv_vpv_spin_pause();
            }
        }

        return;
    }

#endif

    spin_until(atomic32_read(&data->signal) == IPI_UNLOCKED);
}

void __acknowledge_interrupt(void)
{
#if TWANVISOR_ON

    if (twan()->flags.fields.twanvisor_on != 0) {
        tv_vintc_eoi();
        return;
    }

#endif

    lapic_eoi();
}

bool __in_service(u8 vector)
{
#if TWANVISOR_ON

    if (twan()->flags.fields.twanvisor_on != 0)
        return tv_vintc_is_serviced(vector);

#endif

    return is_lapic_isr_set(vector);
}

bool is_critical_exception(u8 vector)
{
    return vector < ARRAY_LEN(interrupt_type_arr) && 
           interrupt_type_arr[vector] == INTERRUPT_TYPE_HARDWARE_EXCEPTION;
}

void log_exception(struct interrupt_info *stack_trace)
{
    u8 vector = stack_trace->vector;

    char *exception_str = NULL;
    if (vector >= ARRAY_LEN(vector_to_str))
        exception_str = "UNKNOWN";
    else
        exception_str = vector_to_str[vector];

    /* cut logs into seperate kdbg's incase stack size is small */

    kdbg("TRACE:\n");

    kdbgf("[CPU]: %u\n", this_processor_id());
    kdbgf("[EXCEPTION]: %s\n", exception_str);

    kdbgf("[ERRCODE]: 0x%lx\n", stack_trace->errcode);
    kdbgf("[RIP]: 0x%lx\n", stack_trace->rip);
    kdbgf("[RSP]: 0x%lx\n", stack_trace->rsp);

    kdbgf("[RBP]: 0x%lx\n", stack_trace->regs.rbp);
    kdbgf("[RAX]: 0x%lx\n", stack_trace->regs.rax);
    kdbgf("[RBX]: 0x%lx\n", stack_trace->regs.rbx);
    kdbgf("[RCX]: 0x%lx\n", stack_trace->regs.rcx);
    kdbgf("[RDX]: 0x%lx\n", stack_trace->regs.rdx);
    kdbgf("[RDI]: 0x%lx\n", stack_trace->regs.rdi);
    kdbgf("[RSI]: 0x%lx\n", stack_trace->regs.rsi);

    kdbgf("[R8]: 0x%lx\n", stack_trace->regs.r8);
    kdbgf("[R9]: 0x%lx\n", stack_trace->regs.r9);
    kdbgf("[R10]: 0x%lx\n", stack_trace->regs.r10);
    kdbgf("[R11]: 0x%lx\n", stack_trace->regs.r11);
    kdbgf("[R12]: 0x%lx\n", stack_trace->regs.r12);
    kdbgf("[R13]: 0x%lx\n", stack_trace->regs.r13);
    kdbgf("[R14]: 0x%lx\n", stack_trace->regs.r14);
    kdbgf("[R15]: 0x%lx\n", stack_trace->regs.r15);

    kdbgf("[RFLAGS]: 0x%lx\n", stack_trace->rflags);
    kdbgf("[CS]: 0x%lx\n", stack_trace->cs);
    kdbgf("[SS]: 0x%lx\n", stack_trace->ss);

    kdbgf("[CR0]: 0x%lx\n", __read_cr0().val);
    kdbgf("[CR2]: 0x%lx\n", __read_cr2());
    kdbgf("[CR3]: 0x%lx\n", __read_cr3().val);
    kdbgf("[CR4]: 0x%lx\n", __read_cr4().val);
}

void ipi_handler(struct interrupt_info *info)
{
    struct twan_kernel *kernel = twan();
    u32 processor_id = this_processor_id();

    for (u32 i = 0; i < ARRAY_LEN(kernel->ipi_table[processor_id]); i++) {

        if (i == processor_id)
            continue;

        struct ipi_data *data = &kernel->ipi_table[processor_id][i];
        if (atomic32_read(&data->signal) == IPI_UNLOCKED)
            continue;

        ipi_func_t func = data->func;
        u64 arg = data->arg;
        bool wait = data->wait;

        if (wait) {

            func(info, arg);
            __ipi_ack(i);

        } else {

            __ipi_ack(i);
            func(info, arg);
        }
    }
}

void self_ipi_handler(struct interrupt_info *info)
{
    u32 processor_id = this_processor_id();

    struct ipi_data *data = &twan()->ipi_table[processor_id][processor_id];

    if (atomic32_read(&data->signal) == IPI_UNLOCKED)
        return;

    u64 arg = data->arg;
    ipi_func_t func = data->func;

    func(info, arg);
    atomic32_set(&data->signal, IPI_UNLOCKED);
}

void __isr_dispatcher(struct interrupt_info *stack_trace)
{
    struct per_cpu *this_cpu = this_cpu_data();

    u8 vector = stack_trace->vector;
    bool was_in_isr = this_cpu->handling_isr;
    int old_intl = this_cpu->intl;

    if (!was_in_isr) {
        this_cpu->handling_isr = true;
        this_cpu->task_ctx = stack_trace;
    }

    bool emulated = stack_trace->emulated == 1;
    
    int intl = vector_to_intl(vector);
    bool external = !emulated && intl > old_intl && __in_service(vector);

    if (external)
        this_cpu->intl = intl;

    int ret = EXCEPTON_UNHANDLED;

    isr_func_t func = atomic_ptr_read(&this_cpu->isr_table[vector]);
    if (func)
        ret = INDIRECT_BRANCH_SAFE(func(stack_trace));

    /* interrupts must be disabled here to prevent our stack being reused
       by same priority interrupt when we acknowledge */

    disable_interrupts();

    if (!external) {

        if (is_critical_exception(vector) && ret != EXCEPTION_HANDLED)
            kpanic_exec_local(log_exception(stack_trace));            

    } else {
        __acknowledge_interrupt();
    }
    
    if (!was_in_isr) {
        this_cpu->handling_isr = false;
        this_cpu->task_ctx = NULL;
    }

    this_cpu->intl = old_intl;
}