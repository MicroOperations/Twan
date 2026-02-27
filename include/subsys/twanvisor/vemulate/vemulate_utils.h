#ifndef _VEMULATE_UTILS_H_
#define _VEMULATE_UTILS_H_

#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/vsched/vpartition.h>

typedef enum
{
    VIPI_ROUTE,
    VTLB_SHOOTDOWN_ROUTE,
    VREAD_VCPU_STATE_ROUTE,
    VPV_SPIN_KICK_ROUTE,
    VNUM_ROUTE_TYPES /* enum guard, not a valid route type */
} vroute_type_t;

typedef enum 
{
    VOP_16_BIT,
    VOP_32_BIT,
    VOP_64_BIT
} vop_mode_t;

#define MSR_READ_LOW 0
#define MSR_READ_HIGH 1024
#define MSR_WRITE_LOW 2048
#define MSR_WRITE_HIGH 3072

inline void vtrap_io(struct vcpu *vcpu, u16 port)
{
    if (port < 0x8000) 
        vcpu->arch.io_bitmap_a[port / 8] |= (1 << (port % 8));
    else 
        vcpu->arch.io_bitmap_b[port / 8] |= (1 << (port % 8));
}

inline void vuntrap_io(struct vcpu *vcpu, u16 port)
{
    if (port < 0x8000) 
        vcpu->arch.io_bitmap_a[port / 8] &= ~(1 << (port % 8));
    else 
        vcpu->arch.io_bitmap_b[port / 8] &= ~(1 << (port % 8));
}

inline bool vis_io_trapped(struct vcpu *vcpu, u16 port)
{
    bool ret;
    if (port < 0x8000) 
        ret = ((vcpu->arch.io_bitmap_a[port / 8] >> (port % 8)) & 1) != 0;
    else 
        ret = ((vcpu->arch.io_bitmap_b[port / 8] >> (port % 8)) & 1) != 0;

    return ret;
}

inline bool vin_msr_bitmap_range(u32 msr)
{
    return (msr <= 0x1ff || (msr >= 0xc0000000 && msr <= 0xc0001fff));
}

inline void vmap_msr_write(u32 msr, int *base, int *idx)
{
    if (msr <= 0x1ff) {

        *base = MSR_WRITE_LOW;
        *idx = msr;

    } else if (msr >= 0xc0000000 && msr <= 0xc0001fff) {

        *base = MSR_WRITE_HIGH;
        *idx = msr - 0xc0000000;

    } else {

        *base = -1;
        *idx = -1;
    }
}

inline void vmap_msr_read(u32 msr, int *base, int *idx)
{
    if (msr <= 0x1ff) {

        *base = MSR_READ_LOW;
        *idx = msr;

    } else if (msr >= 0xc0000000 && msr <= 0xc0001fff) {

        *base = MSR_READ_HIGH;
        *idx = msr - 0xc0000000;

    } else {

        *base = -1;
        *idx = -1;
    }
}

inline void vunvtrap_msr_write(struct vcpu *vcpu, u32 msr)
{
    int base = 0;
    int idx = 0;
    vmap_msr_write(msr, &base, &idx);

    if (base != -1 && idx != -1)
        vcpu->arch.msr_bitmap[base + (idx / 8)] &= ~(1 << (idx % 8));   
}

inline bool vis_msr_write_trapped(struct vcpu *vcpu, u32 msr)
{
    int base = 0;
    int idx = 0;
    vmap_msr_write(msr, &base, &idx);

    return base != -1 && idx != -1 &&
           ((vcpu->arch.msr_bitmap[base + (idx / 8)] >> 
            (idx % 8)) & 1) != 0;
}

inline void vunvtrap_msr_read(struct vcpu *vcpu, u32 msr)
{
    int base = 0;
    int idx = 0;
    vmap_msr_read(msr, &base, &idx);

    if (base != -1 && idx != -1)
        vcpu->arch.msr_bitmap[base + (idx / 8)] &= ~(1 << (idx % 8));   
}

