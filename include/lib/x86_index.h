#ifndef _X86_INDEX_H_
#define _X86_INDEX_H_

#include <types.h>
#include <compiler.h>

/* vmcs encoding */

typedef enum
{
    full,
    high,
} vmcs_access_t;

typedef enum
{
    control,
    vm_exit,
    guest_state,
    host_state,
} vmcs_type_t;

typedef enum
{
    word,
    qword,
    dword,
    natural,
} vmcs_width_t;

#define ENC_VMCS_COMPONENT(access, index, type, width) \
    (unsigned)((unsigned short)(access) |              \
               ((unsigned short)(index) << 1) |        \
               ((unsigned short)(type) << 10) |        \
               ((unsigned short)(width) << 13))
#define ENC_VMCS_COMPONENT_FULL(index, type, width) \
    ENC_VMCS_COMPONENT(full, index, type, width)
#define ENC_VMCS_COMPONENT_HIGH(index, type, width) \
    ENC_VMCS_COMPONENT(high, index, type, width)
#define ENC_VMCS_COMPONENT_FULL_16(index, type) \
    ENC_VMCS_COMPONENT_FULL(index, type, word)
#define ENC_VMCS_COMPONENT_FULL_32(index, type) \
    ENC_VMCS_COMPONENT_FULL(index, type, dword)
#define ENC_VMCS_COMPONENT_FULL_64(index, type) \
    ENC_VMCS_COMPONENT_FULL(index, type, qword)
#define ENC_VMCS_COMPONENT_FULL_NAT(index, type) \
    ENC_VMCS_COMPONENT_FULL(index, type, natural)
#define ENC_VMCS_COMPONENT_HIGH_16(index, type) \
    ENC_VMCS_COMPONENT_HIGH(index, type, word)
#define ENC_VMCS_COMPONENT_HIGH_32(index, type) \
    ENC_VMCS_COMPONENT_HIGH(index, type, dword)
#define ENC_VMCS_COMPONENT_HIGH_64(index, type) \
    ENC_VMCS_COMPONENT_HIGH(index, type, qword)
#define ENC_VMCS_COMPONENT_HIGH_NAT(index, type) \
    ENC_VMCS_COMPONENT_HIGH(index, type, natural)

// 16-bit fields (control)
#define VMCS_CTRL_VPID \
    ENC_VMCS_COMPONENT_FULL_16(0, control)
#define VMCS_CTRL_POSTED_INTERRUPT_NOTIFICATION_VECTOR \
    ENC_VMCS_COMPONENT_FULL_16(1, control)
#define VMCS_CTRL_EPTP_INDEX \
    ENC_VMCS_COMPONENT_FULL_16(2, control)
#define VMCS_CTRL_HLAT_PREFIX_SIZE \
    ENC_VMCS_COMPONENT_FULL_16(3, control)
#define VMCS_CTRL_LAST_PID_POINTER_INDEX \
    ENC_VMCS_COMPONENT_FULL_16(4, control)

// 16-bit fields (guest-state)
#define VMCS_GUEST_ES_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(0, guest_state)
#define VMCS_GUEST_CS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(1, guest_state)
#define VMCS_GUEST_SS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(2, guest_state)
#define VMCS_GUEST_DS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(3, guest_state)
#define VMCS_GUEST_FS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(4, guest_state)
#define VMCS_GUEST_GS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(5, guest_state)
#define VMCS_GUEST_LDTR_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(6, guest_state)
#define VMCS_GUEST_TR_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(7, guest_state)
#define VMCS_GUEST_INTERRUPT_STATUS \
    ENC_VMCS_COMPONENT_FULL_16(8, guest_state)
#define VMCS_GUEST_PML_INDEX \
    ENC_VMCS_COMPONENT_FULL_16(9, guest_state)
#define VMCS_GUEST_UINV \
    ENC_VMCS_COMPONENT_FULL_16(10, guest_state)

// 16-bit fields (host-state)
#define VMCS_HOST_ES_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(0, host_state)
#define VMCS_HOST_CS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(1, host_state)
#define VMCS_HOST_SS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(2, host_state)
#define VMCS_HOST_DS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(3, host_state)
#define VMCS_HOST_FS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(4, host_state)
#define VMCS_HOST_GS_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(5, host_state)
#define VMCS_HOST_TR_SELECTOR \
    ENC_VMCS_COMPONENT_FULL_16(6, host_state)

// 64-bit fields (control)
#define VMCS_CTRL_IO_BITMAP_A \
    ENC_VMCS_COMPONENT_FULL_64(0, control)
#define VMCS_CTRL_IO_BITMAP_B \
    ENC_VMCS_COMPONENT_FULL_64(1, control)
#define VMCS_CTRL_MSR_BITMAPS \
    ENC_VMCS_COMPONENT_FULL_64(2, control)
#define VMCS_CTRL_VMEXIT_MSR_STORE_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(3, control)
#define VMCS_CTRL_VMEXIT_MSR_LOAD_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(4, control)
#define VMCS_CTRL_VMENTRY_MSR_LOAD_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(5, control)
#define VMCS_CTRL_EXECUTIVE_VMCS_POINTER \
    ENC_VMCS_COMPONENT_FULL_64(6, control)
#define VMCS_CTRL_PML_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(7, control)
#define VMCS_CTRL_TSC_OFFSET \
    ENC_VMCS_COMPONENT_FULL_64(8, control)
#define VMCS_CTRL_VIRTUAL_APIC_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(9, control)
#define VMCS_CTRL_APIC_ACCESS_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(10, control)
#define VMCS_CTRL_POSTED_INTERRUPT_DESCRIPTOR_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(11, control)
#define VMCS_CTRL_VM_FUNCTION_CONTROLS \
    ENC_VMCS_COMPONENT_FULL_64(12, control)
#define VMCS_CTRL_EPTP \
    ENC_VMCS_COMPONENT_FULL_64(13, control)
#define VMCS_CTRL_EOI_EXIT_BITMAP_0 \
    ENC_VMCS_COMPONENT_FULL_64(14, control)
#define VMCS_CTRL_EOI_EXIT_BITMAP_1 \
    ENC_VMCS_COMPONENT_FULL_64(15, control)
#define VMCS_CTRL_EOI_EXIT_BITMAP_2 \
    ENC_VMCS_COMPONENT_FULL_64(16, control)
#define VMCS_CTRL_EOI_EXIT_BITMAP_3 \
    ENC_VMCS_COMPONENT_FULL_64(17, control)
#define VMCS_CTRL_EPTP_LIST_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(18, control)
#define VMCS_CTRL_VMREAD_BITMAP_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(19, control)
#define VMCS_CTRL_VMWRITE_BITMAP_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(20, control)
#define VMCS_CTRL_VIRT_EXCEPTION_INFORMATION_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(21, control)
#define VMCS_CTRL_XSS_EXITING_BITMAP \
    ENC_VMCS_COMPONENT_FULL_64(22, control)
#define VMCS_CTRL_ENCLS_EXITING_BITMAP \
    ENC_VMCS_COMPONENT_FULL_64(23, control)
#define VMCS_CTRL_SUB_PAGE_PERMISSION_TABLE_POINTER \
    ENC_VMCS_COMPONENT_FULL_64(24, control)
#define VMCS_CTRL_TSC_MULTIPLIER \
    ENC_VMCS_COMPONENT_FULL_64(25, control)
#define VMCS_CTRL_PROCBASED_CTLS3 \
    ENC_VMCS_COMPONENT_FULL_64(26, control)
#define VMCS_CTRL_ENCLV_EXITING_BITMAP \
    ENC_VMCS_COMPONENT_FULL_64(27, control)
#define VMCS_CTRL_LOW_PASID_DIRECTORY_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(28, control)
#define VMCS_CTRL_HIGH_PASID_DIRECTORY_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(29, control)
#define VMCS_CTRL_SHARED_EPT_POINTER \
    ENC_VMCS_COMPONENT_FULL_64(30, control)
#define VMCS_CTRL_PCONFIG_EXITING_BITMAP \
    ENC_VMCS_COMPONENT_FULL_64(31, control)
#define VMCS_CTRL_HLATP \
    ENC_VMCS_COMPONENT_FULL_64(32, control)
#define VMCS_CTRL_PID_POINTER_TABLE_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(33, control)
#define VMCS_CTRL_SECONDARY_VMEXIT_CONTROLS \
    ENC_VMCS_COMPONENT_FULL_64(34, control)
#define VMCS_CTRL_IA32_SPEC_CTRL_MASK \
    ENC_VMCS_COMPONENT_FULL_64(35, control)
#define VMCS_CTRL_IA32_SPEC_CTRL_SHADOW \
    ENC_VMCS_COMPONENT_FULL_64(36, control)

// 64-bit fields (read-only data)
#define VMCS_RO_GUEST_PHYSICAL_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_64(0, vm_exit)

// 64-bit fields (guest-state)
#define VMCS_GUEST_VMCS_LINK_POINTER \
    ENC_VMCS_COMPONENT_FULL_64(0, guest_state)
#define VMCS_GUEST_IA32_DEBUGCTL \
    ENC_VMCS_COMPONENT_FULL_64(1, guest_state)
#define VMCS_GUEST_IA32_PAT \
    ENC_VMCS_COMPONENT_FULL_64(2, guest_state)
#define VMCS_GUEST_IA32_EFER \
    ENC_VMCS_COMPONENT_FULL_64(3, guest_state)
#define VMCS_GUEST_IA32_PERF_GLOBAL_CTRL \
    ENC_VMCS_COMPONENT_FULL_64(4, guest_state)
#define VMCS_GUEST_PDPTE0 \
    ENC_VMCS_COMPONENT_FULL_64(5, guest_state)
#define VMCS_GUEST_PDPTE1 \
    ENC_VMCS_COMPONENT_FULL_64(6, guest_state)
#define VMCS_GUEST_PDPTE2 \
    ENC_VMCS_COMPONENT_FULL_64(7, guest_state)
#define VMCS_GUEST_PDPTE3 \
    ENC_VMCS_COMPONENT_FULL_64(8, guest_state)
#define VMCS_GUEST_IA32_BNDCFGS \
    ENC_VMCS_COMPONENT_FULL_64(9, guest_state)
#define VMCS_GUEST_IA32_RTIT_CTL \
    ENC_VMCS_COMPONENT_FULL_64(10, guest_state)
#define VMCS_GUEST_IA32_LBR_CTL \
    ENC_VMCS_COMPONENT_FULL_64(11, guest_state)
#define VMCS_GUEST_IA32_PKRS \
    ENC_VMCS_COMPONENT_FULL_64(12, guest_state)

// 64-bit fields (host-state)
#define VMCS_HOST_IA32_PAT \
    ENC_VMCS_COMPONENT_FULL_64(0, host_state)
#define VMCS_HOST_IA32_EFER \
    ENC_VMCS_COMPONENT_FULL_64(1, host_state)
#define VMCS_HOST_IA32_PERF_GLOBAL_CTRL \
    ENC_VMCS_COMPONENT_FULL_64(2, host_state)
#define VMCS_HOST_IA32_PKRS \
    ENC_VMCS_COMPONENT_FULL_64(3, host_state)

// 32-bit fields (control)
#define VMCS_CTRL_PINBASED_CONTROLS \
    ENC_VMCS_COMPONENT_FULL_32(0, control)
#define VMCS_CTRL_PROCBASED_CTLS \
    ENC_VMCS_COMPONENT_FULL_32(1, control)
#define VMCS_CTRL_EXCEPTION_BITMAP \
    ENC_VMCS_COMPONENT_FULL_32(2, control)
#define VMCS_CTRL_PAGE_FAULT_ERROR_CODE_MASK \
    ENC_VMCS_COMPONENT_FULL_32(3, control)
#define VMCS_CTRL_PAGE_FAULT_ERROR_CODE_MATCH \
    ENC_VMCS_COMPONENT_FULL_32(4, control)
#define VMCS_CTRL_CR3_TARGET_COUNT \
    ENC_VMCS_COMPONENT_FULL_32(5, control)
#define VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS \
    ENC_VMCS_COMPONENT_FULL_32(6, control)
#define VMCS_CTRL_VMEXIT_MSR_STORE_COUNT \
    ENC_VMCS_COMPONENT_FULL_32(7, control)
#define VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT \
    ENC_VMCS_COMPONENT_FULL_32(8, control)
#define VMCS_CTRL_VMENTRY_CONTROLS \
    ENC_VMCS_COMPONENT_FULL_32(9, control)
#define VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT \
    ENC_VMCS_COMPONENT_FULL_32(10, control)
#define VMCS_CTRL_VMENTRY_INTERRUPT_INFO \
    ENC_VMCS_COMPONENT_FULL_32(11, control)
#define VMCS_CTRL_VMENTRY_INTERRUPT_ERROR_CODE \
    ENC_VMCS_COMPONENT_FULL_32(12, control)
#define VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH \
    ENC_VMCS_COMPONENT_FULL_32(13, control)
#define VMCS_CTRL_TPR_THRESHOLD \
    ENC_VMCS_COMPONENT_FULL_32(14, control)
#define VMCS_CTRL_PROCBASED_CTLS2 \
    ENC_VMCS_COMPONENT_FULL_32(15, control)
#define VMCS_CTRL_PLE_GAP \
    ENC_VMCS_COMPONENT_FULL_32(16, control)
#define VMCS_CTRL_PLE_WINDOW \
    ENC_VMCS_COMPONENT_FULL_32(17, control)
#define VMCS_CTRL_INSTRUCTION_TIMEOUT_CONTROL \
    ENC_VMCS_COMPONENT_FULL_32(18, control)

// 32-bit fields (read-only data)
#define VMCS_RO_VM_INSTRUCTION_ERROR \
    ENC_VMCS_COMPONENT_FULL_32(0, vm_exit)
#define VMCS_RO_EXIT_REASON \
    ENC_VMCS_COMPONENT_FULL_32(1, vm_exit)
#define VMCS_RO_VMEXIT_INTERRUPT_INFO \
    ENC_VMCS_COMPONENT_FULL_32(2, vm_exit)
#define VMCS_RO_VMEXIT_INTERRUPT_ERROR_CODE \
    ENC_VMCS_COMPONENT_FULL_32(3, vm_exit)
#define VMCS_RO_IDT_VECTORING_INFO_FIELD \
    ENC_VMCS_COMPONENT_FULL_32(4, vm_exit)
#define VMCS_RO_IDT_VECTORING_ERROR_CODE \
    ENC_VMCS_COMPONENT_FULL_32(5, vm_exit)
#define VMCS_RO_VMEXIT_INSTRUCTION_LENGTH \
    ENC_VMCS_COMPONENT_FULL_32(6, vm_exit)
#define VMCS_RO_VMEXIT_INSTRUCTION_INFO \
    ENC_VMCS_COMPONENT_FULL_32(7, vm_exit)

// 32-bit fields (guest-state)
#define VMCS_GUEST_ES_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(0, guest_state)
#define VMCS_GUEST_CS_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(1, guest_state)
#define VMCS_GUEST_SS_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(2, guest_state)
#define VMCS_GUEST_DS_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(3, guest_state)
#define VMCS_GUEST_FS_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(4, guest_state)
#define VMCS_GUEST_GS_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(5, guest_state)
#define VMCS_GUEST_LDTR_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(6, guest_state)
#define VMCS_GUEST_TR_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(7, guest_state)
#define VMCS_GUEST_GDTR_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(8, guest_state)
#define VMCS_GUEST_IDTR_LIMIT \
    ENC_VMCS_COMPONENT_FULL_32(9, guest_state)
#define VMCS_GUEST_ES_ACCESS_RIGHTS \
    ENC_VMCS_COMPONENT_FULL_32(10, guest_state)
#define VMCS_GUEST_CS_ACCESS_RIGHTS \
    ENC_VMCS_COMPONENT_FULL_32(11, guest_state)
#define VMCS_GUEST_SS_ACCESS_RIGHTS \
    ENC_VMCS_COMPONENT_FULL_32(12, guest_state)
