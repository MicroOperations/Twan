#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/subsys/twanvisor/visr/visr_dispatcher.h>
#include <include/subsys/twanvisor/twanvisor.h>
#include <include/subsys/twanvisor/visr/visr_index.h>
#include <include/subsys/twanvisor/vemulate/vemulate_utils.h>
#include <include/subsys/twanvisor/vsched/vsched_timer.h>
#include <include/subsys/twanvisor/vsched/vsched_yield.h>
#include <include/kernel/isr/isr_dispatcher.h>
#include <include/kernel/apic/apic.h>

void vacknowledge_interrupt(u8 vector)
{
    if (vis_lapic_isr_set(vector))
        vlapic_eoi();
}

void vpanic_exception(struct interrupt_info *stack_trace)
{
    u8 vector = stack_trace->dispatch_info.fields.vector;

    char *exception_str = NULL;
    if (vector >= ARRAY_LEN(vector_to_str))
        exception_str = "UNKNOWN";
    else
        exception_str = vector_to_str[vector];

    /* cut logs into seperate vdbg's incase stack size is small */

    vdbg("[GLOBAL VPANIC]\n");
    vdbg("TRACE:\n");

    vdbgf("[CPU]: %u\n", this_processor_id());
    vdbgf("[EXCEPTION]: %s\n", exception_str);

    vdbgf("[ERRCODE]: 0x%lx\n", stack_trace->errcode);
    vdbgf("[RIP]: 0x%lx\n", stack_trace->rip);
    vdbgf("[RSP]: 0x%lx\n", stack_trace->rsp);

    vdbgf("[RBP]: 0x%lx\n", stack_trace->regs.rbp);
    vdbgf("[RAX]: 0x%lx\n", stack_trace->regs.rax);
    vdbgf("[RBX]: 0x%lx\n", stack_trace->regs.rbx);
    vdbgf("[RCX]: 0x%lx\n", stack_trace->regs.rcx);
    vdbgf("[RDX]: 0x%lx\n", stack_trace->regs.rdx);
    vdbgf("[RDI]: 0x%lx\n", stack_trace->regs.rdi);
    vdbgf("[RSI]: 0x%lx\n", stack_trace->regs.rsi);

    vdbgf("[R8]: 0x%lx\n", stack_trace->regs.r8);
    vdbgf("[R9]: 0x%lx\n", stack_trace->regs.r9);
    vdbgf("[R10]: 0x%lx\n", stack_trace->regs.r10);
    vdbgf("[R11]: 0x%lx\n", stack_trace->regs.r11);
    vdbgf("[R12]: 0x%lx\n", stack_trace->regs.r12);
    vdbgf("[R13]: 0x%lx\n", stack_trace->regs.r13);
    vdbgf("[R14]: 0x%lx\n", stack_trace->regs.r14);
    vdbgf("[R15]: 0x%lx\n", stack_trace->regs.r15);

    vdbgf("[RFLAGS]: 0x%lx\n", stack_trace->rflags);
    vdbgf("[CS]: 0x%lx\n", stack_trace->cs);
    vdbgf("[SS]: 0x%lx\n", stack_trace->ss);

    vdbgf("[CR0]: 0x%lx\n", __read_cr0().val);
    vdbgf("[CR2]: 0x%lx\n", __read_cr2());
    vdbgf("[CR3]: 0x%lx\n", __read_cr3().val);
    vdbgf("[CR4]: 0x%lx\n", __read_cr4().val);

    vdead_global();
}

void vdispatch_interrupt(u8 vector)
{
    /* optimise by clearing the dq after setting the nodes as pending */

    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vthis_cpu->dispatch_lock, &node);

    struct dq *dq = &vthis_cpu->dispatch_vectors[vector];
    struct list_double *cur = dq_peekfront(dq);

    while (cur) {

        struct vcpu *vcpu = dispatch_node_to_vcpu(cur, vector);
        vemu_set_interrupt_pending(vcpu, vector, vector == NMI);

        struct list_double *next = cur->next;
       
        cur->next = NULL;
        cur->prev = NULL;
        cur = next;
    }

    dq->front = NULL;
    dq->rear = NULL;

    vmcs_unlock_isr_restore(&vthis_cpu->dispatch_lock, &node);
}