inline void vunvtrap_msr(struct vcpu *vcpu, u32 msr)
{
    vunvtrap_msr_read(vcpu, msr);
    vunvtrap_msr_write(vcpu, msr);
}

inline bool vis_msr_read_trapped(struct vcpu *vcpu, u32 msr)
{
    int base = 0;
    int idx = 0;
    vmap_msr_read(msr, &base, &idx);

    return base != -1 && idx != -1 && 
           ((vcpu->arch.msr_bitmap[base + (idx / 8)] >> (
             idx % 8)) & 1) != 0;
}

inline bool vinject_interrupt(u8 vector, interrupt_type_t type, 
                            bool deliver_errcode, u64 errcode, 
                            bool deliver_length, u32 length)
{
    vmentry_interrupt_info_t info  = {
        .fields = {
            .deliver_errcode = deliver_errcode,
            .type = type,
            .vector = vector,
            .valid = 1
        }
    };

    if (!__vmwrite(VMCS_CTRL_VMENTRY_INTERRUPT_INFO, info.val))
        return false;
    
    if (deliver_errcode) {
        
        if (!__vmwrite(VMCS_CTRL_VMENTRY_INTERRUPT_ERROR_CODE, errcode))
            return false;
    }

    if (deliver_length) {

        if (!__vmwrite(VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH, length))
            return false;
    }

    return true;
}

inline bool vinject_gp(u64 errcode)
{
    return vinject_interrupt(GENERAL_PROTECTION_FAULT, 
                            INTERRUPT_TYPE_HARDWARE_EXCEPTION, true, errcode,
                            false, 0);
}

inline bool vinject_ud(void)
{
    return vinject_interrupt(INVALID_OPCODE, INTERRUPT_TYPE_HARDWARE_EXCEPTION,
                            false, 0, false, 0);
}

inline bool vinject_ac(u64 errcode)
{
    return vinject_interrupt(ALIGNMENT_CHECK, INTERRUPT_TYPE_HARDWARE_EXCEPTION,
                            true, errcode, false, 0);
}

inline bool vinject_db(interrupt_type_t int_type, bool deliver_len, u32 len)
{
    return vinject_interrupt(DEBUG_EXCEPTION, int_type,
                            false, 0, deliver_len, len);
}

inline int vget_guest_mode(void)
{
    cr0_t cr0 = {.val = vmread(VMCS_GUEST_CR0)};
    if (cr0.fields.pe == 0)
        return VOP_16_BIT;

    ia32_efer_t efer = {.val = vmread(VMCS_GUEST_IA32_EFER)};
    if (efer.fields.lma != 0) {

        access_rights_t rights = {.val = vmread32(VMCS_GUEST_CS_ACCESS_RIGHTS)};
        return rights.fields.longmode != 0 ? VOP_64_BIT : 
               rights.fields.db != 0 ? VOP_32_BIT : VOP_16_BIT;
    }

    rflags_t flags = {.val = vmread(VMCS_GUEST_RFLAGS)};
    if (flags.fields.vm != 0)
        return VOP_16_BIT;

    access_rights_t rights = {.val = vmread32(VMCS_GUEST_CS_ACCESS_RIGHTS)};
    return rights.fields.db != 0 ? VOP_32_BIT : VOP_16_BIT;    
}

inline void vadvance_guest_rip(void)
{
    int mode = vget_guest_mode();

    u64 rip = vmread(VMCS_GUEST_RIP) + 
              vmread(VMCS_RO_VMEXIT_INSTRUCTION_LENGTH);

    switch (mode) {
    
        case VOP_16_BIT:
            rip &= 0xffff;
            break;

        case VOP_32_BIT:
            rip &= 0xffffffff;
            break;

        case VOP_64_BIT:
            break;

        default:
            VBUG_ON(true);
            break;

    }

    __vmwrite(VMCS_GUEST_RIP, rip);
}