#define VMCS_GUEST_DS_ACCESS_RIGHTS \
    ENC_VMCS_COMPONENT_FULL_32(13, guest_state)
#define VMCS_GUEST_FS_ACCESS_RIGHTS \
    ENC_VMCS_COMPONENT_FULL_32(14, guest_state)
#define VMCS_GUEST_GS_ACCESS_RIGHTS \
    ENC_VMCS_COMPONENT_FULL_32(15, guest_state)
#define VMCS_GUEST_LDTR_ACCESS_RIGHTS \
    ENC_VMCS_COMPONENT_FULL_32(16, guest_state)
#define VMCS_GUEST_TR_ACCESS_RIGHTS \
    ENC_VMCS_COMPONENT_FULL_32(17, guest_state)
#define VMCS_GUEST_INTERRUPTIBILITY_STATE \
    ENC_VMCS_COMPONENT_FULL_32(18, guest_state)
#define VMCS_GUEST_ACTIVITY_STATE \
    ENC_VMCS_COMPONENT_FULL_32(19, guest_state)
#define VMCS_GUEST_SMBASE \
    ENC_VMCS_COMPONENT_FULL_32(20, guest_state)
#define VMCS_GUEST_IA32_SYSENTER_CS \
    ENC_VMCS_COMPONENT_FULL_32(21, guest_state)
#define VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE \
    ENC_VMCS_COMPONENT_FULL_32(23, guest_state)

// 32-bit fields (host-state)
#define VMCS_HOST_IA32_SYSENTER_CS \
    ENC_VMCS_COMPONENT_FULL_32(0, host_state)

// natural-width fields (control)
#define VMCS_CTRL_CR0_GUEST_HOST_MASK \
    ENC_VMCS_COMPONENT_FULL_NAT(0, control)
#define VMCS_CTRL_CR4_GUEST_HOST_MASK \
    ENC_VMCS_COMPONENT_FULL_NAT(1, control)
#define VMCS_CTRL_CR0_READ_SHADOW \
    ENC_VMCS_COMPONENT_FULL_NAT(2, control)
#define VMCS_CTRL_CR4_READ_SHADOW \
    ENC_VMCS_COMPONENT_FULL_NAT(3, control)
#define VMCS_CTRL_CR3_TARGET_VALUE_0 \
    ENC_VMCS_COMPONENT_FULL_NAT(4, control)
#define VMCS_CTRL_CR3_TARGET_VALUE_1 \
    ENC_VMCS_COMPONENT_FULL_NAT(5, control)
#define VMCS_CTRL_CR3_TARGET_VALUE_2 \
    ENC_VMCS_COMPONENT_FULL_NAT(6, control)
#define VMCS_CTRL_CR3_TARGET_VALUE_3 \
    ENC_VMCS_COMPONENT_FULL_NAT(7, control)

// natural-width fields (read-only data)
#define VMCS_RO_EXIT_QUALIFICATION \
    ENC_VMCS_COMPONENT_FULL_NAT(0, vm_exit)
#define VMCS_RO_IO_RCX \
    ENC_VMCS_COMPONENT_FULL_NAT(1, vm_exit)
#define VMCS_RO_IO_RSI \
    ENC_VMCS_COMPONENT_FULL_NAT(2, vm_exit)
#define VMCS_RO_IO_RDI \
    ENC_VMCS_COMPONENT_FULL_NAT(3, vm_exit)
#define VMCS_RO_IO_RIP \
    ENC_VMCS_COMPONENT_FULL_NAT(4, vm_exit)
#define VMCS_RO_GUEST_LINEAR_ADDRESS \
    ENC_VMCS_COMPONENT_FULL_NAT(5, vm_exit)

// natural-width fields (guest-state)
#define VMCS_GUEST_CR0 \
    ENC_VMCS_COMPONENT_FULL_NAT(0, guest_state)
#define VMCS_GUEST_CR3 \
    ENC_VMCS_COMPONENT_FULL_NAT(1, guest_state)
#define VMCS_GUEST_CR4 \
    ENC_VMCS_COMPONENT_FULL_NAT(2, guest_state)
#define VMCS_GUEST_ES_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(3, guest_state)
#define VMCS_GUEST_CS_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(4, guest_state)
#define VMCS_GUEST_SS_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(5, guest_state)
#define VMCS_GUEST_DS_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(6, guest_state)
#define VMCS_GUEST_FS_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(7, guest_state)
#define VMCS_GUEST_GS_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(8, guest_state)
#define VMCS_GUEST_LDTR_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(9, guest_state)
#define VMCS_GUEST_TR_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(10, guest_state)
#define VMCS_GUEST_GDTR_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(11, guest_state)
#define VMCS_GUEST_IDTR_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(12, guest_state)
#define VMCS_GUEST_DR7 \
    ENC_VMCS_COMPONENT_FULL_NAT(13, guest_state)
#define VMCS_GUEST_RSP \
    ENC_VMCS_COMPONENT_FULL_NAT(14, guest_state)
#define VMCS_GUEST_RIP \
    ENC_VMCS_COMPONENT_FULL_NAT(15, guest_state)
#define VMCS_GUEST_RFLAGS \
    ENC_VMCS_COMPONENT_FULL_NAT(16, guest_state)
#define VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS \
    ENC_VMCS_COMPONENT_FULL_NAT(17, guest_state)
#define VMCS_GUEST_IA32_SYSENTER_ESP \
    ENC_VMCS_COMPONENT_FULL_NAT(18, guest_state)
#define VMCS_GUEST_IA32_SYSENTER_EIP \
    ENC_VMCS_COMPONENT_FULL_NAT(19, guest_state)
#define VMCS_GUEST_IA32_S_CET \
    ENC_VMCS_COMPONENT_FULL_NAT(20, guest_state)
#define VMCS_GUEST_SSP \
    ENC_VMCS_COMPONENT_FULL_NAT(21, guest_state)
#define VMCS_GUEST_IA32_INTERRUPT_SSP_TABLE_ADDR \
    ENC_VMCS_COMPONENT_FULL_NAT(22, guest_state)

// natural-width fields (host-state)
#define VMCS_HOST_CR0 \
    ENC_VMCS_COMPONENT_FULL_NAT(0, host_state)
#define VMCS_HOST_CR3 \
    ENC_VMCS_COMPONENT_FULL_NAT(1, host_state)
#define VMCS_HOST_CR4 \
    ENC_VMCS_COMPONENT_FULL_NAT(2, host_state)
#define VMCS_HOST_FS_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(3, host_state)
#define VMCS_HOST_GS_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(4, host_state)
#define VMCS_HOST_TR_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(5, host_state)
#define VMCS_HOST_GDTR_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(6, host_state)
#define VMCS_HOST_IDTR_BASE \
    ENC_VMCS_COMPONENT_FULL_NAT(7, host_state)
#define VMCS_HOST_IA32_SYSENTER_ESP \
    ENC_VMCS_COMPONENT_FULL_NAT(8, host_state)
#define VMCS_HOST_IA32_SYSENTER_EIP \
    ENC_VMCS_COMPONENT_FULL_NAT(9, host_state)
#define VMCS_HOST_RSP \
    ENC_VMCS_COMPONENT_FULL_NAT(10, host_state)
#define VMCS_HOST_RIP \
    ENC_VMCS_COMPONENT_FULL_NAT(11, host_state)
#define VMCS_HOST_IA32_S_CET \
    ENC_VMCS_COMPONENT_FULL_NAT(12, host_state)
#define VMCS_HOST_SSP \
    ENC_VMCS_COMPONENT_FULL_NAT(13, host_state)
#define VMCS_HOST_IA32_INTERRUPT_SSP_TABLE_ADDR \
    ENC_VMCS_COMPONENT_FULL_NAT(14, host_state)

/* data structures */

#define IDT_RESET_BASE 0
#define IDT_RESET_LIMIT 0xffff

#define GDT_RESET_BASE 0
#define GDT_RESET_LIMIT 0xffff

#define IOAPIC_REG_ID 0
#define IOAPIC_REG_VERSION 1
#define IOAPIC_REG_ARBITRATION_PRIORITY 2
#define IOAPIC_REG_REDIRECTION_SIZE 2
#define IOAPIC_REG_REDIRECTION_MIN 0x10
#define IOAPIC_REG_REDIRECTION_MAX 0x3f

#define IOAPIC_REG_OFFSET 0
#define IOAPIC_DATA_OFFSET 16

typedef union
{
    u64 val;
    struct
    {
        u64 cf : 1;
        u64 reserved0 : 1;
        u64 pf : 1;
        u64 reserved1 : 1;
        u64 af : 1;
        u64 reserved2 : 1;
        u64 zf : 1;
        u64 sf : 1;
        u64 tf : 1;
        u64 _if : 1; 
        u64 df : 1;
        u64 of : 1;
        u64 iopl : 2;
        u64 nt : 1;
        u64 reserved3 : 1;
        u64 rf : 1;
        u64 vm : 1;
        u64 ac : 1;
        u64 vif : 1;
        u64 vip : 1;
        u64 id : 1;
        u64 reserved4 : 42;
    } fields;
} rflags_t;

#define DEFAULT_FCW 0x037f
#define DEFAULT_MXCSR 0x1f80

struct fxsave32 
{
    u16 fcw;
    u16 fsw;
    u8 ftw;
    u8 reserved1;
    u16 fop;
    u32 fip;
    u16 fcs;
    u16 reserved2;
    u32 fdp;
    u16 fds;
    u16 reserved3;
    u32 mxcsr;
    u32 mxcsr_mask;

    struct 
    {
        u64 mantissa;
        u16 exponent;
        u8 reserved[6];
    } st_mm[8];
    
    struct 
    {
        u64 low;
        u64 high;
    } xmm[8];

    u8 reserved4[224];
} __packed;

struct fxsave64 
{
    u16 fcw;
    u16 fsw;
    u8 ftw;
    u8 reserved1;
    u16 fop;
    u64 fip;
    u64 fdp;
    u32 mxcsr;
    u32 mxcsr_mask;

    struct 
    {
        u64 mantissa;
        u16 exponent;
        u8 reserved[6];
    } st_mm[8];

    struct 
    {
        u64 low;
        u64 high;
    } xmm[16];

    u8 reserved2[96];
} __packed;

#define IOAPIC_DEST_MODE_PHYSICAL 0
#define IOAPIC_DEST_MODE_LOGICAL 1

typedef union 
{
    u64 val;
    struct 
    {
        u64 vector : 8;
        u64 delivery_mode : 3;
        u64 destination_mode : 1;
        u64 delivery_status : 1;
        u64 polarity : 1;
        u64 int_received : 1;
        u64 trigger_mode : 1;
        u64 interrupt_mask : 1;
        u64 reserved0 : 39;
        u64 dest : 8;
    } fields;
} ioapic_redirection_entry_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 delivery_mode : 3;
        u32 destination_mode : 1;
        u32 delivery_status : 1;
        u32 polarity : 1;
        u32 int_received : 1;
        u32 trigger_mode : 1;
        u32 interrupt_mask : 1;
        u32 reserved0 : 15;
    } fields;
} ioapic_redirection_entry_low_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 24;
        u32 dest : 8;
    } fields;
} ioapic_redirection_entry_high_t;

struct ioapic
{
    u32 reg;
    u32 pad[3];
    u32 data;
} __packed;
SIZE_ASSERT(struct ioapic, 20);

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 24;
        u32 ioapic_id : 4;
        u32 reserved1 : 4;
    } fields; 
} ioapic_id_t;

typedef union 
{
    u32 val;
    struct
    {
        u32 version : 8;
        u32 reserved0 : 8;
        u32 redirection_entries_no : 8;
        u32 reserved1 : 8;
    } fields;
} ioapic_version_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 24;
        u32 arbitration_priority : 4;
        u32 reserved1 : 4;
    } fields;
} ioapic_arbitration_priority_t;

typedef enum 
{
    DM_NORMAL,
    DM_LOW_PRIORITY,
    DM_SMI,
    DM_NMI = 4,
    DM_INIT,
    DM_STARTUP,
    DM_EXTERNAL
} delivery_mode_t;

#define ioapic_reg_low(pin) (IOAPIC_REG_REDIRECTION_MIN + ((pin) * 2))
#define ioapic_reg_high(pin) (ioapic_reg_low((pin)) + 1)

#define LAPIC_PHYS_REG_BASE 0xFEE00000

#define LAPIC_ID_OFFSET 0x20
#define LAPIC_VERSION_OFFSET 0x30
#define LAPIC_TPR_OFFSET 0x80
#define LAPIC_APR_OFFSET 0x90
#define LAPIC_PPR_OFFSET 0xa0
#define LAPIC_EOI_OFFSET 0xb0
#define LAPIC_RRD_OFFSET 0xc0
#define LAPIC_LDR_OFFSET 0xd0
#define LAPIC_DFR_OFFSET 0xe0
#define LAPIC_SIVR_OFFSET 0xf0
#define LAPIC_ISR_OFFSET 0x100
#define LAPIC_TMR_OFFSET 0x180
#define LAPIC_IRR_OFFSET 0x200
#define LAPIC_ESR_OFFSET 0x280
#define LAPIC_CMCI_OFFSET 0x2f0
#define LAPIC_ICR_LOW_OFFSET 0x300
#define LAPIC_ICR_HIGH_OFFSET 0x310
#define LAPIC_TIMER_OFFSET 0x320
#define LAPIC_TSR_OFFSET 0x330
#define LAPIC_PMCR_OFFSET 0x340
#define LAPIC_LINT0_OFFSET 0x350
#define LAPIC_LINT1_OFFSET 0x360
#define LAPIC_LVT_ERROR_OFFSET 0x370
#define LAPIC_INITIAL_COUNT_OFFSET 0x380
#define LAPIC_CUR_COUNT_OFFSET 0x390
#define LAPIC_DCR_OFFSET 0x3e0

typedef union 
{
    u32 val;
    struct 
    {
        u32 version : 8;
        u32 reserved0 : 8;
        u32 max_lvt_entry : 8;
        u32 eoi_broadcast_suppression : 1;
        u32 reserved1 : 7;
    } fields;
} lapic_version_t;

typedef union
{
    u32 val;
    struct
    {
        u32 vector : 8;
        u32 lapic_enable : 1;
        u32 focused_processor_check : 1;
        u32 reserved0 : 2;
        u32 eoi_not_broadcast : 1;
        u32 reserved1 : 19;
    } fields;
} lapic_sivr_t;

typedef union
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 delivery_mode : 3;
        u32 reserved0 : 1;
        u32 delivery_status : 1;
        u32 reserved1 : 3;
        u32 mask : 1;
        u32 reserved2 : 15;
    } fields;
} lapic_cmci_t;

typedef union
{
    u32 val;
    struct
    {
        u32 vector : 8;
        u32 delivery_mode : 3;
        u32 destination_mode : 1;
        u32 delivery_status : 1;
        u32 reserved0 : 1;
        u32 level : 1;
        u32 trigger_mode : 1;
        u32 reserved1 : 2;
        u32 destination_type : 2;
        u32 reserved2 : 12;
    } fields;
} lapic_icr_low_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 destination : 32;
    } fields;
} lapic_icr_high_t;

typedef enum 
{
    SINGLE_TARGET,
    SELF_TARGET,
    ALL_TARGETS,
    OTHER_TARGETS,
} lapic_destination_type_t;

#define LAPIC_DEST_PHYSICAL 0
#define LAPIC_DEST_LOGICAL 1

#define LAPIC_DEASSERT 0
#define LAPIC_ASSERT 1

#define LAPIC_TRIGGER_EDGE 0
#define LAPIC_TRIGGER_LEVEL 1

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 reserved0 : 4;
        u32 delivery_status : 1;
        u32 reserved1 : 3;
        u32 mask : 1;
        u32 timer_mode : 2;
        u32 reserved2 : 13;
    } fields;
} lapic_timer_t;

