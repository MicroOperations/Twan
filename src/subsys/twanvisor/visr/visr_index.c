#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/subsys/twanvisor/visr/visr_dispatcher.h>
#include <include/subsys/twanvisor/visr/visr_index.h>
#include <include/subsys/twanvisor/twanvisor.h>
#include <include/subsys/twanvisor/vemulate/vemulate_utils.h>
#include <include/subsys/twanvisor/vsched/vsched_timer.h>
#include <include/kernel/kapi.h>

extern void __vemulate_interrupt(u64 rsp, u8 vector, u64 errcode);

static vector_t vreserved_exception_vectors[] = {
    DIVIDE_ERROR,
    DEBUG_EXCEPTION,
    BREAKPOINT,
    OVERFLOW,
    BOUND_RANGE_EXCEEDED,
    INVALID_OPCODE,
    DEVICE_NOT_AVAILABLE,
    DOUBLE_FAULT,
    COPROCESSOR_SEGMENT_OVERRUN,
    INVALID_TSS,
    SEGMENT_NOT_PRESENT,
    STACK_SEGMENT_FAULT,
    GENERAL_PROTECTION_FAULT,
    PAGE_FAULT,
    MATH_FAULT,
    ALIGNMENT_CHECK,
    SIMD_FLOATING_POINT_EXCEPTION,
    VIRTUALISATION_EXCEPTION,
    CONTROL_PROTECTION_EXCEPTION,
};

void vset_available_vectors(struct bmp256 *available_vectors)
{
    bmp256_set_all(available_vectors);

    bmp256_unset(available_vectors, DOUBLE_FAULT);
    bmp256_unset(available_vectors, MACHINE_CHECK);
    bmp256_unset(available_vectors, VSPURIOUS_INT_VECTOR);
    bmp256_unset(available_vectors, VSCHED_TIMER_VECTOR);
    bmp256_unset(available_vectors, VIPI_VECTOR);
}

__unroll_loops bool vis_exception_vector(u8 vector)
{
    for (u32 i = 0; i < ARRAY_LEN(vreserved_exception_vectors); i++) {
        
        if (vreserved_exception_vectors[i] == vector)
            return true;
    }
    
    return false;
}

void vemulate_self_ipi(ipi_func_t func, u64 arg)
{
    u64 flags = read_flags_and_disable_interrupts();

    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    vthis_cpu->vipi_data.is_self_ipi = true;
    vthis_cpu->vipi_data.self_ipi.func = func;
    vthis_cpu->vipi_data.self_ipi.arg = arg;
    
    u64 rsp = (u64)&vthis_cpu->int_stack[sizeof(vthis_cpu->int_stack)];

    __vemulate_interrupt(rsp, VIPI_VECTOR, 0);

    write_flags(flags);
}

int vipi_async(u32 vprocessor_id)
{
    u32 this_processor_id = vthis_vprocessor_id();
    if (this_processor_id == vprocessor_id)
        return -EINVAL;

    struct vtwan_kernel *vkernel = vtwan();

    if (vprocessor_id >= vkernel->num_enabled_cpus)
        return -EINVAL;

    struct vper_cpu *vtarget_cpu = &vkernel->per_cpu_data[vprocessor_id];

    u32 lapic_id = vtarget_cpu->lapic_id;

    u64 flags = read_flags_and_disable_interrupts();

    vlapic_send_ipi(lapic_id, DM_NORMAL, LAPIC_DEST_PHYSICAL, 
                    SINGLE_TARGET, VIPI_VECTOR);

    write_flags(flags);

    return 0;
}

