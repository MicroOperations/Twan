#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vportal/venter.h>
#include <subsys/twanvisor/vportal/vrecovery.h>
#include <subsys/twanvisor/vsched/vsched_timer.h>
#include <subsys/twanvisor/visr/visr_dispatcher.h>
#include <subsys/twanvisor/vemulate/vemulate_utils.h>

void __venter(void)
{
    disable_interrupts();

    struct vper_cpu *vthis_cpu = vthis_cpu_data();

    /* if we got preempted we need to reload the architectural guest state */

    struct vcpu *current = vcurrent_vcpu();
    if (current->flags.fields.preempted != 0) {

        if (!__vmptrld(current->arch.vmcs_phys))
            vfailure_recover();

        current->flags.fields.preempted = 0;
    }

    /* invalidate the tlb context, transition to running and check which 
       scheduling operations are pending */

    struct vscheduler *vsched = vthis_vscheduler();
    struct mcsnode vsched_node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vsched->lock, &vsched_node);

    if (current->vsched_metadata.terminate) {
        vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);
        vtransitioning_recover();
    }

    current->vsched_metadata.state = VRUNNING;

    vpid_t vpid = current->vpid;
    bool invvpid = current->vsched_metadata.tlb_flush_pending && 
                   vpid.fields.enabled != 0;

    current->vsched_metadata.tlb_flush_pending = false;

    u32 ticks = current->vsched_metadata.current_time_slice_ticks;
    bool reearm = current->vsched_metadata.rearm;
    current->vsched_metadata.rearm = false;

    vmcs_unlock_isr_restore(&vsched->lock, &vsched_node);

    /* carry out all pending operations */

    if (invvpid)
        invvpid_single(vpid.fields.vpid);

    if (reearm)
        vsched_arm_timer(ticks);

    if (current->voperation_queue.pending.fields.vmwrite_cr4 != 0)
        __vmwrite(VMCS_GUEST_CR4, current->voperation_queue.cr4.val);

    if (current->voperation_queue.pending.fields.should_advance != 0)
        vadvance_guest_rip();

    current->voperation_queue.pending.val = 0;

    /* inject any pending interrupt based on precedence */

    int external_vector = -1;

    struct mcsnode pending_node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&current->visr_pending.lock, &pending_node);

    vinterrupt_delivery_data_t data = current->visr_pending.delivery;

    int exception_count = data.fields.gp0_pending + data.fields.ud_pending +
                          data.fields.db_pending + data.fields.ac0_pending;

    VBUG_ON(exception_count > 1);

    bool gp0_pending = data.fields.gp0_pending != 0;
    bool ud_pending = data.fields.ud_pending != 0;
    bool db_pending = data.fields.db_pending != 0;
    bool ac0_pending = data.fields.ac0_pending != 0;
    bool nmi_pending = data.fields.nmi_pending != 0;

    bool in_nmi = data.fields.in_nmi != 0;
    bool nmi_window_exiting = data.fields.nmi_window_exit != 0;
    bool int_window_exiting = data.fields.int_window_exit != 0;

    struct bmp256 *pending = &current->visr_pending.pending_external_interrupts;
    int current_intl = current->visr_pending.delivery.fields.intl;
    u32 in_intl = current->visr_pending.delivery.fields.in_intl;

    bool injected = false;
    if (gp0_pending) {

        vinject_gp(0);
        current->visr_pending.delivery.fields.gp0_pending = 0;
        injected = true;

    } else if (ud_pending) {

        vinject_ud();
        current->visr_pending.delivery.fields.ud_pending = 0;
        injected = true;

    } else if (db_pending) {

        u32 int_type = data.fields.int_type;

        switch (int_type) {
        
            case INTERRUPT_TYPE_HARDWARE_EXCEPTION:

                vinject_db(int_type, false, 0);
                break;

            case INTERRUPT_TYPE_SOFTWARE_INT:
            case INTERRUPT_TYPE_PRIVILEGED_SOFTWARE_EXCEPTION:
            case INTERRUPT_TYPE_SOFTWARE_EXCEPTION:

                u32 len = vmread(VMCS_RO_VMEXIT_INSTRUCTION_LENGTH);
                vinject_db(int_type, true, len);
                break;

            default:
                VBUG_ON(true);
                break;
        }

        current->visr_pending.delivery.fields.db_pending = 0;
        injected = true;

    } else if (ac0_pending) {

        vinject_ac(0);
        current->visr_pending.delivery.fields.ac0_pending = 0;
        injected = true;

    } else if (nmi_pending) {

        if (in_nmi || vis_nmis_blocked()) {
            
            if (!nmi_window_exiting) {

                vmx_procbased_ctls_t proc = {
                    .val = vmread(VMCS_CTRL_PROCBASED_CTLS)
                };

                proc.fields.nmi_window_exiting = 1;
                __vmwrite(VMCS_CTRL_PROCBASED_CTLS, proc.val);
                current->visr_pending.delivery.fields.nmi_window_exit = 1;
            }

        } else {

            if (nmi_window_exiting) {

                vmx_procbased_ctls_t proc = {
                    .val = vmread(VMCS_CTRL_PROCBASED_CTLS)
                };

                proc.fields.nmi_window_exiting = 0;
                __vmwrite(VMCS_CTRL_PROCBASED_CTLS, proc.val);
                current->visr_pending.delivery.fields.nmi_window_exit = 0;
            }

            vinject_interrupt_external(NMI, true);

            current->visr_pending.delivery.fields.nmi_pending = 0;
            current->visr_pending.delivery.fields.in_nmi = 1;
            external_vector = NMI;
            injected = true;
        }
        
    } 
    
    int vector = bmp256_fls(pending);
    
    if (vector >= 0 && !injected) {

        int serviced_intl = fls32(in_intl);
        int intl = vector_to_intl(vector);

        if (intl > serviced_intl && intl >= current_intl) {

            if (vis_external_interrupts_blocked()) {

                if (!int_window_exiting) {

                    vmx_procbased_ctls_t proc = {
                        .val = vmread(VMCS_CTRL_PROCBASED_CTLS)
                    };

                    proc.fields.interrupt_window_exiting = 1;
                    __vmwrite(VMCS_CTRL_PROCBASED_CTLS, proc.val);
                    current->visr_pending.delivery.fields.int_window_exit = 1;
                }

            } else {

                if (int_window_exiting) {

                    vmx_procbased_ctls_t proc = {
                        .val = vmread(VMCS_CTRL_PROCBASED_CTLS)
                    };

                    proc.fields.interrupt_window_exiting = 0;
                    __vmwrite(VMCS_CTRL_PROCBASED_CTLS, proc.val);
                    current->visr_pending.delivery.fields.int_window_exit = 0;
                }

                vinject_interrupt_external(vector, false);

                bmp256_unset(pending, vector);
                current->visr_pending.delivery.fields.in_intl |= (1 << intl);
                external_vector = vector;
            }
        }
    }

    vmcs_unlock_isr_restore(&current->visr_pending.lock, &pending_node);

    /* requeue the vcpu if it is subscribed to receive events from the vector */
    if (external_vector != -1) {

        struct dq *dq = &vthis_cpu->dispatch_vectors[external_vector];
        struct list_double *node = &current->vdispatch_nodes[external_vector];
        struct bmp256 *subscriptions = &current->visr_metadata.subscribed_vectors;

        struct mcsnode dispatch_node = INITIALIZE_MCSNODE();
        vmcs_lock_isr_save(&vthis_cpu->dispatch_lock, &dispatch_node);

        if (bmp256_test(subscriptions, external_vector))
            dq_pushback(dq, node);

        vmcs_unlock_isr_restore(&vthis_cpu->dispatch_lock, &dispatch_node);
    }
}

#endif