inline bool vmwrite_adjusted(u32 msr, u64 field, u64 val)
{
    u64 cap = __rdmsrl(msr);
    
    val |= (cap & 0xffffffff);
    val &= (cap >> 32);

    return __vmwrite(field, val);
}

inline cr0_t vadjust_cr0(cr0_t cr0)
{
    cr0.val |= __rdmsrl(IA32_VMX_CR0_FIXED0);
    cr0.val &= __rdmsrl(IA32_VMX_CR0_FIXED1);

    return cr0;
}

inline cr4_t vadjust_cr4(cr4_t cr4)
{
    cr4.val |= __rdmsrl(IA32_VMX_CR4_FIXED0);
    cr4.val &= __rdmsrl(IA32_VMX_CR4_FIXED1);

    return cr4;
}

u64 vlapic_read(u32 offset);
void vlapic_write(u32 offset, u64 val);

inline void vset_lapic_oneshot(u8 vector, u64 ticks, lapic_dcr_config_t dcr)
{
    lapic_timer_t timer = {
        .fields = {
            .vector = vector,
            .timer_mode = LAPIC_ONESHOT,
        }
    };

    vlapic_write(LAPIC_DCR_OFFSET, dcr);
    vlapic_write(LAPIC_INITIAL_COUNT_OFFSET, ticks);
    vlapic_write(LAPIC_TIMER_OFFSET, timer.val);
}

inline bool vis_lapic_irr_set(u8 vector)
{
    u32 idx = vector / 32;
    u32 bit_pos = vector % 32;
    
    u32 irr_offset = LAPIC_IRR_OFFSET + (idx * 16);
    return ((vlapic_read(irr_offset) >> bit_pos) & 1) != 0;
}

inline bool vis_lapic_isr_set(u8 vector)
{
    u32 idx = vector / 32;
    u32 bit_pos = vector % 32;
    
    u32 isr_offset = LAPIC_ISR_OFFSET + (idx * 16);
    return ((vlapic_read(isr_offset) >> bit_pos) & 1) != 0;
}

inline bool vis_lapic_oneshot_done(void)
{
    return (vlapic_read(LAPIC_CUR_COUNT_OFFSET) & 0xffffffff) == 0;
}

inline void vlapic_eoi(void)
{
    vlapic_write(LAPIC_EOI_OFFSET, 0);
}

inline void vinject_interrupt_external(u8 vector, bool nmi)
{
    interrupt_type_t type = vector == NMI && nmi ? INTERRUPT_TYPE_NMI : 
                                                  INTERRUPT_TYPE_EXTERNAL;
   
    vinject_interrupt(vector, type, false, 0, false, 0);
}

inline void vlapic_wait_delivery_complete(void)
{
    if (vthis_cpu_data()->arch_flags.support.fields.x2apic != 0)
        return;
    
    lapic_icr_low_t icr_low;

    do {
        icr_low.val = vlapic_read(LAPIC_ICR_LOW_OFFSET);
        cpu_relax();
    } while (icr_low.fields.delivery_status != 0);
}

inline void vlapic_send_ipi(u32 dest, u32 delivery_mode, u32 dest_mode, 
                            u32 dest_type, u32 vector)
{
    vlapic_wait_delivery_complete();

    lapic_icr_low_t icr_low = {
        .fields = {
            .vector = vector,
            .delivery_mode = delivery_mode,
            .level = LAPIC_ASSERT,
            .destination_mode = dest_mode,
            .trigger_mode = LAPIC_TRIGGER_EDGE,
            .destination_type = dest_type
        }
    };

    if (vthis_cpu_data()->arch_flags.support.fields.x2apic == 0)
        dest <<= 24;

    lapic_icr_high_t icr_high = {
        .fields = {
            .destination = dest
        }
    };

    u64 val = ((u64)icr_high.val << 32) | icr_low.val;

    vlapic_write(LAPIC_ICR_HIGH_OFFSET, val);
    vlapic_wait_delivery_complete();
}