typedef enum 
{
    LAPIC_ONESHOT,
    LAPIC_PERIODIC,
    LAPIC_TSC_DEADLINE
} lapic_timer_mode_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 send_checksum_error : 1;
        u32 receive_checksum_error : 1;
        u32 send_accept_error : 1;
        u32 receive_accept_error : 1;
        u32 redirect_ipi : 1;
        u32 send_illegal_vector : 1;
        u32 receive_illegal_vector : 1;
        u32 illegal_register_address : 1;
        u32 reserved0 : 24;
    } fields;
} lapic_esr_t;

typedef union
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 delivery_mode : 3;
        u32 reserved0 : 1;
        u32 delivery_status : 1;
        u32 reserved1 : 3;
        u32 mask : 1;
        u32 reserved2 : 15;
    } fields;
} lapic_tsr_t;

typedef union
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 delivery_mode : 3;
        u32 reserved0 : 1;
        u32 delivery_status : 1;
        u32 reserved1 : 3;
        u32 mask : 1;
        u32 reserved2 : 15;
    } fields;
} lapic_pmcr_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 delivery_mode : 3;
        u32 reserved0 : 1;
        u32 delivery_status : 1;
        u32 polarity : 1;
        u32 remote_irr : 1;
        u32 trigger_mode : 1;
        u32 mask : 1;
        u32 reserved1 : 15;
    } fields;
} lapic_lint_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 low : 2;
        u32 reserved0 : 1;
        u32 high : 1;
        u32 reserved1 : 28;
    } fields;
} lapic_dcr_t;

typedef enum 
{
    DIV_2   = 0b0000,
    DIV_4   = 0b0001,
    DIV_8   = 0b0010,
    DIV_16  = 0b0011,
    DIV_32  = 0b1000,
    DIV_64  = 0b1001,
    DIV_128 = 0b1010,
    DIV_1   = 0b1011
} lapic_dcr_config_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 24;
        u32 logical_apic_id : 8;
    } fields;
} lapic_ldr_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 28;
        u32 model : 4;
    } fields;
} lapic_dfr_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 sub_class : 4;
        u32 class : 4;
        u32 reserved0 : 24;
    } fields;
} lapic_apr_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 subclass : 4;
        u32 class : 4;
        u32 reserved0 : 28;
    } fields;
} lapic_tpr_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 subclass : 4;
        u32 class : 4;
        u32 reserved0 : 28;
    } fields;
} lapic_ppr_t;

#define PAGE_SHIFT_4KB 12
#define PAGE_SIZE_4KB (1 << PAGE_SHIFT_4KB)
#define PAGE_MASK_4KB (~(PAGE_SIZE_4KB - 1))

#define PAGE_SHIFT_2MB 21
#define PAGE_SIZE_2MB (1 << PAGE_SHIFT_2MB)
#define PAGE_MASK_2MB (~(PAGE_MASK_2MB - 1))

#define PAGE_SHIFT_1GB 30
#define PAGE_SIZE_1GB (1 << PAGE_SHIFT_1GB)

#define pml4_index_of(va) (((va) >> 39) & 0x1ff)
#define pdpt_index_of(va) (((va) >> PAGE_SHIFT_1GB) & 0x1ff)
#define pd_index_of(va) (((va) >> PAGE_SHIFT_2MB) & 0x1ff)
#define pt_index_of(va) (((va) >> PAGE_SHIFT_4KB) & 0x1ff)

#define offsetof_4k(va) ((va) & 0xfff)
#define offsetof_1gb(va) ((va) & 0x3fffffff)

/* va_t on intel is assumed to be for a 2MB page */
typedef union
{
    u64 val;
    struct 
    {
        u64 offset : 21;
        u64 pd : 9;
        u64 pdpt : 9;
        u64 pml4 : 9;
        u64 sign : 16;
    } fields;
} va_t;

typedef union
{
    u16 val;
    struct 
    {
        u16 rpl : 2;
        u16 ti : 1;
        u16 index : 13;
    } fields;
} selector_t;

struct descriptor_table64
{
    u16 limit;
    u64 base;
} __packed;
SIZE_ASSERT(struct descriptor_table64, 10);

#define TSS_AVAILABLE 0x9
#define TSS_BUSY 0xb

typedef union
{
    u32 val;
    struct
    {
        u32 segment_type : 4;
        u32 descriptor_type : 1;
        u32 dpl : 2;
        u32 p : 1;
        u32 reserved0 : 4;
        u32 avl : 1;
        u32 longmode : 1;
        u32 db : 1;
        u32 g : 1;
        u32 segment_unusable : 1;
        u32 reserved1 : 15;
    } fields;
} access_rights_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 limit_low : 16;
        u64 base_low : 16;
        u64 base_mid : 8;
        u64 segment_type : 4;
        u64 descriptor_type : 1;
        u64 dpl : 2;
        u64 present : 1;
        u64 limit_high : 4;
        u64 avl : 1;
        u64 long_mode : 1;
        u64 db : 1;
        u64 granularity : 1;
        u64 base_high : 8;
    } fields;
} gdt_descriptor32_t;

struct gdt_descriptor64
{
    union
    {
        u64 val;
        struct
        {
            u64 limit_low : 16;
            u64 base_low : 16;
            u64 base_mid : 8;
            u64 segment_type : 4;
            u64 descriptor_type : 1;
            u64 dpl : 2;
            u64 present : 1;
            u64 limit_high : 4;
            u64 avl : 1;
            u64 reserved1 : 1;
            u64 reserved2 : 1;
            u64 granularity : 1;
            u64 base_high : 8;
        } fields;
    } descriptor_lower;

    u32 base_upper;
    u32 reserved0;
} __packed;
SIZE_ASSERT(struct gdt_descriptor64, 16);

struct tss64 
{
    u32 reserved0;          
    u64 rsp0;               
    u64 rsp1;               
    u64 rsp2;               
    u64 reserved1;          
    u64 ist1;               
    u64 ist2;               
    u64 ist3;               
    u64 ist4;               
    u64 ist5;               
    u64 ist6;             
    u64 ist7;               
    u64 reserved2;          
    u16 reserved3;          
    u16 io_map_base;       
} __packed;
SIZE_ASSERT(struct tss64, 104);

typedef union
{
    u8 val;
    struct
    {
        u8 gate_type : 4;
        u8 reserved0 : 1;
        u8 dpl : 2;
        u8 present : 1;
    } fields;
} idt_desc_attr_t;

struct idt_descriptor32
{
    u16 offset1;
    selector_t selector;
    u8 reserved0;
    idt_desc_attr_t attr;
    u16 offset2;
} __packed;
SIZE_ASSERT(struct idt_descriptor32, 8);

struct idt_descriptor64
{
    u16 offset_low;
    selector_t selector;
    u8 ist;
    idt_desc_attr_t attr;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved0;
} __packed;
SIZE_ASSERT(struct idt_descriptor64, 16);

#define INTERRUPT_GATE64 0xe
#define TRAP_GATE64 0xf

typedef enum
{
    DIVIDE_ERROR,
    DEBUG_EXCEPTION,
    NMI,
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
    VECTOR15,
    MATH_FAULT,
    ALIGNMENT_CHECK,
    MACHINE_CHECK,
    SIMD_FLOATING_POINT_EXCEPTION,
    VIRTUALISATION_EXCEPTION,
    CONTROL_PROTECTION_EXCEPTION,
    VECTOR22,
    VECTOR23,
    VECTOR24,
    VECTOR25,
    VECTOR26,
    VECTOR27,
    VECTOR28,
    VECTOR29,
    VECTOR30,
    VECTOR31,
    EXTERNAL_INTERRUPT_MIN = 32,
    EXTERNAL_INTERRUPT_MAX = 255
} vector_t;

/* control regs */

typedef union
{
    u64 val;
    struct
    {
        u64 pe : 1;
        u64 mp : 1;
        u64 em : 1;
        u64 ts : 1;
        u64 et : 1;
        u64 ne : 1;
        u64 reserved0 : 10;
        u64 wp : 1;
        u64 reserved1 : 1;
        u64 am : 1;
        u64 reserved2 : 10;
        u64 nw : 1;
        u64 cd : 1;
        u64 pg : 1;
        u64 undefined0 : 32;
    } fields;
} cr0_t;

typedef union 
{
    u16 val;
    struct 
    {
        u64 pe : 1;
        u64 mp : 1;
        u64 em : 1;
        u64 ts : 1;
        u64 et : 1;
        u64 ne : 1;
        u64 reserved0 : 10;
    } fields;
} msw_t;

typedef union
{
    u64 val;
    struct
    {
        u64 pcid : 12;
        u64 physAddr : 48;
        u64 reserved0 : 1;
        u64 user_lam57 : 1;
        u64 user_lam48 : 1;
        u64 flush_pcid : 1;
    } fields;
} cr3_pcide_t;

typedef union
{
    u64 val;
    struct 
    {
        u64 ignored0 : 3;
        u64 pwt : 1;
        u64 pcd : 1;
        u64 ignored1 : 7;
        u64 pml4_phys : 48;
        u64 reserved0 : 1;
        u64 user_lam57 : 1;
        u64 user_lam48 : 1;
        u64 reserved1 : 1;
    } fields;
} cr3_t;

typedef union
{
    u64 val;
    struct
    {
        u64 vme : 1;
        u64 pvi : 1;
        u64 tsd : 1;
        u64 de : 1;
        u64 pse : 1;
        u64 pae : 1;
        u64 mce : 1;
        u64 pge : 1;
        u64 pce : 1;
        u64 osfxr : 1;
        u64 osxmmexcpt : 1;
        u64 umip : 1;
        u64 la57 : 1;
        u64 vmxe : 1;
        u64 smxe : 1;
        u64 reserved0 : 1;
        u64 fsgsbase : 1;
        u64 pcide : 1;
        u64 osxsave : 1;
        u64 kl : 1;
        u64 smep : 1;
        u64 smap : 1;
        u64 pke : 1;
        u64 cet : 1;
        u64 pks : 1;
        u64 uintr : 1;
        u64 reserved1 : 1;
        u64 lass : 1;
        u64 lam : 1;
        u64 reserved2 : 3;
        u64 fred : 1;
        u64 reserved3 : 31;
    } fields;
} cr4_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 tpr : 4;
        u64 reserved0 : 60;
    } fields;
} cr8_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 present : 1;
        u64 rw : 1;
        u64 us : 1;
        u64 pwt : 1;
        u64 pcd : 1;
        u64 accessed : 1;
        u64 ignored0 : 1;
        u64 reserved0 : 1;
        u64 ignored1 : 3;
        u64 hlat_restart : 1;
        u64 pdpt_phys : 39;
        u64 reserved1 : 1;
        u64 ignored2 : 11;
        u64 xd : 1;
    } fields;
} pml4e_t;

typedef union
{
    u64 val;

    struct 
    {
        u64 present : 1;
        u64 rw : 1;
        u64 us : 1;
        u64 pwt : 1;
        u64 pcd : 1;
        u64 accessed : 1;
        u64 ignored0 : 1;
        u64 ps : 1;
        u64 ignored1 : 3;
        u64 hlat_restart : 1;
        u64 pd_phys : 39;
        u64 reserved1 : 1;
        u64 ignored2 : 11;
        u64 xd : 1;
    } fields;
} pdpte_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 present : 1;
        u64 rw : 1;
        u64 us : 1;
        u64 pwt : 1;
        u64 pcd : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 ps : 1;
        u64 global : 1;
        u64 ignored0 : 2;
        u64 hlat_restart : 1;
        u64 pat : 1;
        u64 reserved0 : 17;
        u64 pfn : 21;
        u64 reserved1 : 1;
        u64 ignored1 : 7;
        u64 pk : 4;
        u64 xd : 1; 
    } fields;
} pdpte_huge_t;

typedef union
{
    u64 val;
    struct 
    {
        u64 present : 1;
        u64 rw : 1;
        u64 us : 1;
        u64 pwt : 1;
        u64 pcd : 1;
        u64 accessed : 1;
        u64 ignored0 : 1;
        u64 ps : 1;
        u64 ignored1 : 3;
        u64 hlat_restart : 1;
        u64 pt_phys : 39;
        u64 reserved0 : 1;
        u64 ignored2 : 11;
        u64 xd : 1;
    } fields;
} pde_t;

typedef union
{
    u64 val;
    struct 
    {
        u64 present : 1;
        u64 rw : 1;
        u64 us : 1;
        u64 pwt : 1;
        u64 pcd : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 ps : 1;
        u64 global : 1;
        u64 ignored0 : 2;
        u64 hlat_restart : 1;
        u64 pat : 1;
        u64 reserved0 : 8;
        u64 pfn : 30;
        u64 reserved1 : 1;
        u64 ignored1 : 7;
        u64 pk : 4;
        u64 xd : 1;
    } fields;
} pde_huge_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 present : 1;
        u64 rw : 1;
        u64 us : 1;
        u64 pwt : 1;
        u64 pcd : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 pat : 1;
        u64 global : 1;
        u64 ignored0 : 2;
        u64 hlat_restart : 1;
        u64 pfn : 39;
        u64 reserved0 : 1;
        u64 ignored1 : 7;
        u64 pk : 4;
        u64 xd : 1;
    } fields;
} pte_t;

/* cpuid */

#define CPUID_MANUFACTURER_ID 0
#define GENUINE_INTEL_EBX 0x756E6547
#define GENUINE_INTEL_ECX 0x6C65746E
#define GENUINE_INTEL_EDX 0x49656E69

#define CPUID_FEATURE_BITS 1
typedef union
{
    u32 val;
    struct
    {
        u32 stepping_id : 4;
        u32 model : 4;
        u32 family_id : 4;
        u32 processor_type : 2;
        u32 reserved0 : 2;
        u32 extended_model_id : 4;
        u32 extended_family_id : 8;
        u32 reserved1 : 4;
    } fields;
} feature_bits_a_t;

typedef union
{
    u32 val;
    struct
    {
        u32 brand_index : 8;
        u32 cache_line_size : 8;
        u32 max_addressible_ids : 8;
        u32 lapic_id : 8;
    } fields;
} feature_bits_b_t;

typedef union
{
    u32 val;
    struct
    {
        u32 sse3 : 1;
        u32 pclmulqdq : 1;
        u32 dtes64 : 1;
        u32 monitor_mwait : 1;
        u32 ds_cpl : 1;
        u32 vmx : 1;
        u32 smx : 1;
        u32 est : 1;
        u32 tm2 : 1;
        u32 ssse3 : 1;
        u32 cnxt_id : 1;
        u32 sdbg : 1;
        u32 fma : 1;
        u32 cx16 : 1;
        u32 xptr : 1;
        u32 pdcm : 1;
        u32 reserved0 : 1;
        u32 pcid : 1;
        u32 dca : 1;
        u32 sse4_1 : 1;
        u32 sse4_2 : 1;
        u32 x2apic : 1;
        u32 movbe : 1;
        u32 popcnt : 1;
        u32 tsc_deadline : 1;
        u32 aes_ni : 1;
        u32 xsave : 1;
        u32 osxsave : 1;
        u32 avx : 1;
        u32 f16c : 1;
        u32 rdrnd : 1;
        u32 hypervisor : 1;
    } fields;
} feature_bits_c_t;

typedef union
{
    u32 val;
    struct
    {
        u32 fpu : 1;
        u32 vme : 1;
        u32 de : 1;
        u32 pse : 1;
        u32 tsc : 1;
        u32 msr : 1;
        u32 pae : 1;
        u32 mce : 1;
        u32 cx8 : 1;
        u32 apic : 1;
        u32 reserved0 : 1;
        u32 sep : 1;
        u32 mtrr : 1;
        u32 pge : 1;
        u32 mca : 1;
        u32 cmov : 1;
        u32 pat : 1;
        u32 pse_36 : 1;
        u32 psn : 1;
        u32 clfsh : 1;
        u32 nx : 1;
        u32 ds : 1;
        u32 acpi : 1;
        u32 mmx : 1;
        u32 fxsr : 1;
        u32 sse : 1;
        u32 sse2 : 1;
        u32 ss : 1;
        u32 htt : 1;
        u32 tm : 1;
        u32 ia64 : 1;
        u32 pbe : 1;
    } fields;
} feature_bits_d_t;