int vipi_queue_ack(u32 vprocessor_id, struct vcpu *vcpu)
{
    u32 this_processor_id = vthis_vprocessor_id();
    if (this_processor_id == vprocessor_id)
        return -EINVAL;

    struct vtwan_kernel *vkernel = vtwan();

    if (vprocessor_id >= vkernel->num_enabled_cpus)
        return -EINVAL;

    struct vper_cpu *vtarget_cpu = vper_cpu_data(vprocessor_id);
    struct mcsnode node = INITIALIZE_MCSNODE();

    struct dq *dq = &vtarget_cpu->vipi_data.vcpus.vcpu_sender_dq;
    struct list_double *ipi_node = &vcpu->ipi_nodes[vprocessor_id];

    vmcs_lock_isr_save(&vtarget_cpu->vipi_data.vcpus.lock, &node);

    if (!dq_is_queued(dq, ipi_node))
        dq_pushback(dq, ipi_node);

    vmcs_unlock_isr_restore(&vtarget_cpu->vipi_data.vcpus.lock, &node);

    return 0;
}

void vipi_ack(void)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();
    
    struct mcsnode ipi_node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vthis_cpu->vipi_data.vcpus.lock, &ipi_node);
    dq_clear(&vthis_cpu->vipi_data.vcpus.vcpu_sender_dq);
    vmcs_unlock_isr_restore(&vthis_cpu->vipi_data.vcpus.lock, &ipi_node);
}

int vipi_check_ack(u32 vprocessor_id, struct vcpu *vcpu)
{
    u32 this_processor_id = vthis_vprocessor_id();
    if (this_processor_id == vprocessor_id)
        return -EINVAL;

    struct vtwan_kernel *vkernel = vtwan();

    if (vprocessor_id >= vkernel->num_enabled_cpus)
        return -EINVAL;

    struct vper_cpu *vtarget = vper_cpu_data(vprocessor_id);
    struct list_double *node = &vcpu->ipi_nodes[vprocessor_id];

    struct mcsnode ipi_node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vtarget->vipi_data.vcpus.lock, &ipi_node);
    bool ret = dq_is_queued(&vtarget->vipi_data.vcpus.vcpu_sender_dq, node);
    vmcs_unlock_isr_restore(&vtarget->vipi_data.vcpus.lock, &ipi_node);

    return ret ? 0 : 1;
}

void vhandle_buggy_lapic_isr(void)
{
    /* on certain testing equipment, interrupts occurring in parallel with
       vmexits were causing the lapic ISR to be set but not vectored, and
       would then cause a vmexit due to external interrupt on vmentry. we 
       should handle this case here by draining the ISR before we drain IPIs
       from the IRR, when this is done, the interrupt does not get vectored nor
       does it cause the vmexit due to external interrupt on vmentry, */

    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    for (int i = 255; i >= 240; i--) {

        if (vis_lapic_isr_set(i)) {

            switch (i) {

                case VSCHED_TIMER_VECTOR:

                    if (vis_sched_timer_done())
                        vcurrent_vcpu()->flags.fields.yield_request = 1;

                    break;

                case VIPI_VECTOR:

                    if (vthis_cpu->vipi_data.dead)
                        vdead_local();

                    break;

#if SPURIOUS_INT_VECTOR >= 240

                case VSPURIOUS_INT_VECTOR:
                    break;
#endif

                default:
                
                    vdispatch_interrupt(i);
                    break;
            }

            vlapic_eoi();
        }
    }
}

void vipi_drain_no_yield(void)
{
    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    u64 flags = read_flags_and_disable_interrupts();

    vhandle_buggy_lapic_isr();

    if (vis_lapic_irr_set(VIPI_VECTOR)) {

        vthis_cpu->vipi_data.vcpus.drain = 1;

        vcurrent_vcpu_disable_preemption();
        enable_interrupts();
        
        spin_until(vthis_cpu->vipi_data.vcpus.drain == 0);

        disable_interrupts();
        vcurrent_vcpu_enable_preemption_no_yield();
    }

    write_flags(flags);
}

void vdead_local(void)
{
    disable_interrupts();
    halt_loop();
}

void vdead_global(void)
{
    struct vtwan_kernel *vkernel = vtwan();

    u32 me = vthis_vprocessor_id();

    u32 num_enabled_cpus = vkernel->num_enabled_cpus;
    for (u32 i = 0; i < num_enabled_cpus; i++) {

        if (i != me)  {
            vper_cpu_data(i)->vipi_data.dead = true;
            vipi_async(i);
        }
    }

    vdead_local();
}

#endif