inline u64 vgpr_val(struct vregs *vregs, u32 gpr)
{
    u64 val = 0;
    
    switch (gpr) {

        case RAX:
            val = vregs->regs.rax;
            break;

        case RCX:
            val = vregs->regs.rcx;
            break;

        case RDX:
            val = vregs->regs.rdx;
            break;

        case RBX:
            val = vregs->regs.rbx;
            break;

        case RSP:
            val = vmread(VMCS_GUEST_RSP);
            break;

        case RBP:
            val = vregs->regs.rbp;
            break;

        case RSI:
            val = vregs->regs.rsi;
            break;

        case RDI:
            val = vregs->regs.rdi;
            break;

        case R8:
            val = vregs->regs.r8;
            break;

        case R9:
            val = vregs->regs.r9;
            break;

        case R10:
            val = vregs->regs.r10;
            break;

        case R11:
            val = vregs->regs.r11;
            break;

        case R12:
            val = vregs->regs.r12;
            break;

        case R13:
            val = vregs->regs.r13;
            break;

        case R14:
            val = vregs->regs.r14;
            break;
        
        case R15:
            val = vregs->regs.r15;
            break;

        default:
            VBUG_ON(true);
            break;
    }

    return val;
}

inline void vqueue_advance_guest(void)
{
    struct vcpu *current = vcurrent_vcpu();
    current->voperation_queue.pending.fields.should_advance = 1;
}

inline bool vis_guest_cpl0(int *mode)
{
    cr0_t cr0 = {.val = vmread(VMCS_GUEST_CR0)};
    if (cr0.fields.pe == 0) {
        *mode = VOP_16_BIT;
        return true;
    }

    access_rights_t ar = {.val = vmread32(VMCS_GUEST_CS_ACCESS_RIGHTS)};
    if (ar.fields.dpl != 0)
        return false;

    ia32_efer_t efer = {.val = vmread(VMCS_GUEST_IA32_EFER)};
    if (efer.fields.lma != 0) {

        access_rights_t rights = {.val = vmread32(VMCS_GUEST_CS_ACCESS_RIGHTS)};

        *mode = rights.fields.longmode != 0 ? VOP_64_BIT :
                rights.fields.db != 0 ? VOP_32_BIT : VOP_16_BIT;

        return true;
    }

    rflags_t flags = {.val = vmread(VMCS_GUEST_RFLAGS)};
    if (flags.fields.vm != 0)
        return false;

    access_rights_t rights = {.val = vmread32(VMCS_GUEST_CS_ACCESS_RIGHTS)};

    *mode = rights.fields.db != 0 ? VOP_32_BIT : VOP_16_BIT;
    return true;
}

inline void vqueue_inject_gp0(void)
{
    struct vcpu *current = vcurrent_vcpu();
    
    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&current->visr_pending.lock, &node);
    current->visr_pending.delivery.fields.gp0_pending = 1;
    vmcs_unlock_isr_restore(&current->visr_pending.lock, &node);
}

inline void vqueue_inject_ud(void)
{
    struct vcpu *current = vcurrent_vcpu();
    
    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&current->visr_pending.lock, &node);
    current->visr_pending.delivery.fields.ud_pending = 1;
    vmcs_unlock_isr_restore(&current->visr_pending.lock, &node);
}

inline void vqueue_inject_db(interrupt_type_t int_type)
{
    struct vcpu *current = vcurrent_vcpu();
    
    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&current->visr_pending.lock, &node);

    current->visr_pending.delivery.fields.db_pending = 1;
    current->visr_pending.delivery.fields.int_type = int_type;

    vmcs_unlock_isr_restore(&current->visr_pending.lock, &node);
}

inline void vqueue_inject_ac0(void)
{
    struct vcpu *current = vcurrent_vcpu();
    
    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&current->visr_pending.lock, &node);
    current->visr_pending.delivery.fields.ac0_pending = 1;
    vmcs_unlock_isr_restore(&current->visr_pending.lock, &node);
}