#define CPUID_PTM 6
typedef union
{
    u32 val;
    struct
    {
        u32 dts : 1;
        u32 tbt : 1;
        u32 arat : 1;
        u32 reserved0 : 1;
        u32 pln : 1;
        u32 ecmd : 1;
        u32 ptm : 1;
        u32 hwp : 1;
        u32 hwp_notification : 1;
        u32 hwp_activity_window : 1;
        u32 hwp_energy_perf_pref : 1;
        u32 hwp_package_level_req : 1;
        u32 reserved1 : 1;
        u32 hdc : 1;
        u32 tbmt : 1;
        u32 ia32_hwp_capabilities_highest_perf_ints : 1;
        u32 hwp_peci_override : 1;
        u32 flexible_hwp : 1;
        u32 fast_access_mode : 1;
        u32 hw_feedback : 1;
        u32 i32_hwp_request_idle_ignored_sibling_active : 1;
        u32 reserved2 : 1;
        u32 hwp_control_msr : 1;
        u32 thread_director_supported : 1;
        u32 ia32_therm_interrupt_bit_25_supported : 1;
        u32 reserved3 : 7;
    } fields;
} ptm_a_t;

typedef union
{
    u32 val;
    struct
    {
        u32 interrupt_thresholds_no_dts : 4;
        u32 reserved0 : 28;
    } fields;
} ptm_b_t;

typedef union
{
    u32 val;
    struct
    {
        u32 effective_frequency_interface_supported : 1;
        u32 acnt2_capability : 1;
        u32 reserved0 : 1;
        u32 performance_energy_bias_capability : 1;
        u32 reserved1 : 4;
        u32 thread_director_classes_supported_count : 8;
        u32 reserved2 : 16;
    } fields;
} ptm_c_t;

typedef union
{
    u32 val;
    struct
    {
        u32 perfmon_capability_reporting_supported : 1;
        u32 efficiency_capability_reporting_supported : 1;
        u32 reserved0 : 6;
        u32 hardware_feedback_interface_struct_size : 4;
        u32 reserved1 : 4;
        u32 this_hardware_feedback_interface_struct_index : 16;
    } fields;
} ptm_d_t;

#define CPUID_EXTENDED_FEATURES 7

typedef union 
{
    u32 val;
    struct 
    {
        u32 fsgsbase : 1;
        u32 tsc_adjust : 1;
        u32 sgx : 1;
        u32 bmi1 : 1;
        u32 hle : 1;
        u32 avx2 : 1;
        u32 fdp_excption_only : 1;
        u32 smep : 1;
        u32 bmi2 : 1;
        u32 erms : 1;
        u32 invpcid : 1;
        u32 rtm : 1;
        u32 rdtm : 1;
        u32 fpu_cs_ds_deprecated : 1;
        u32 mpx : 1;
        u32 rdta : 1;
        u32 avx512_f : 1;
        u32 avx512_dq : 1;
        u32 rdseed : 1;
        u32 adx : 1;
        u32 smap : 1;
        u32 avx512_ifma : 1;
        u32 pcommit : 1;
        u32 clflushopt : 1;
        u32 clwb : 1;
        u32 pt : 1;
        u32 avx512_fa : 1;
        u32 avx512_er : 1;
        u32 avx512_cd : 1;
        u32 sha : 1;
        u32 avx512_bw : 1;
        u32 avx512_vl : 1;
    } fields;
} extended_features0_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 prefetchwt1 : 1;
        u32 avx_512_vbmi1 : 1;
        u32 umip : 1;
        u32 pku : 1;
        u32 ospke : 1;
        u32 waitpkg : 1;
        u32 avx_512_vbmi2 : 1;
        u32 cet : 1;
        u32 gfni : 1;
        u32 vaes : 1;
        u32 vpclmulqdq : 1;
        u32 avx512_vnni : 1;
        u32 avx512_bitalg : 1;
        u32 tme_en : 1;
        u32 avx512_vpopcntdq : 1;
        u32 fzm : 1;
        u32 la57 : 1;
        u32 mawau : 5;
        u32 rdpid : 1;
        u32 kl : 1;
        u32 bus_lock_debug_exceptions : 1;
        u32 cldemote : 1;
        u32 mprr : 1;
        u32 movdiri : 1;
        u32 movdir64b : 1;
        u32 enqcmd : 1;
        u32 sgx_lc : 1;
        u32 pks : 1;
    } fields;
} extended_features0_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 sgx_tem : 1;
        u32 sgx_keys : 1;
        u32 avx_4vnniw : 1;
        u32 avx_4fmaps : 1;
        u32 fsrm : 1;
        u32 uintr : 1;
        u32 reserved0 : 1;
        u32 reserved1 : 1;
        u32 avx_vp2intersect : 1;
        u32 srbds_ctrl : 1;
        u32 md_clear : 1;
        u32 rtm_always_abort : 1;
        u32 reserved2 : 1;
        u32 rtm_force_abort : 1;
        u32 serialize : 1;
        u32 hybrid : 1;
        u32 tsxldtrk : 1;
        u32 reserved3 : 1;
        u32 pconfig : 1;
        u32 lbr : 1;
        u32 cet_ibt : 1;
        u32 reserved4 : 1;
        u32 amx_bf16 : 1;
        u32 avx512_fp16 : 1;
        u32 amx_tile : 1;
        u32 amx_int8 : 1;
        u32 ibrs : 1;
        u32 stibp : 1;
        u32 l1d_flush : 1;
        u32 ia32_arch_capabilities : 1;
        u32 ia32_core_capabilities : 1;
        u32 ssbd : 1;
    } fields;
} extended_features0_d_t ;

typedef union 
{
    u32 val;
    struct 
    {
        u32 sha512 : 1;
        u32 sm3 : 1;
        u32 sm4 : 1;
        u32 rao_int : 1;
        u32 avx_vnni : 1;
        u32 avx512_bf16 : 1;
        u32 lass : 1;
        u32 cmpccxadd : 1;
        u32 archperfmonext : 1;
        u32 dedup : 1;
        u32 fzrm : 1;
        u32 fsrs : 1;
        u32 rsrcs : 1;
        u32 reserved0 : 4;
        u32 fred : 1;
        u32 lkgs : 1;
        u32 wrmsrns : 1;
        u32 nmi_src : 1;
        u32 amx_fp16 : 1;
        u32 hreset : 1;
        u32 avx_ifma : 1;
        u32 reserved1 : 2;
        u32 lam : 1;
        u32 msrlist : 1;
        u32 reserved2 : 2;
        u32 cachewarp_mitigation : 1;
        u32 movrs : 1;
    } fields;
} extended_features1_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 4;
        u32 avx_vnni_int8 : 1;
        u32 avx_ne_convert : 1;
        u32 reserved1 : 2;
        u32 amx_complex : 1;
        u32 reserved2 : 1;
        u32 avx_vnni_int16 : 1;
        u32 reserved3 : 2;
        u32 uintr_timer : 1;
        u32 prefetchi : 1;
        u32 user_msr : 1;
        u32 reserved4 : 1;
        u32 uiret_uif_from_flags : 1;
        u32 cet_sss : 1;
        u32 avx10 : 1;
        u32 reserved5 : 1;
        u32 apx_f : 1;
        u32 reserved6 : 1;
        u32 mwait : 1;
        u32 slsm : 1;
        u32 reserved7 : 7;
    } fields;
} extended_features1_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 psfd : 1;
        u32 ipred_ctrl : 1;
        u32 rrsba_ctrl : 1;
        u32 ddpd_u : 1;
        u32 bhi_ctrl : 1;
        u32 mcdt_no : 1;
        u32 uc_lock_disable : 1;
        u32 monitor_mitigation_no : 1;
        u32 reserved0 : 24;
    } fields;
} extended_features2_d_t;

#define CPUID_ARCH_PERFMON 0x0A
typedef union 
{
    u32 val;
    struct 
    {
        u32 version_id : 8;
        u32 no_pmcs : 8;
        u32 bitwidth : 8;
        u32 no_arch_events_supported : 8;
    } fields;
} arch_perfmon_a_t;

/* basicallyyyy the bits enumerated in ebx r of 
   'negative polarity' so if theyre set its 
   unsupported so ye, but for extended its 
   positive polarity which is a lil weird lol */
typedef union 
{
    u32 val;
    struct
    {
        u32 unhalted_core_cycles : 1;
        u32 instructions_retired : 1;
        u32 unhalted_ref_cycles : 1;
        u32 llc_ref : 1;
        u32 llc_misses : 1;
        u32 branch_instr_retired : 1;
        u32 branch_miss_retired : 1;
        u32 topdown_slots : 1;
        u32 topdown_backend_bound : 1;
        u32 topdown_bad_speculation : 1;
        u32 topdown_frontend_bound : 1;
        u32 topdown_retiring : 1;
        u32 lbr_inserts : 1;
        u32 reserved0 : 19;
    } fields;
} arch_perfmon_b_t;

typedef union
{
    u32 val;
    struct
    {
       u32 fixed_func_pmcs : 5;
       u32 bitwidth : 8;
       u32 reserved0 : 2;
       u32 anythread_deprecation : 1;
       u32 reserved1 : 16;
    } fields;
} arch_perfmon_d_t;

#define CPUID_ARCH_PERFMON_EXT 0x23

typedef union 
{
    u32 val;
    struct
    {
        u32 umask2_supported : 1;
        u32 eq_supported : 1;
        u32 reserved0 : 30;
    } fields;
} arch_perfmon_ext_0b_t;

typedef union
{
    u32 val;
    struct 
    {   
        u32 no_tma_slots : 8;
        u32 reserved0 : 24;
    } fields;
} arch_perfmon_ext_0c_t;

#define CPUID_PCONFIG 0x1b

#define CPUID_TSC_CORE_CRYSTAL 0x15

typedef union 
{
    u32 val;
    struct 
    {
        u32 tsc_core_crystal_freq_ratio_denominator : 32;
    } fields;
} tsc_core_crystal_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 tsc_core_crystal_freq_ratio_numerator : 32;
    } fields;
} tsc_core_crystal_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 core_crystal_freq_hz : 32;
    } fields;
} tsc_core_crystal_c_t;

#define CPUID_CORE_FREQUENCY 0x16

typedef union 
{
    u32 val;
    struct 
    {
        u32 processor_base_freq_mhz : 16;
        u32 reserved0 : 16;
    } fields;
} core_frequency_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 processor_max_freq_mhz : 16;
        u32 reserved0 : 16;
    } fields;
} core_frequency_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 bus_ref_freq_mhz : 16;
        u32 reserved0 : 16;
    } fields;
} core_frequency_c_t;

#define CPUID_EXTENDED_SIG 0x80000001

typedef union
{
    u32 val;
    struct 
    {
        u32 reserved0 : 11;
        u32 syscall_sysret64 : 1;
        u32 reserved1 : 8;
        u32 nx : 1;
        u32 reserved2 : 5;
        u32 page_1gb : 1;
        u32 rdtscp : 1;
        u32 reserved3 : 1;
        u32 intel64 : 1;
        u32 reserved4 : 2;
    } fields;
} extended_sig_d_t;

#define CPUID_EXTENDED_FUNCTION1 0x80000007

typedef union
{
    u32 val;
    struct 
    {
        u32 reserved0 : 8;
        u32 tsc_invariant : 1;
        u32 reserved1 : 23;
    } fields;
} extended_function1_d_t;

#define CPUID_EXTENDED_FUNCTION2 0x80000008

typedef union
{
    u32 val;
    struct 
    {
        u32 phys_addr_size : 8;
        u32 lin_addr_size : 8;
        u32 guest_phys_addr_size : 8;
        u32 reserved0 : 8;
    } fields;
} extended_function2_a_t;

/* msr's */

#define IA32_XSS 0xda0

#define IA32_MONITOR_FILTER_SIZE 0x06

#define IA32_UINTR_RR 0x985
#define IA32_UINTR_HANDLER 0x986
#define IA32_UINTR_STACKADJUST 0x987
#define IA32_UINTR_MISC 0x988
#define IA32_UINTR_PD 0x989
#define IA32_UINTR_TT 0x98a
#define IA32_UINTR_TIMER 0x1b00

#define IA32_FRED_CONFIG 0x1d4
#define IA32_FRED_RSP0 0x1cc
#define IA32_FRED_RSP1 0x1cd
#define IA32_FRED_RSP2 0x1ce
#define IA32_FRED_RSP3 0x1cf
#define IA32_FRED_STKLVLS 0x1d0
#define IA32_FRED_SSP1 0x1d1
#define IA32_FRED_SSP2 0x1d2
#define IA32_FRED_SSP3 0x1d3

#define IA32_UMWAIT_CONTROL 0xe1

#define IA32_TSC_AUX 0xC0000103
typedef union 
{
    u64 val;
    struct 
    {
        u32 sig : 32;
        u32 reserved0 : 32;
    } fields;
} ia32_tsc_aux_t;

#define IA32_MCG_CAP 0x179
typedef union
{
    u64 val;
    struct
    {

        u64 count : 8;
        u64 ia32_mcg_ctl_present : 1;
        u64 mcg_ext_present : 1;
        u64 mcp_cmci_present : 1;
        u64 mcg_tes_present : 1;
        u64 reserved0 : 4;
        u64 mcg_ext_count : 8;
        u64 mcg_ser_present : 1;
        u64 reserved1 : 1;
        u64 mcg_elog_present : 1;
        u64 mcg_lmce_present : 1;
        u64 reserved2 : 36;
    } fields;
} ia32_mcg_cap_t;

#define IA32_PAT 0x277
typedef union
{
    u64 val;
    struct
    {
        u64 pa0 : 3;
        u64 reserved0 : 5;
        u64 pa1 : 3;
        u64 reserved1 : 5;
        u64 pa2 : 3;
        u64 reserved2 : 5;
        u64 pa3 : 3;
        u64 reserved3 : 5;
        u64 pa4 : 3;
        u64 reserved4 : 5;
        u64 pa5 : 3;
        u64 reserved5 : 5;
        u64 pa6 : 3;
        u64 reserved6 : 5;
        u64 pa7 : 3;
        u64 reserved7 : 5;
    } fields;
} ia32_pat_t;

#define IA32_EFER 0xC0000080
typedef union
{
    u64 val;
    struct
    {
        u64 syscall_enable : 1;
        u64 reserved0 : 7;
        u64 lme : 1;
        u64 reserved1 : 1;
        u64 lma : 1;
        u64 nxe : 1;
        u64 reserved2 : 52;
    } fields;
} ia32_efer_t;

#define IA32_FEATURE_CONTROL 0x03A
typedef union
{
    u64 val;
    struct
    {
        u64 locked : 1;
        u64 vmx_inside_smx : 1;
        u64 vmx_outside_smx : 1;
        u64 reserved0 : 5;
        u64 senter_local_enables : 7;
        u64 senter_global_enable : 1;
        u64 reserved1 : 1;
        u64 sgx_launch_control_enable : 1;
        u64 sgx_global_enable : 1;
        u64 reserved2 : 1;
        u64 lmce_on : 1;
        u64 reserved3 : 43;
    } fields;
} ia32_feature_control_t;

#define IA32_MISC_ENABLE 0x1A0
typedef union
{
    u64 val;
    struct
    {
        u64 fast_strings_enable : 1;
        u64 reserved1 : 2;
        u64 tcc : 1;
        u64 reserved2 : 3;
        u64 perfmon_enable : 1;
        u64 reserved3 : 3;
        u64 bts_unavailable : 1;
        u64 pebs_unavailable : 1;
        u64 reserved4 : 3;
        u64 enhanced_speedstep_enabled : 1;
        u64 reserved5 : 1;
        u64 monitor_fsm_enabled : 1;
        u64 reserved6 : 3;
        u64 limit_cpuid_maxval : 1;
        u64 xtrp_message_disable : 1;
        u64 reserved : 20;
    } fields;
} ia32_misc_enable_t;