void __visr_dispatcher(struct interrupt_info *stack_trace)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    u8 vector = stack_trace->dispatch_info.fields.vector;
    bool emulated = stack_trace->dispatch_info.fields.emulated != 0;
    bool was_in_isr = vthis_cpu->handling_isr;

    if (!was_in_isr) {
        vthis_cpu->handling_isr = true;
        vthis_cpu->vcpu_ctx = stack_trace;
    }

    bool should_dispatch = false;
    
    switch (vector) {

        case VSCHED_TIMER_VECTOR:

            if (vis_sched_timer_done())
                vsched_preempt_isr();
                
            break;
        
        case VIPI_VECTOR:

            if (vthis_cpu->vipi_data.dead)
                vdead_local();

            if (vthis_cpu->vipi_data.is_self_ipi) {

                ipi_func_t self_ipi_func = vthis_cpu->vipi_data.self_ipi.func;
                u64 self_ipi_arg = vthis_cpu->vipi_data.self_ipi.arg;
               
                if (self_ipi_func)
                    self_ipi_func(self_ipi_arg);

                vthis_cpu->vipi_data.is_self_ipi = false;
            } else {
                vthis_cpu->vipi_data.vcpus.drain = 0;
            }

            break;

        case VSPURIOUS_INT_VECTOR:
            break;

        default:

            if (vis_lapic_isr_set(vector)) {

                /* root should not route interrupts here */
                VBUG_ON(vector == DOUBLE_FAULT);
                VBUG_ON(vector == MACHINE_CHECK);

                should_dispatch = true;
                break;

            } else {

                /* root should route nmi's as external */
                VBUG_ON(vector == NMI);
            }

            if (vis_exception_vector(vector)) {
                    
                if (vthis_cpu->shield.ext.fields.active != 0 && 
                    vthis_cpu->shield.ext.fields.exception_vector == vector) {

                    vthis_cpu->shield.ext.fields.faulted = 1;

                    if (vthis_cpu->shield.ext.fields.recover_by_length != 0)
                        stack_trace->rip += vthis_cpu->shield.ext.fields.len;
                    else
                        stack_trace->rip = vthis_cpu->shield.recovery_rip;

                    break;
                }

                vpanic_exception(stack_trace);
                break;
            }
            
            /* (currently mce's are not supported - they will cause shutdown) */

            /* treating mce's as a special case rather than as an 'exception
               vector' */
            if (vector == MACHINE_CHECK) {

            }

            break;
    }

    if (should_dispatch)
        vdispatch_interrupt(vector);

    if (!emulated)
        vacknowledge_interrupt(vector);
    
    if (!was_in_isr) {
        vthis_cpu->handling_isr = false;
        vthis_cpu->vcpu_ctx = NULL;
    }
}

void vexit_ext_dispatcher(u8 vector)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    bool yield_pending = false;

    switch (vector) {

        case VSCHED_TIMER_VECTOR:

            if (vis_sched_timer_done())
                yield_pending = true;

            break;

        case VIPI_VECTOR:

            if (vthis_cpu->vipi_data.dead)
                vdead_local();

            break;

        case VSPURIOUS_INT_VECTOR:
            break;

        default:

            /* root should not route interrupts to these vectors  */
            VBUG_ON(vector == DOUBLE_FAULT);
            VBUG_ON(vector == MACHINE_CHECK);

            vdispatch_interrupt(vector);
            break;
    }

    vacknowledge_interrupt(vector);

#if TWANVISOR_VIPI_DRAIN_STRICT

    /* external interrupts are treated as a special case in how we should 
       handle vipi draining */
    vipi_drain_no_yield();

#endif

    if (yield_pending)
        vsched_yield();
}

#endif