inline void vqueue_vmwrite_cr4(cr4_t cr4)
{
    struct vcpu *current = vcurrent_vcpu();
    current->voperation_queue.pending.fields.vmwrite_cr4 = 1;
    current->voperation_queue.cr4 = cr4;    
}

inline void vout_nmi(struct vcpu *vcpu)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vcpu->visr_pending.lock, &node);
    vcpu->visr_pending.delivery.fields.in_nmi = 0;
    vmcs_unlock_isr_restore(&vcpu->visr_pending.lock, &node);
}

inline void vset_intl(struct vcpu *vcpu, intl_t intl)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vcpu->visr_pending.lock, &node);
    vcpu->visr_pending.delivery.fields.intl = intl.val;
    vmcs_unlock_isr_restore(&vcpu->visr_pending.lock, &node);
}

inline bool vis_nmis_blocked(void)
{
    guest_interruptibility_state_t state = {
        .val = vmread(VMCS_GUEST_INTERRUPTIBILITY_STATE)
    };
    
    return state.fields.mov_ss_blocking != 0 || state.fields.nmi_blocking != 0;
}

inline bool vis_external_interrupts_blocked(void)
{
    rflags_t flags = {
        .val = vmread(VMCS_GUEST_RFLAGS)
    };

    if (flags.fields._if == 0)
        return true;

    guest_interruptibility_state_t state = {
        .val = vmread(VMCS_GUEST_INTERRUPTIBILITY_STATE)
    };

    return state.fields.sti_blocking != 0 || state.fields.mov_ss_blocking != 0;
}

void vtrap_msr_write(struct vcpu *vcpu, u32 msr);
void vtrap_msr_read(struct vcpu *vcpu, u32 msr);
void vtrap_msr(struct vcpu *vcpu, u32 msr);

void __vemu_set_interrupt_pending(struct vcpu *vcpu, u8 vector, bool nmi);
int vemu_set_interrupt_pending(struct vcpu *vcpu, u8 vector, bool nmi);

bool vemu_is_interrupt_pending(struct vcpu *vcpu, u8 vector, bool nmi);

int __vemu_unpause_vcpu(struct vcpu *vcpu, bool inject, u8 vector, bool nmi);
int vemu_unpause_vcpu_local(u32 processor_id, bool inject, u8 vector, bool nmi);

int vemu_unpause_vcpu_far(u8 target_vid, u32 processor_id, bool inject, 
                          u8 vector, bool nmi);

void __vemu_inject_external_interrupt(struct vcpu *vcpu, u8 vector, bool nmi);
int vemu_inject_external_interrupt_local(u32 processor_id, u8 vector, bool nmi);

int vemu_inject_external_interrupt_far(u8 target_vid, u32 processor_id, 
                                       u32 vector, bool nmi);
                                       
int vemu_tlb_invalidate(u8 target_vid);

int vemu_alter_vcpu(u8 vid, u32 processor_id, bool alter_ticks, u32 ticks, 
                    bool alter_criticality, u8 criticality);

vcpu_state_t __vemu_read_vcpu_state(struct vcpu *vcpu);
int vemu_read_vcpu_state_local(u32 processor_id);
int vemu_read_vcpu_state_far(u8 target_vid, u32 processor_id);

int vemu_set_route(u8 target_vid, u8 sender_vid, vroute_type_t route_type, 
                   bool allow);

int vemu_set_vcpu_subscription_perms(u8 vid, u32 processor_id, u8 vector,
                                     bool allow);

int vemu_set_criticality_perm(u8 vid, u32 physical_processor_id, 
                              vcriticality_perm_t perm); 

int vemu_read_criticality_level(u8 requester_vid, u32 physical_processor_id);

int vemu_write_criticality_level(u8 requester_vid, u32 physical_processor_id, 
                                 u8 criticality_level);   
                                 
void __vemu_pv_spin_kick(struct vcpu *vcpu);
int vemu_pv_spin_kick_local(u32 processor_id);
int vemu_pv_spin_kick_far(u8 target_vid, u32 processor_id);

#endif