#define IA32_SPEC_CTRL 0x48
typedef union
{
    u64 val;
    struct
    {
        u64 ibrs : 1;  
        u64 stibp : 1; 
        u64 ssbd : 1;
        u64 ipred_dis_u : 1;
        u64 ipred_dis_s : 1;
        u64 rrsba_dis_u : 1;
        u64 rrsba_dis_s : 1;
        u64 psfd : 1;   
        u64 ddpd_u : 1; 
        u64 reserved1 : 1;
        u64 bhi_dis_s : 1; 
        u64 reserved2 : 53;
    } fields;
} ia32_spec_ctrl_t;

#define IA32_ARCH_CAPABILITIES 0x10A
typedef union
{
    u64 val;
    struct
    {
        u64 rdcl_no : 1;
        u64 ibrs_all : 1;
        u64 rsba : 1;
        u64 skip_l1dfl_vmentry : 1;
        u64 ssb_no : 1;
        u64 mds_no : 1;
        u64 if_pschange_mc_no : 1;
        u64 tsx_ctrl : 1;
        u64 taa_no : 1;
        u64 mcu_control : 1;
        u64 mis__packedage_ctls : 1;
        u64 energy_filtering_ctl : 1;
        u64 doitm : 1;
        u64 sbdr_ssdp_no : 1;
        u64 fbsdp_no : 1;
        u64 psdp_no : 1;
        u64 mcu_enumeration : 1;
        u64 fb_clear : 1;
        u64 fb_clear_ctrl : 1;
        u64 rrsba : 1;
        u64 bhi_no : 1;
        u64 xapic_disable_status : 1;
        u64 mcu_extended_service : 1;
        u64 overclocking_status : 1;
        u64 pbrsb_no : 1;
        u64 gds_ctrl : 1;
        u64 gds_no : 1;
        u64 rfds_no : 1;
        u64 rfds_clear : 1;
        u64 ign_umonitor_support : 1;
        u64 mon_umon_mitg_support : 1;
        u64 reserved0 : 1;
        u64 pbopt_supported : 1;
        u64 reserved1 : 29;
        u64 its_no : 1;
        u64 msr_virtual_enumeration_supported : 1;
    } fields;
} ia32_arch_capabilities_t;

#define IA32_UARCH_MISC_CTRL 0x1b01

#define IA32_PRED_CMD 0x49

#define IA32_PBOPT_CTRL 0xbf

#define IA32_XAPIC_DISABLE_STATUS 0xbd

#define IA32_TSX_FORCE_ABORT 0x10f
typedef union 
{
    u64 val;
    struct 
    {
        u64 rtm_force_abort : 1;
        u64 tsx_cpuid_clear : 1;
        u64 sdv_enable_rtm : 1;
        u64 reserved0 : 61;
    } fields;
} ia32_tsx_force_abort_t;

#define IA32_TSX_CTRL 0x122
typedef union 
{
    u64 val;
    struct 
    {
        u64 rtm_always_aborts : 1;
        u64 tsx_cpuid_clear : 1;
        u64 reserved0 : 62;
    } fields;
} ia32_tsx_ctrl_t;

#define IA32_MCU_OPT_CTRL 0x123
typedef union
{
    u64 val;
    struct
    {
        u64 rngds_mit_dis : 1;
        u64 rtm_allow : 1;
        u64 rtm_locked : 1;
        u64 fb_clear_dis : 1;
        u64 gds_mit_dis : 1;
        u64 gds_mit_locked : 1;
        u64 ign_umonitor : 1;
        u64 mon_umon_mit : 1;
        u64 reserved0 : 56;
    } fields;
} ia32_mcu_opt_ctrl_t;

#define IA32_DEBUG_CTL 0x1D9
typedef union
{
    u64 val;
    struct
    {
        u64 lbr : 1;
        u64 btf : 1;
        u64 bld : 1;
        u64 reserved1 : 3;
        u64 tr : 1;
        u64 bts : 1;
        u64 btint : 1;
        u64 bts_off_os : 1;
        u64 bts_off_usr : 1;
        u64 freeze_lbrs_on_pmi : 1;
        u64 freeze_perfmon_on_pmi : 1;
        u64 enable_unicore_pmi : 1;
        u64 freeze_while_smm : 1;
        u64 rtm_debug : 1;
        u64 reserved2 : 48;
    } fields;
} ia32_debug_ctl_t;

#define IA32_FS_BASE 0xC0000100
#define IA32_GS_BASE 0xC0000101
#define IA32_KERNEL_GS_BASE 0xC0000102

#define IA32_BARRIER 0x2f

#define IA32_SR_BIOS_DONE 0x151
typedef union 
{
    u64 val;
    struct 
    {
        u64 invd_ud : 1;
        u64 reserved0 : 63;
    } fields;
} ia32_sr_bios_done_t;

#define IA32_USER_MSR_CTL 0x1c

#define IA32_VIRTUAL_ENUMERATION 0x50000000
typedef union 
{
    u64 val;
    struct 
    {
        u64 ia32_virtual_mitigation_ctrl_supported : 1;
        u64 reserved0 : 63;
    } fields;
} ia32_virtual_enumeration_t;

#define IA32_VIRTUAL_MITIGATION_ENUM 0x50000001
typedef union 
{
    u64 val;
    struct 
    {
        u64 bhb_clear_seq_s_supported : 1;
        u64 retpoline_s_supported : 1;
        u64 reserved0 : 62;
    } fields;
} ia32_virtual_mitigation_enum_t;

#define IA32_VIRTUAL_MITIGATION_CTRL 0x50000002
typedef union 
{
    u64 val;
    struct 
    {
        u64 bhb_clear_seq_s_used : 1;
        u64 retpoline_s_used : 1;
        u64 reserved0 : 62;
    } fields;
} ia32_virtual_mitigation_ctrl_t;

#define IA32_PERF_GLOBAL_STATUS 0x38E
#define IA32_PERF_GLOBAL_STATUS_RESET 0x390
#define IA32_PERF_GLOBAL_STATUS_SET 0x391
#define IA32_PERF_GLOBAL_INUSE 0x392
typedef union
{
    u64 val;
    struct
    {
        u64 ovf_pmcn : 31;
        u64 reserved0 : 1;
        u64 ovf_fixedctrm : 15;
        u64 reserved1 : 1;
        u64 ovf_perf_metrics : 1;
        u64 reserved2 : 6;
        u64 ToPA_filled_pmi : 1;
        u64 reserved3 : 2;
        u64 lbr_frozen : 1;
        u64 pmc_frozen : 1;
        u64 asci : 1;
        u64 ovf_uncore : 1;
        u64 ovf_ds_buf : 1;
        u64 cond_changed : 1;
    } fields;
} ia32_perf_global_status_t;

#define IA32_PERF_GLOBAL_CTRL 0x38F
typedef union
{
    u64 val;
    struct
    {
        u64 enable_pmcn : 31;
        u64 reserved0 : 1;
        u64 enable_fixedctrm : 15;
        u64 reserved1 : 1;
        u64 enable_perf_metrics : 1;
        u64 reserved2 : 15;
    } fields;
} ia32_perf_global_ctrl_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 perfevtseln_in_use : 32;
        u64 fixed_ctrn_in_use : 31;
        u64 pmi_in_use : 1;
    } fields;
} ia32_perf_global_inuse_t;

#define IA32_LBR_CTL 0x14CE
typedef union
{
    u64 val;
    struct
    {
        u64 lbr_enabled : 1;
        u64 os : 1;
        u64 usr : 1;
        u64 call_stack : 1;
        u64 reserved1 : 12;
        u64 cond : 1;
        u64 near_rel_jmp : 1;
        u64 near_ind_jmp : 1;
        u64 near_rel_call : 1;
        u64 near_ind_call : 1;
        u64 near_ret : 1;
        u64 other_branch : 1;
        u64 reserved2 : 41;
    } fields;
} ia32_lbr_ctl_t;

#define IA32_LBR_DEPTH 0x14CF

typedef enum
{
    IA32_LBR_0_INFO = 0x1200,
    IA32_LBR_31_INFO = 0x121F,
    
    IA32_LBR_0_FROM_IP = 0x1500,
    IA32_LBR_31_FROM_IP = 0x151F,

    IA32_LBR_0_TO_IP = 0x1600,
    IA32_LBR_31_TO_IP = 0x161F,
} lbr_stack_entry;

typedef union
{
    u64 val;
    struct
    {
        u64 cyc_cnt : 16;
        u64 undefined : 40;
        u64 br_type : 4;
        u64 cyc_cnt_valid : 1;
        u64 tsx_abort : 1;
        u64 in_tsx : 1;
        u64 mispred : 1;
    } fields;
} ia32_lbr_info_t;

typedef enum 
{
    COND,
    NEAR_JMP_IND,
    NEAR_JMP_REL,
    NEAR_CALL_IND,
    NEAR_CALL_REL,
    NEAR_RET,
    RESERVED,
    OTHER,
    UNKNOWN,
} lbr_branch_type;

#define IA32_PMC0 0xC1
#define IA32_A_PMC0 0x4C1
#define IA32_PERFEVTSEL0 0x186
typedef union
{
    u64 val;
    struct
    {
        u64 evtsel : 8;
        u64 umask : 8;
        u64 usr : 1;
        u64 os : 1;
        u64 edge : 1;
        u64 pc : 1;
        u64 ovf_pmi : 1;
        u64 anythread : 1;
        u64 enable_pmc : 1;
        u64 inv : 1;
        u64 cmask : 8;
        u64 reserved0 : 32;
    } fields;
} ia32_perfevtsel_t;

#define IA32_PERF_CAPABILITIES 0x345
typedef union
{
    u64 val;
    struct
    {
        u64 lbr_format : 6;
        u64 pebs_trap : 1;
        u64 pebs_save_arch_regs : 1;
        u64 pebs_record_format : 4;
        u64 freeze_smm_supported : 1;
        u64 fw_ia32_a_pmcx : 1;
        u64 pebs_baseline : 1;
        u64 perf_metrics_available : 1;
        u64 pebs_written_to_intel_pt_ts : 1;
        u64 pebs_retire_latency_output : 1;
        u64 tsx_address : 1;
        u64 rdpmc_metrics_clear : 1;
        u64 reserved0 : 44;
    } fields;
} ia32_perf_capabilities_t;

/* 
    use these for tmam, heres a note to myself so I dont frickin forget 
    what each of em r for

    * IA32_FIXED_CTR0 - counts instructions retired 
    * IA32_FIXED_CTR1 - counts cycles when the core is unhalted
    * IA32_FIXED_CTR2 - counts at tsc rate when core is unhalted
    * IA32_FIXED_CTR3 - counts topdown slots for an unhalted core
    * IA32_FIXED_CTR4 - counts topdown slots wasted bcoz of bad speculation
    * IA32_FIXED_CTR5 - counts topdown slots wasted bc of frontend delays
    * IA32_FIXED_CTR6 - counts topdown slots successfully retired
*/
typedef enum 
{
    IA32_FIXED_CTR0 = 0x309, 
    IA32_FIXED_CTR1, 
    IA32_FIXED_CTR2,
    IA32_FIXED_CTR3, 
    IA32_FIXED_CTR4, 
    IA32_FIXED_CTR5, 
    IA32_FIXED_CTR6, 
} ia32_fixed_ctr_addr_t;

#define IA32_FIXED_CTR_CTRL 0x38D
typedef union
{
    u64 val;
    struct 
    {
        u64 fixedctr0_os : 1;
        u64 fixedctr0_usr : 1;
        u64 fixedctr0_anythread : 1;
        u64 fixedctr0_ovf_pmi : 1;
        u64 fixedctr1_os : 1;
        u64 fixedctr1_usr : 1;
        u64 fixedctr1_anythread : 1;
        u64 fixedctr1_ovf_pmi : 1;
        u64 fixedctr2_os : 1;
        u64 fixedctr2_usr : 1;
        u64 fixedctr2_anythread : 1;
        u64 fixedctr2_ovf_pmi : 1;
        u64 fixedctr3_os : 1;
        u64 fixedctr3_usr : 1;
        u64 reserved0 : 1;
        u64 fixedctr3_ovf_pmi : 1;
        u64 reserved1 : 48;
    } fields;
} ia32_fixed_ctr_ctrl_t;

#define IA32_PMC_GP0_CTR 0x1900
#define IA32_PMC_FX0_CTR 0x1980
typedef union
{
    u64 val;
    struct
    {
        u64 reload_val : 48;
        u64 reserved0 : 16;
    } fields;
} ia32_pmc_gp_ctr_t;
typedef ia32_pmc_gp_ctr_t ia32_pmc_fx_ctr_t;

#define IA32_PMC_GP0_CFG_A 0x1901
typedef union
{
    u64 val;
    struct
    {
        u64 evtsel : 8;
        u64 umask1 : 8;
        u64 usr : 1;
        u64 os : 1;
        u64 edge : 1;
        u64 reserved0 : 1;
        u64 ovf_pmi : 1;
        u64 anythread : 1;
        u64 enable_pmc : 1;
        u64 inv : 1;
        u64 cmask : 8;
        u64 reserved1 : 3;
        u64 enable_lbr_log : 1;
        u64 equal : 1;
        u64 reserved2 : 3;
        u64 umask2 : 8;
        u64 reserved3 : 16;
    } fields;
} ia32_pmc_gp_cfg_a_t;
typedef ia32_pmc_gp_cfg_a_t ia32_pmc_fx_cfg_a_t;

#define IA32_PMC_GP2_CFG_B 0x190A
#define IA32_PMC_FX0_CFG_B 0x1982
typedef union
{
    u64 val;
    struct 
    {
        u64 reserved0 : 2;
        u64 reload_pmc2 : 1;
        u64 reload_pmc3 : 1;
        u64 reload_pmc4 : 1;
        u64 reload_pmc5 : 1;
        u64 reload_pmc6 : 1;
        u64 reload_pmc7 : 1;
        u64 reserved1 : 24;
        u64 reload_fc0 : 1;
        u64 reload_fc1 : 1;
        u64 reserved2 : 14;
        u64 metrics_clear : 1;
        u64 reserved3 : 15;
    } fields;
} ia32_pmc_gp_cfg_b_t;
typedef ia32_pmc_gp_cfg_b_t ia32_pmc_fx_cfg_b_t;

#define IA32_PMC_GP0_CFG_C 0x1903
#define IA32_PMC_FX0_CFG_C 0x1983
typedef union 
{
    u64 val;
    struct 
    {
        u64 reload_val : 32;
        u64 reserved0 : 32;
    } fields;
} ia32_pmc_gp_cfg_c_t;
typedef ia32_pmc_gp_cfg_c_t ia32_pmc_fx_cfg_c_t;

#define IA32_FLUSH_CMD 0x10b
typedef union 
{
    u64 val;
    struct 
    {
        u64 l1d_flush : 1;
        u64 reserved0 : 63;
    };
} ia32_flush_cmd_t;

#define IA32_APIC_BASE 0x1b
typedef union
{
    u64 val;
    struct
    {
        u64 reserved0 : 8;
        u64 bsp : 1;
        u64 reserved1 : 1;
        u64 enable_x2apic_mode : 1;
        u64 apic_global_enable : 1;
        u64 apic_base_phys : 52;
    } fields;
} ia32_apic_base_t;

#define IA32_XAPIC_DISABLE_STATUS 0xbd
typedef union {
    u64 val;
    struct
    {
        u64 legacy_xapic_disabled : 1;
        u64 reserved0 : 63;
    } fields;
} ia32_xapic_disable_status_t;

#define IA32_X2APIC_BASE 0x800
#define IA32_X2APIC_ID 0x802
#define IA32_X2APIC_VERSION 0x803
#define IA32_X2APIC_TPR 0x808
#define IA32_X2APIC_PPR 0x80a
#define IA32_X2APIC_EOI 0x80b
#define IA32_X2APIC_LDR 0x80d
#define IA32_X2APIC_SIVR 0x80f
enum  
{
    IA32_X2APIC_ISR0 = 0x810,
    IA32_X2APIC_ISR1,
    IA32_X2APIC_ISR2,
    IA32_X2APIC_ISR3,
    IA32_X2APIC_ISR4,
    IA32_X2APIC_ISR5,
    IA32_X2APIC_ISR6,
    IA32_X2APIC_ISR7,

    IA32_X2APIC_TMR0,
    IA32_X2APIC_TMR1,
    IA32_X2APIC_TMR2,
    IA32_X2APIC_TMR3,
    IA32_X2APIC_TMR4,
    IA32_X2APIC_TMR5,
    IA32_X2APIC_TMR6,
    IA32_X2APIC_TMR7,

    IA32_X2APIC_IRR0,
    IA32_X2APIC_IRR1,
    IA32_X2APIC_IRR2,
    IA32_X2APIC_IRR3,
    IA32_X2APIC_IRR4,
    IA32_X2APIC_IRR5,
    IA32_X2APIC_IRR6,
    IA32_X2APIC_IRR7,

    IA32_X2APIC_ESR
};

#define IA32_X2APIC_LVT_CMCI 0x82f
#define IA32_X2APIC_ICR 0x830

enum
{
    IA32_X2APIC_LVT_TIMER = 0x832,
    IA32_X2APIC_LVT_THERMAL,
    IA32_X2APIC_LVT_PMI,
    IA32_X2APIC_LVT_LINT0,
    IA32_X2APIC_LVT_LINT1,
    IA32_X2APIC_LVT_ERROR,
    IA32_X2APIC_LVT_INIT_COUNT,
    IA32_X2APIC_LVT_CUR_COUNT
};

#define IA32_X2APIC_DIV_CONF 2110
#define IA32_X2APIC_SELF_IPI 2111

/* intrinsics */

inline rflags_t __read_rflags(void)
{
    rflags_t rflags;
    __asm__ __volatile__ ("pushfq; popq %0;":"=r"(rflags.val));
    return rflags;
}

inline void __write_rflags(rflags_t rflags)
{
    __asm__ __volatile__ ("pushq %0; popfq;"::"r"(rflags.val));
}

inline void __cpuid(u32 *eax, u32 *ebx, u32 *ecx, u32 *edx)
{
    __asm__ __volatile__ (
        "cpuid"
        :"=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        :"a"(*eax), "c"(*ecx)
    );
}

inline void cpuid(u32 leaf, u32 subleaf, u32 *eax, 
                         u32 *ebx, u32 *ecx, u32 *edx)
{
    __asm__ __volatile__ (
        "cpuid"
        :"=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        :"a"(leaf), "c"(subleaf)
    );
}

inline bool is_cpu_intel(void)
{
    u32 regs[4] = {0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);
    return regs[1] == GENUINE_INTEL_EBX && 
           regs[3] == GENUINE_INTEL_EDX &&
           regs[2] == GENUINE_INTEL_ECX;
}

inline void __wrmsrl(u32 msr, u64 val)
{
    __asm__ __volatile__ (
        "movl %%esi, %%eax;"
        "shrq $32, %%rsi;"
        "movl %%esi, %%edx;"
        "wrmsr;"
        :
        :"c"(msr), "S"(val)
    );
}

inline u64 __rdmsrl(u32 msr)
{
    u32 low = 0;
    u32 high = 0;
    __asm__ __volatile__ ("rdmsr" :"=a"(low), "=d"(high) :"c"(msr));

    return ((u64)high << 32) | low;
}

inline u8 __inb(u16 port)
{
    u8 ret;
    __asm__ __volatile__ ("inb %1, %0" :"=a"(ret) :"d"(port));
    return ret;
}

inline u16 __inw(u16 port)
{
    u16 ret;
    __asm__ __volatile__ ("inw %1, %0" :"=a"(ret) :"d"(port));
    return ret;
}

inline u32 __inl(u16 port)
{
    u32 ret;
    __asm__ __volatile__ ("inl %1, %0" :"=a"(ret) :"d"(port));
    return ret;
}

inline void __outb(u16 port, u8 val)
{
    __asm__ __volatile__ ("outb %0, %1" ::"a"(val), "d"(port));
}

inline void __outw(u16 port, u16 val)
{
    __asm__ __volatile__ ("outw %0, %1" ::"a"(val), "d"(port));
}

inline void __outl(u16 port, u32 val)
{
    __asm__ __volatile__ ("outl %0, %1" ::"a"(val), "d"(port));
}

inline void __io_wait(void)
{
    __outb(128, 0);
}

inline void __hlt(void)
{
    __asm__ __volatile__ ("hlt");
}

inline void __hlt_loop(void)
{
    while (1)
        __hlt();
}

inline void __lfence(void)
{
    __asm__ __volatile__ ("lfence");
}

inline void __pause(void)
{
    __asm__ __volatile__ ("pause");
}

inline cr0_t __read_cr0(void)
{
    cr0_t cr0;
    __asm__ __volatile__ ("movq %%cr0, %0":"=r"(cr0.val));
    return cr0;
}

inline u64 __read_cr2(void)
{
    u64 cr2;
    __asm__ __volatile__ ("movq %%cr2, %0":"=r"(cr2));   
    return cr2;
}

inline cr3_t __read_cr3(void)
{
    cr3_t cr3;
    __asm__ __volatile__ ("movq %%cr3, %0":"=r"(cr3.val));
    return cr3;
}

inline cr4_t __read_cr4(void)
{
    cr4_t cr4;
    __asm__ __volatile__ ("movq %%cr4, %0":"=r"(cr4.val));
    return cr4;
}

inline cr8_t __read_cr8(void)
{
    cr8_t cr8;
    __asm__ __volatile__ ("movq %%cr8, %0":"=r"(cr8.val));
    return cr8;
}

inline void __write_cr0(cr0_t cr0)
{
    __asm__ __volatile__ ("movq %0, %%cr0"::"r"(cr0.val));
}

inline void __write_cr4(cr4_t cr4)
{
    __asm__ __volatile__ ("movq %0, %%cr4"::"r"(cr4.val));   
}

inline void __write_cr8(cr8_t cr8)
{
    __asm__ __volatile__ ("movq %0, %%cr8"::"r"(cr8.val));   
}

inline void __wbinvd(void)
{
    __asm__ __volatile__ ("wbinvd");
}

inline void __invlpg(u8 *addr)
{
    __asm__ __volatile__ ("invlpg (%0)"::"r"(addr):"memory");
}

inline void __flush_tlb(void)
{
    __asm__ __volatile__ (
        "mov %%cr3, %%rax;"
        "mov %%rax, %%cr3;" 
        :
        :
        : "%rax");
}

inline u64 __rdtsc64(void)
{
    u32 eax;
    u32 edx;

    __asm__ __volatile__ ("rdtsc;":"=a"(eax), "=d"(edx));

    return ((u64)edx << 32) | eax;
}

inline void __prefetcht0(u8 *addr)
{
    __asm__ __volatile__ ("prefetcht0 %0"::"m"(*addr):"memory");
}

inline void __prefetcht1(u8 *addr)
{
    __asm__ __volatile__ ("prefetcht1 %0"::"m"(*addr):"memory");
}

inline void __prefetcht2(u8 *addr)
{
    __asm__ __volatile__ ("prefetcht2 %0"::"m"(*addr):"memory");
}

inline void __prefetchnta(u8 *addr)
{
    __asm__ __volatile__ ("prefetchnta %0"::"m"(*addr):"memory");
}

inline void __sgdt(struct descriptor_table64 *gdtr)
{
    __asm__ __volatile__ ("sgdt %0;":"=m"(*gdtr) ::"memory");
}

inline void __lgdt(struct descriptor_table64 *gdtr)
{
    __asm__ __volatile__ ("lgdt %0;"::"m"(*gdtr) :"memory");
}

inline void __ltr(u16 tr)
{
    __asm__ __volatile__ ("ltr %0;"::"r"(tr));
}

inline void __sidt(struct descriptor_table64 *idtr)
{
    __asm__ __volatile__ ("sidt %0;":"=m"(*idtr) ::"memory");
}

inline void __lidt(struct descriptor_table64 *idtr)
{
    __asm__ __volatile__ ("lidt %0;"::"m"(*idtr) :"memory");
}

inline void __cli(void)
{
    __asm__ __volatile ("cli");
}

inline void __sti(void)
{
    __asm__ __volatile__ ("sti");
}

inline u64 __rdgsbase(void)
{
    u64 gs_base;
    __asm__ __volatile__ ("rdgsbase %0":"=r"(gs_base));
    return gs_base;
}

inline u32 __readgs32(u32 offset)
{
    u32 val;
    __asm__ __volatile__ ("movl %%gs:(%1), %0":"=r"(val) :"r"(offset));
    return val;
}

inline u64 __readgs64(u32 offset)
{
    u64 val;
    __asm__ __volatile__ ("movq %%gs:(%1), %0":"=r"(val) :"r"(offset));
    return val;
}

inline void __writegs64(u32 offset, u64 val)
{
    __asm__ __volatile__ ("movq %0, %%gs:(%1)"::"r"(val), "r"(offset));
}

inline u32 __readfs32(u32 offset)
{
    u32 val;
    __asm__ __volatile__ ("movl %%fs:(%1), %0":"=r"(val) :"r"(offset));
    return val;
}

inline u64 __readfs64(u32 offset)
{
    u64 val;
    __asm__ __volatile__ ("movq %%fs:(%1), %0":"=r"(val) :"r"(offset));
    return val;
}

inline void __writefs64(u32 offset, u64 val)
{
    __asm__ __volatile__ ("movq %0, %%fs:(%1)"::"r"(val), "r"(offset));
}

inline u64 __read_dr7(void)
{
    u64 val;
    __asm__ __volatile__ ("movq %%dr7, %0":"=r"(val));
    return val;
}

inline bool __lar(u16 segment, u32 *outp)
{
    u8 ret = 0;

    __asm__ __volatile__(
        "lar %[segment], %[outp];"
        "sete %[ret];"
        : [outp] "=r"(*outp), [ret] "=r"(ret)
        : [segment] "r"(segment)
        : "cc");

    return ret;
}

inline bool __lsl(u16 segment, u32 *outp)
{
    u8 ret = 0;

    __asm__ __volatile__(
        "lsl %[segment], %[outp];"
        "sete %[ret];"
        : [outp] "=r"(*outp), [ret] "=r"(ret)
        : [segment] "r"(segment)
        : "cc");

    return ret;
}

inline selector_t __read_cs(void)
{
    selector_t cs;
    __asm__ __volatile__ ("movl %%cs, %%eax":"=a"(cs.val));
    return cs;
}

inline selector_t __read_ds(void)
{
    selector_t ds;
    __asm__ __volatile__ ("movl %%ds, %%eax":"=a"(ds.val));
    return ds;
}

inline selector_t __read_ss(void)
{
    selector_t ss;
    __asm__ __volatile__ ("movl %%ss, %%eax":"=a"(ss.val));
    return ss;
}

inline selector_t __read_es(void)
{
    selector_t es;
    __asm__ __volatile__ ("movl %%es, %%eax":"=a"(es.val));
    return es;
}

inline selector_t __read_fs(void)
{
    selector_t fs;
    __asm__ __volatile__ ("movl %%fs, %%eax":"=a"(fs.val));
    return fs;
}

inline selector_t __read_gs(void)
{
    selector_t gs;
    __asm__ __volatile__ ("movl %%gs, %%eax":"=a"(gs.val));
    return gs;
}

inline selector_t __sldt(void)
{
    selector_t ldtr;
    __asm__ __volatile__ ("sldt %%eax":"=a"(ldtr.val));
    return ldtr;
}

inline selector_t __str(void)
{
    selector_t tr;
    __asm__ __volatile__ ("str %%eax":"=a"(tr.val));
    return tr;
}

inline void __fxsave(void *addr)
{
    __asm__ __volatile__ ("fxsave (%0)"::"r"(addr):"memory");
}

inline void __fxrstore(void *addr)
{
    __asm__ __volatile__ ("fxrstore (%0)"::"r"(addr):"memory");
}

inline u64 __segment_base(selector_t selector, gdt_descriptor32_t *gdt)
{
    gdt_descriptor32_t *desc = &gdt[selector.fields.index];
    
    if (desc->fields.present == 0)
        return 0;

    u64 base = (desc->fields.base_high << 24) | 
               (desc->fields.base_mid << 16) |
                desc->fields.base_low;

    bool system = desc->fields.descriptor_type == 0;
    u32 type = desc->fields.segment_type;
    if (system && (type == TSS_AVAILABLE || type == TSS_BUSY)) {

        struct gdt_descriptor64 *tss_desc = (void *)desc;
        base |= ((u64)tss_desc->base_upper << 32);
    }

    return base;
}

inline u32 __segment_limit(selector_t selector)
{
    u32 limit = 0;
    __lsl(selector.val, &limit);
    return limit;
}

inline access_rights_t __segment_ar(selector_t selector)
{
    access_rights_t ar = {0};
    if (selector.fields.index == 0) {
        ar.fields.segment_unusable = 1;
        return ar;
    }

    __lar(selector.val, &ar.val);

    ar.val >>= 8;
    ar.fields.reserved0 = 0;
    ar.fields.reserved1 = 0;
    ar.fields.segment_unusable = 0;

    return ar;
}

typedef enum 
{
    INVAL,
    SMT,
    CORE
} subleaf_type_0x0b_t;

//bool __this_topology_0x1f(u32 *lapic_id, u32 *thread_id,
//                          u32 *core_id, u32 *pkg_id)

bool __this_topology_0x0b(u32 *lapic_id, u32 *thread_id, 
                          u32 *core_id, u32 *pkg_id);
   
void __this_topology_legacy(u32 *lapic_id, u32 *thread_id, 
                            u32 *core_id, u32 *pkg_id);

void this_topology(u32 *lapic_id, u32 *thread_id, u32 *core_id, u32 *pkg_id);

/* VMX */

#define CPUID_HV_MIN 0x40000000
#define CPUID_HV_MAX 0x4FFFFFFF

#define VMXON_REGION_SIZE 4096
#define VMCS_REGION_SIZE 4096

#define IA32_VMX_BASIC 0x480
typedef union
{
    u64 val;
    struct
    {
        u64 revision_id : 31;
        u64 reserved0 : 1;
        u64 num_bytes : 13;
        u64 reserved1 : 3;
        u64 phys_addr_width : 1;
        u64 dual_monitor_supported : 1;
        u64 vmcs_mem_type : 4;
        u64 vmcs_instruction_info_reports : 1;
        u64 defaults_to_one_clear : 1;
        u64 vmentry_hw_exceptions : 1;
        u64 reserved2 : 7;
    } fields;
} ia32_vmx_basic_t;

#define IA32_VMX_PINBASED_CTLS 0x481
#define IA32_VMX_TRUE_PINBASED_CTLS 0x48D

#define IA32_VMX_PROCBASED_CTLS 0x482
#define IA32_VMX_TRUE_PROCBASED_CTLS 0x48E

#define IA32_VMX_PROCBASED_CTLS2 0x48B
#define IA32_VMX_PROCBASED_CTLS3 0x492

#define IA32_VMX_EXIT_CTLS 0x483
#define IA32_VMX_TRUE_EXIT_CTLS 0x48F

#define IA32_VMX_EXIT_CTLS2 0x493

#define IA32_VMX_ENTRY_CTLS 0x484
#define IA32_VMX_TRUE_ENTRY_CTLS 0x490

#define IA32_VMX_VMFUNC 0x491

#define IA32_VMX_MISC 0x485
typedef union
{
    u64 val;
    struct
    {
        u64 preempt_timer_tsc_relation : 5;
        u64 lma_store : 1;
        u64 activity_state : 3;
        u64 reserved0 : 5;
        u64 pt : 1;
        u64 smm_reads_smbase : 1;
        u64 cr3_target_num : 9;
        u64 msr_store_list_recommended_no : 3;
        u64 can_block_smi_vmxoff : 1;
        u64 vmwrite_supported : 1;
        u64 exception_injection_supported : 1;
        u64 reserved1 : 1;
        u64 mseg_revision_id : 32;
    } fields;
} ia32_vmx_misc_t;

#define IA32_VMX_CR0_FIXED0 0x486
#define IA32_VMX_CR0_FIXED1 0x487
#define IA32_VMX_CR4_FIXED0 0x488
#define IA32_VMX_CR4_FIXED1 0x489

#define IA32_VMX_VMCS_ENUM 0x48A
typedef union
{
    u32 val;
    struct
    {
        u32 access_type : 1;
        u32 index : 9;
        u32 field_type : 2;
        u32 reserved0 : 1;
        u32 field_width : 2;
        u32 reserved1 : 17;
    } fields;
} ia32_vmx_vmcs_enum_t;

#define IA32_VMX_EPT_VPID_CAP 0x48C
typedef union
{
    u64 val;
    struct
    {
        u64 execute_only_transitions_supported : 1;
        u64 reserved0 : 5;
        u64 pagewalk_len_4_supported : 1;
        u64 pagewalk_len_5_supported : 1;
        u64 ept_uc_supported : 1;
        u64 reserved1 : 5;
        u64 ept_wb_supported : 1;
        u64 reserved2 : 1;
        u64 pde_2mb_page_supported : 1;
        u64 pdpte_1gb_page_supported : 1;
        u64 reserved3 : 2;
        u64 invept_supported : 1;
        u64 ept_accessed_dirty_supported : 1;
        u64 ept_violation_adv_reports : 1;
        u64 ept_s_cet_supported : 1;
        u64 reserved4 : 1;
        u64 single_ctx_invept_supported : 1;
        u64 all_ctx_invept_supported : 1;
        u64 reserved5 : 5;
        u64 invvpid_supported : 1;
        u64 reserved6 : 7;
        u64 individual_addr_invvpid_supported : 1;
        u64 single_ctx_invvpid_supported : 1;
        u64 all_ctx_invvpid_supported : 1;
        u64 single_ctx_rg_invvpid_supported : 1;
        u64 reserved7 : 4;
        u64 hlat_prefix_size : 6;
        u64 reserved8 : 10;
    } fields;
} ia32_vmx_ept_vpid_cap_t;

typedef enum
{
    EPT_UC,
    EPT_WC,
    EPT_RESERVED0,
    EPT_RESERVED1,
    EPT_WT,
    EPT_WP,
    EPT_WB,
    EPT_CACHING_POLICY_GUARD
} ept_caching_policy_t;

typedef union
{
    u64 val;
    struct
    {
        u64 memtype : 3;
        u64 walklength : 3;
        u64 accessed_dirty_enabled : 1;
        u64 ss_ar_enforcement : 1;
        u64 reserved0 : 4;
        u64 phys : 52;
    } fields;
} eptp_t;

typedef union
{
    u64 val;
    struct
    {
        u64 read : 1;
        u64 write : 1;
        u64 execute : 1;
        u64 reserved0 : 5;
        u64 accessed : 1;
        u64 reserved1 : 1;
        u64 user_execute : 1;
        u64 reserved2 : 1;
        u64 pdpt_phys : 52;
    } fields;
} ept_pml4e_t;

typedef union
{
    u64 val;
    struct
    {
        u64 read : 1;
        u64 write : 1;
        u64 execute : 1;
        u64 memtype : 3;
        u64 ignore_pat : 1;
        u64 is_page_1gb : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 user_execute : 1;
        u64 reserved0 : 1;
        u64 reserved1 : 18;
        u64 pfn : 22;
        u64 reserved2 : 5;
        u64 guest_pg_verification : 1;
        u64 pg_write_access : 1;
        u64 reserved3 : 1;
        u64 supervisor_shadow_stack : 1;
        u64 reserved4 : 2;
        u64 suppress_ve : 1;
    } fields;
} ept_pdpte_1gb_t;

typedef union
{
    u64 val;
    struct
    {
        u64 read : 1;
        u64 write : 1;
        u64 execute : 1;
        u64 reserved0 : 7;
        u64 user_execute : 1;
        u64 reserved1 : 1;
        u64 pd_phys : 52;
    } fields;
} ept_pdpte_t;

typedef union
{
    u64 val;
    struct
    {
        u64 read : 1;
        u64 write : 1;
        u64 execute : 1;
        u64 memtype : 3;
        u64 ignore_pat : 1;
        u64 is_page_2mb : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 user_execute : 1;
        u64 reserved0 : 10;
        u64 pfn : 30;
        u64 reserved1 : 6;
        u64 guest_pg_verification : 1;
        u64 pg_write_accesste : 1;
        u64 reserved2 : 1;
        u64 supervisor_shadow_stack : 1;
        u64 reserved3 : 2;
        u64 suppress_ve : 1;
    } fields;
} ept_pde_2mb_t;

typedef union
{
    u64 val;
    struct
    {
        u64 read : 1;
        u64 write : 1;
        u64 execute : 1;
        u64 reserved0 : 7;
        u64 user_execute : 1;
        u64 reserved1 : 1;
        u64 pt_phys : 52;
    } fields;
} ept_pde_t;

typedef union
{
    u64 val;
    struct
    {
        u64 read : 1;
        u64 write : 1;
        u64 execute : 1;
        u64 memtype : 3;
        u64 ignore_pat : 1;
        u64 reserved0 : 1;
        u64 accessed : 1;
        u64 dirty : 1;
        u64 user_execute : 1;
        u64 reserved1 : 1;
        u64 pfn : 39;
        u64 reserved2 : 6;
        u64 guest_pg_verification : 1;
        u64 pg_write_access : 1;
        u64 reserved3 : 1;
        u64 supervisor_shadow_stack : 1;
        u64 sub_pg_write_permissions : 1;
        u64 reserved4 : 1;
        u64 suppress_ve : 1;
    } fields;
} ept_pte_t;

#define IA32_PRMRR_PHYS_BASE 0x1f4
typedef union 
{
    u64 val;
    struct 
    {
        u64 memtype : 3;
        u64 reserved0 : 9;
        u64 base : 34;
        u64 reserved1 : 18;
    } fields;
} ia32_prmrr_phys_base_t;

#define IA32_PRMRR_PHYS_MASK 0x1f5
typedef union 
{
    u64 val;
    struct 
    {
        u64 reserved0 : 10;
        u64 lock : 1;
        u64 valid : 1;
        u64 mask : 34;
        u64 reserved1 : 18; 
    } fields;
} ia32_prmrr_phys_mask_t;

#define IA32_MTRRCAP 0xFE
typedef union
{
    u64 val;
    struct
    {
        u64 vcnt : 8;
        u64 fixed_range_mtrrs : 1;
        u64 reserved0 : 1;
        u64 wc : 1;
        u64 smrr : 1;
        u64 prmrr : 1;
        u64 reserved1 : 51;
    } fields;
} ia32_mtrrcap_t;

typedef enum
{
    IA32_MTRR_PHYSBASE0 = 0x200,
    IA32_MTRR_PHYSMASK0,
    IA32_MTRR_PHYSBASE1,
    IA32_MTRR_PHYSMASK1,
    IA32_MTRR_PHYSBASE2,
    IA32_MTRR_PHYSMASK2,
    IA32_MTRR_PHYSBASE3,
    IA32_MTRR_PHYSMASK3,
    IA32_MTRR_PHYSBASE4,
    IA32_MTRR_PHYSMASK4,
    IA32_MTRR_PHYSBASE5,
    IA32_MTRR_PHYSMASK5,
    IA32_MTRR_PHYSBASE6,
    IA32_MTRR_PHYSMASK6,
    IA32_MTRR_PHYSBASE7,
    IA32_MTRR_PHYSMASK7,
    IA32_MTRR_PHYSBASE8,
    IA32_MTRR_PHYSMASK8,
    IA32_MTRR_PHYSBASE9,
    IA32_MTRR_PHYSMASK9,
} ia32_mtrr_phys_t;

typedef union
{
    u64 val;
    struct
    {
        u64 type : 8;
        u64 reserved0 : 4;
        u64 physbase : 52;
    } fields;
} ia32_mtrr_physbase_t;

typedef union
{
    u64 val;
    struct
    {
        u64 reserved0 : 11;
        u64 valid : 1;
        u64 physmask : 52;
    } fields;
} ia32_mtrr_physmask_t;

#define IA32_MTRR_FIX64K_00000 0x250

#define IA32_MTRR_FIX16K_80000 0x258
#define IA32_MTRR_FIX16K_A0000 0x259

typedef enum
{
    IA32_MTRR_FIX4K_C0000 = 0x268,
    IA32_MTRR_FIX4K_C8000,
    IA32_MTRR_FIX4K_D0000,
    IA32_MTRR_FIX4K_D8000,
    IA32_MTRR_FIX4K_E0000,
    IA32_MTRR_FIX4K_E8000,
    IA32_MTRR_FIX4K_F0000,
    IA32_MTRR_FIX4K_F8000,
} ia32_mtrr_fix4k_t;

#define IA32_MTRR_DEF_TYPE 0x2ff
typedef union
{
    u64 val;
    struct
    {
        u64 default_memtype : 3;
        u64 reserved0 : 7;
        u64 fixed_range_mtrr_enable : 1;
        u64 mtrr_enable : 1;
        u64 reserved1 : 52;
    } fields;
} ia32_mtrr_def_type_t;

struct vmxon_region
{
    union
    {
        u32 val;
        struct
        {
            u32 revision_id : 31;
            u32 reserved0 : 1;
        } fields;
    } header;
    u32 abort_id;

    char data[VMXON_REGION_SIZE - sizeof(u64)];
} __packed;
SIZE_ASSERT(struct vmxon_region, VMXON_REGION_SIZE);

#define HOST_SELECTOR_MASK 0xF8

typedef union
{
    u32 val;
    struct
    {
        u32 basic_reason : 16;
        u32 cleared_to_0 : 1;
        u32 undefined0 : 8;
        u32 shadow_stack_busy : 1;
        u32 bus_lock_assert : 1;
        u32 enclave_mode : 1;
        u32 pending_mtf : 1;
        u32 exit_from_root : 1;
        u32 undefined1 : 1;
        u32 vmentry_failure : 1;
    } fields;
} vmexit_reason_t;

typedef enum
{
    active,
    hlt,
    shutdown,
    wait_for_sipi,
} guest_activity_state_t;

typedef union
{
    u32 val;
    struct
    {
        u32 divide_error : 1;
        u32 debug : 1;
        u32 nmi_interrupt : 1;
        u32 breakpoint : 1;
        u32 overflow : 1;
        u32 bound_range_exceeded : 1;
        u32 invalid_opcode : 1;
        u32 device_not_available : 1;
        u32 double_fault : 1;
        u32 coprocessor_segment_overrun : 1;
        u32 invalid_tss : 1;
        u32 segment_not_present : 1;
        u32 stack_segment_fault : 1;
        u32 general_protection : 1;
        u32 page_fault : 1;
        u32 reserved0 : 1;
        u32 floating_point_error : 1;
        u32 alignment_check : 1;
        u32 machine_check : 1;
        u32 simd_floating_point_exception : 1;
        u32 virtualization_exception : 1;
        u32 control_protection_exception : 1;
        u32 reserved1 : 10;
    } fields;
} vmx_exception_bitmap_t;

typedef union
{
    u32 val;
    struct
    {
        u32 external_interrupt_exiting : 1;
        u32 reserved0 : 2;
        u32 nmi_exiting : 1;
        u32 reserved1 : 1;
        u32 virtual_nmis : 1;
        u32 activate_vmx_preemption_timer : 1;
        u32 process_posted_interrupts : 1;
        u32 reserved2 : 24;
    } fields;
} vmx_pinbased_ctls_t;

typedef union
{
    u32 val;
    struct
    {
        u32 reserved0 : 2;
        u32 interrupt_window_exiting : 1;
        u32 use_tsc_offsetting : 1;
        u32 reserved1 : 3;
        u32 hlt_exiting : 1;
        u32 reserved2 : 1;
        u32 invlpg_exiting : 1;
        u32 mwait_exiting : 1;
        u32 rdpmc_exiting : 1;
        u32 rdtsc_exiting : 1;
        u32 reserved3 : 2;
        u32 cr3_load_exiting : 1;
        u32 cr3_store_exiting : 1;
        u32 activate_tertiary_controls : 1;
        u32 reserved4 : 1;
        u32 cr8_load_exiting : 1;
        u32 cr8_store_exiting : 1;
        u32 use_tpr_shadow : 1;
        u32 nmi_window_exiting : 1;
        u32 mov_dr_exiting : 1;
        u32 unconditional_io_exiting : 1;
        u32 use_io_bitmaps : 1;
        u32 reserved_5 : 1;
        u32 monitor_trap_flag : 1;
        u32 use_msr_bitmaps : 1;
        u32 monitor_exiting : 1;
        u32 pause_exiting : 1;
        u32 activate_secondary_controls : 1;
    } fields;
} vmx_procbased_ctls_t;

typedef union
{
    u32 val;
    struct
    {
        u32 virtualize_apic_accesses : 1;
        u32 enable_ept : 1;
        u32 descriptor_table_exiting : 1;
        u32 enable_rdtscp : 1;
        u32 virtualize_x2apic_mode : 1;
        u32 enable_vpid : 1;
        u32 wbinvd_exiting : 1;
        u32 unrestricted_guest : 1;
        u32 apic_register_virtualization : 1;
        u32 virtual_interrupt_delivery : 1;
        u32 pause_loop_exiting : 1;
        u32 rdrand_exiting : 1;
        u32 enable_invpcid : 1;
        u32 enable_vm_functions : 1;
        u32 vmcs_shadowing : 1;
        u32 enable_encls_exiting : 1;
        u32 rdseed_exiting : 1;
        u32 enable_pml : 1;
        u32 ept_violation_ve : 1;
        u32 conceal_vmx_from_pt : 1;
        u32 enable_xsaves_xrstors : 1;
        u32 pasid_translation : 1;
        u32 mode_based_ept_execute : 1;
        u32 ept_sub_page_write_permissions : 1;
        u32 intel_pt_use_guest_phys_addr : 1;
        u32 use_tsc_scaling : 1;
        u32 enable_user_wait_and_pause : 1;
        u32 enable_pconfig : 1;
        u32 enable_enclv_exiting : 1;
        u32 reserved0 : 1;
        u32 vmm_bus_lock_detection : 1;
        u32 instruction_timeout : 1;
    } fields;
} vmx_procbased_ctls2_t;

typedef union
{
    u64 val;
    struct
    {
        u64 loadiwkey_exiting : 1;
        u64 enable_hlat : 1;
        u64 ept_paging_write_control : 1;
        u64 guest_paging_verification : 1;
        u64 ipi_virtualization : 1;
        u64 reserved0 : 1;
        u64 enable_msr_list_instructions : 1;
        u64 virtualize_ia32_spec_ctrl : 1;
        u64 reserved1 : 56;
    } fields;
} vmx_procbased_ctls3_t;

typedef union
{
    u32 val;
    struct
    {
        u32 reserved0 : 2;
        u32 save_debug_controls : 1;
        u32 reserved1 : 6;
        u32 host_address_space_size : 1;
        u32 reserved2 : 2;
        u32 load_ia32_perf_global_ctrl : 1;
        u32 reserved3 : 2;
        u32 acknowledge_interrupt_on_exit : 1;
        u32 reserved4 : 2;
        u32 save_ia32_pat : 1;
        u32 load_ia32_pat : 1;
        u32 save_ia32_efer : 1;
        u32 load_ia32_efer : 1;
        u32 save_vmx_preemption_timer : 1;
        u32 clear_ia32_bndcfgs : 1;
        u32 conceal_vmx_from_pt : 1;
        u32 clear_ia32_rtit_ctl : 1;
        u32 clear_ia32_lbr_ctl : 1;
        u32 clear_uinv : 1;
        u32 load_cet_state : 1;
        u32 load_pkrs : 1;
        u32 save_ia32_perf_global_ctl : 1;
        u32 activate_secondary_controls : 1;
    } fields;
} vmx_exit_ctls_t;

typedef union
{
    u32 val;
    struct
    {
        u32 save_fred : 1;
        u32 load_fred : 1;
        u32 reserved0 : 1;
        u32 prematurely_busy_shadow_stack : 1;
        u32 reserved1 : 28;
    } fields;
} vmx_exit_ctls2_t;

typedef union
{
    u32 val;
    struct
    {
        u32 reserved0 : 2;
        u32 load_debug_controls : 1;
        u32 reserved1 : 6;
        u32 ia32e_mode_guest : 1;
        u32 entry_to_smm : 1;
        u32 deactivate_dual_monitor_treatment : 1;
        u32 reserved2 : 1;
        u32 load_ia32_perf_global_ctrl : 1;
        u32 load_ia32_pat : 1;
        u32 load_ia32_efer : 1;
        u32 load_ia32_bndcfgs : 1;
        u32 conceal_vmx_from_pt : 1;
        u32 load_ia32_rtit_ctl : 1;
        u32 load_uinv : 1;
        u32 load_cet_state : 1;
        u32 load_ia32_lbr_ctl : 1;
        u32 load_pkrs : 1;
        u32 load_fred : 1;
        u32 reserved3 : 8;
    } fields;
} vmx_entry_ctls_t;

struct vmcs_region
{
    union
    {
        u32 val;
        struct
        {
            u32 revision_id : 31;
            u32 shadow_vmcs_indicator : 1;
        } fields;

    } header;

    u32 abort_id;

    char data[VMCS_REGION_SIZE - sizeof(u64)];

} __packed;
SIZE_ASSERT(struct vmcs_region, VMCS_REGION_SIZE);

struct msr_entry
{
    u32 msr_index;
    u32 reserved0;
    u64 msr_data;
} __packed;
SIZE_ASSERT(struct msr_entry, 16);

struct ve_info_area
{
    
    u32 reason;
    u32 reserved0;
    u64 exit_qualification;
    u64 guest_linear_address;
    u64 guest_physical_address;
    u16 ept_index;
    char data[VMCS_REGION_SIZE - 34];
} __packed;

SIZE_ASSERT(struct ve_info_area, 4096);

typedef enum
{
    EXIT_REASON_EXCEPTION,
    EXIT_REASON_EXT_INTR,
    EXIT_REASON_TRIPLE_FAULT,
    EXIT_REASON_INIT,
    EXIT_REASON_SIPI,
    EXIT_REASON_IO_SMI,
    EXIT_REASON_SMI,
    EXIT_REASON_INTR_WINDOW,
    EXIT_REASON_NMI_WINDOW,
    EXIT_REASON_TASK_SWITCH,
    EXIT_REASON_CPUID,
    EXIT_REASON_GETSEC,
    EXIT_REASON_HLT,
    EXIT_REASON_INVD,
    EXIT_REASON_INVLPG,
    EXIT_REASON_RDPMC,
    EXIT_REASON_RDTSC,
    EXIT_REASON_RSM,
    EXIT_REASON_VMCALL,
    EXIT_REASON_VMCLEAR,
    EXIT_REASON_VMLAUNCH,
    EXIT_REASON_VMPTRLD,
    EXIT_REASON_VMPTRST,
    EXIT_REASON_VMREAD,
    EXIT_REASON_VMRESUME,
    EXIT_REASON_VMWRITE,
    EXIT_REASON_VMXOFF,
    EXIT_REASON_VMXON,
    EXIT_REASON_CR_ACCESS,
    EXIT_REASON_DR_ACCESS,
    EXIT_REASON_INOUT,
    EXIT_REASON_RDMSR,
    EXIT_REASON_WRMSR,
    EXIT_REASON_VMENTRY_INVAL_VMCS,
    EXIT_REASON_VMENTRY_MSR_LOADING,
    EXIT_REASON_MWAIT = 36,
    EXIT_REASON_MTF,
    EXIT_REASON_MONITOR = 39,
    EXIT_REASON_PAUSE,
    EXIT_REASON_MCE_DURING_ENTRY,
    EXIT_REASON_TPR = 43,
    EXIT_REASON_APIC_ACCESS,
    EXIT_REASON_VIRTUALIZED_EOI,
    EXIT_REASON_GDTR_IDTR,
    EXIT_REASON_LDTR_TR,
    EXIT_REASON_EPT_FAULT,
    EXIT_REASON_EPT_MISCONFIG,
    EXIT_REASON_INVEPT,
    EXIT_REASON_RDTSCP,
    EXIT_REASON_VMX_PREEMPT,
    EXIT_REASON_INVVPID,
    EXIT_REASON_WBINVD,
    EXIT_REASON_XSETBV,
    EXIT_REASON_APIC_WRITE,
    EXIT_REASON_RDRAND,
    EXIT_REASON_INVPCID,
    EXIT_REASON_VMFUNC,
    EXIT_REASON_ENCLS,
    EXIT_REASON_RDSEED,
    EXIT_REASON_PGMOD_LOG_FULL,
    EXIT_REASON_XSAVES,
    EXIT_REASON_XRSTORS,
    EXIT_REASON_PCONFIG,
    EXIT_REASON_SPP,
    EXIT_REASON_UMWAIT,
    EXIT_REASON_TPAUSE,
    EXIT_REASON_LOADIWKEY,
    EXIT_REASON_ENCLV,
    EXIT_REASON_ENQCMD_PASID_FAILURE = 72,
    EXIT_REASON_ENQCMDS_FAILURE,
    EXIT_REASON_BUSLOCK,
    EXIT_REASON_INSTRUCTION_TIMEOUT,
    EXIT_REASON_SEAMCALL,
    EXIT_REASON_TDCALL,
    EXIT_REASON_RDMSRLIST,
    EXIT_REASON_WRMSRLIST,
} basic_vmexit_reason_t;

#define VMENTRY_INVAL_GUEST_STATE 33
#define VMENTRY_MSR_LOADING 34
#define VMENTRY_MCE 41

typedef union
{
    u64 val;
    struct
    {
        u64 read : 1;
        u64 write : 1;
        u64 execute : 1;
        u64 gpa_read : 1;
        u64 gpa_write : 1;
        u64 gpa_execute : 1;
        u64 gpa_user_execute : 1;
        u64 gla_valid : 1;
        u64 access_gpa : 1;
        u64 adv_user_linear_addr : 1;
        u64 adv_rw : 1;
        u64 adv_nx : 1;
        u64 nmi_unblocking_iret : 1;
        u64 shadow_stack_access : 1;
        u64 reserved0 : 2;
        u64 pt : 1;
        u64 reserved1 : 47;
    } fields;
} ept_qualification_t;

typedef union
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 type : 3;
        u32 deliver_errcode : 1;
        u32 reserved0 : 19;
        u32 valid : 1;
    } fields;
} vmentry_interrupt_info_t;

typedef union
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 type : 3;
        u32 errcode_valid : 1;
        u32 reserved0 : 18;
        u32 nmi_unblocking : 1;
        u32 valid : 1;
    } fields;
} vmexit_interrupt_info_t;

typedef enum
{
	INTERRUPT_TYPE_EXTERNAL = 0,
	INTERRUPT_TYPE_RESERVED = 1,
	INTERRUPT_TYPE_NMI = 2,
	INTERRUPT_TYPE_HARDWARE_EXCEPTION = 3,
	INTERRUPT_TYPE_SOFTWARE_INT = 4,
	INTERRUPT_TYPE_PRIVILEGED_SOFTWARE_EXCEPTION = 5,
	INTERRUPT_TYPE_SOFTWARE_EXCEPTION = 6,
	INTERRUPT_TYPE_OTHER_EVENT = 7
} interrupt_type_t;

struct invvpid_descriptor {
    u64 vpid;
    u64 linear_addr;
};

typedef enum 
{
    INDIVIDUAL_ADDR,
    SINGLE_CONTEXT,
    ALL_CONTEXTS,
    SINGLE_CONTEXT_NON_GLOBALS,
} invvpid_type_t;

inline void ibhf_stub(void)
{
    __asm__ __volatile__ (
        ".byte 0xf3, 0x48, 0x0f, 0x1e, 0xf8;"
    );
}

inline void ibpb_stub(void)
{
    __wrmsrl(IA32_PRED_CMD, 1);
}

inline void l1d_flush_stub(void)
{
    __wrmsrl(IA32_FLUSH_CMD, 1);
}

inline void verw_fb_clear_stub(void)
{
    __asm__ __volatile__ (
        "xorl %%eax, %%eax;"
        "verw %%ax;"
        :
        :
        :"%eax"
    );
}

bool may_be_vulnerable_to_its(void);
bool is_vmx_supported(void);

inline bool __vmxon(u64 phys)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmxon %[phys];"
        "seta %[ret];"
        : [ret] "=r"(ret)
        : [phys] "m"(phys)
        : "cc", "memory");

    return ret;
}

inline bool __vmxoff(void)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmxoff;"
        "seta %[ret];"
        : [ret] "=r"(ret)
        :
        : "cc", "memory");

    return ret;
}

inline bool __vmread(u64 field, u64 *outp)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmread %[field], %[outp];"
        "seta %[ret];"
        : [ret] "=r"(ret), [outp] "=rm"(*outp)
        : [field] "r"(field)
        : "cc", "memory");

    return ret;
}

inline bool __vmwrite(u64 field, u64 inp)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmwrite %[inp], %[field];"
        "seta %[ret];"
        : [ret] "=r"(ret)
        : [field] "r"(field), [inp] "rm"(inp)
        : "cc", "memory");

    return ret;
}

inline bool __vmread32(u64 field, u32 *outp)
{
    u64 val = 0;
    bool ret = __vmread(field, &val);
    if (ret)
        *outp = (u32)val;

    return ret;
}

inline bool __vmread16(u64 field, u16 *outp)
{
    u64 val = 0;
    bool ret = __vmread(field, &val);
    if (ret)
        *outp = (u16)val;

    return ret;
}

inline u64 vmread(u64 field)
{
    u64 val = 0;
    __vmread(field, &val);
    return val;
}

inline u32 vmread32(u64 field)
{
    u32 val = 0;
    __vmread32(field, &val);
    return val;
}

inline u16 vmread16(u64 field)
{
    u16 val = 0;
    __vmread16(field, &val);
    return val;
}

inline bool __vmptrld(u64 phys)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmptrld %[phys];"
        "seta %[ret];"
        : [ret] "=r"(ret)
        : [phys] "m"(phys)
        : "cc", "memory");

    return ret;
}

inline bool __vmptrst(u64 *outp)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmptrst %[phys];"
        "seta %[ret];"
        : [phys] "=m"(*outp), [ret] "=r"(ret)
        :
        : "cc", "memory");

    return ret;
}

inline bool __vmresume(void)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmresume;"
        "seta %[ret];"
        : [ret] "=r"(ret)
        :
        : "cc");

    return ret;
}

inline bool __vmclear(u64 phys)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmclear %[phys];"
        "seta %[ret];"
        : [ret] "=r"(ret)
        : [phys] "m"(phys)
        : "cc", "memory");

    return ret;
}

inline bool __vmlaunch(void)
{
    u8 ret = 0;
    __asm__ __volatile__(
        "vmlaunch;"
        "seta %[ret];"
        : [ret] "=r"(ret)
        :
        : "cc");

    return ret;
}

inline void invvpid_single(u64 vpid)
{
    struct invvpid_descriptor desc = {
        .vpid = vpid
    };

    u64 type = SINGLE_CONTEXT;
    
    __asm__ __volatile__ ("invvpid %0, %1" ::"m"(desc), "r"(type):"memory");
}

typedef union 
{
    u32 val;
    struct 
    {
        u32 cr : 4;
        u32 access_type : 2;
        u32 lmsw_operand_type : 1;
        u32 reserved0 : 1;
        u32 gpr : 4;
        u32 reserved1 : 4;
        u32 lmsw_source : 16;
    } fields;
} cr_access_qualification_t;

typedef enum 
{
    MOV_TO_CR,
    MOV_FROM_CR,
    CLTS,
    LMSW
} cr_access_type_t;

#define LMSW_OPERAND_REG 0
#define LMSW_OPERAND_MEM 1

typedef enum 
{
    RAX,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
} gpr_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 size : 3;
        u32 direction : 1;
        u32 string_instruction : 1;
        u32 rep : 1;
        u32 operand_encoding : 1;
        u32 reserved0 : 9;
        u32 port : 16;
    } fields;
} io_qualification_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 undefined0 : 7;
        u32 address_size : 3;
        u32 undefined1 : 5;
        u32 segment_selector : 3;
        u32 undefined2 : 14;
    } fields;
} ins_outs_info_t;

typedef enum 
{
    IO_BITS_8,
    IO_BITS_16,
    IO_BITS_32
} io_access_size_t;

#define IO_OUT 0
#define IO_IN 1

#define OPERAND_DX 0
#define OPERAND_IMMEDIATE 1

typedef union 
{
    u32 val;
    struct 
    {
        u32 scaling : 2;
        u32 undefined0 : 5;
        u32 address_size : 3;
        u32 zero : 1;
        u32 operand_size : 1;
        u32 undefined1 : 3;
        u32 segment_selector : 3;
        u32 index_gpr : 4;
        u32 index_gpr_invalid : 1;
        u32 base_gpr : 4;
        u32 base_gpr_invalid : 1;
        u32 identity : 2;
        u32 undefined2 : 2;
    } fields;
} gdtr_idtr_info_t;

typedef enum 
{
    NO_SCALING,
    SCALE_2,
    SCALE_4,
    SCALE_8
} scaling_t;

typedef enum 
{
    ADDR_BITS_16,
    ADDR_BITS_32,
    ADDR_BITS_64
} address_size_t;

typedef enum 
{
    ES,
    CS,
    SS,
    DS,
    FS,
    GS
} segment_selector_t;

typedef enum 
{
    SGDT,
    SIDT,
    LGDT,
    LIDT
} gdtr_idtr_identity_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 scaling : 2;
        u32 undefined0 : 1;
        u32 gpr : 4;
        u32 address_size : 3;
        u32 mem_reg : 1;
        u32 undefined1 : 4;
        u32 segment_selector : 3;
        u32 index_gpr : 4;
        u32 index_gpr_invalid : 1;
        u32 base_gpr : 4;
        u32 base_gpr_invalid : 1;
        u32 identity : 2;
        u32 undefined2 : 2;
    } fields;
} ldtr_tr_info_t;

#define LDTR_TR_MEM 0
#define LDTR_TR_REG 1

typedef enum 
{
    SLDT,
    STR,
    LLDT,
    LTR
} ldtr_tr_identity_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector : 8;
        u32 vectored_event_type : 3;
        u32 errcode_delivered : 1;
        u32 nmi_unblocking : 1;
        u32 reserved0 : 18;
        u32 valid : 1;
    } fields;
} vectored_events_info_t;

typedef union
{
    u32 val;
    struct 
    {
        u32 sti_blocking : 1;
        u32 mov_ss_blocking : 1;
        u32 smi_blocking : 1;
        u32 nmi_blocking : 1;
        u32 enclave_interruption : 1;
        u32 reserved0 : 27;
    } fields;
} guest_interruptibility_state_t;

typedef union 
{
    u64 val;
    struct 
    {
        u64 drx : 4;
        u64 undefined0 : 7;
        u64 bus_lock_detected : 1;
        u64 undefined1 : 1;
        u64 debug_reg_access_detected : 1;
        u64 single_step : 1;
        u64 undefined2 : 1;
        u64 rtm : 1;
        u64 reserved0 : 47;
    } fields;
} debug_exception_qualification_t;

#endif