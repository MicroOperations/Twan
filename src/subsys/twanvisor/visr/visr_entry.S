#define ASM_FILE 1

#include <include/subsys/twanvisor/vconf.h>
#if TWANVISOR_ON

#include <include/arch.h>

#define VISR_ENTRY_PREFIX __visr_entry_

// void __visr_dispatcher(struct interrupt_info *stack_trace);
.extern __visr_dispatcher

.globl visr_entry_table
.type visr_entry_table, @object

// void __vemulate_interrupt(u64 rsp, u8 vector, u64 errcode)
.globl __vemulate_interrupt
.type __vemulate_interrupt, @function

.align 64
.section .text

__vdo_isr_entry:

    cld

    SETUP_ISR_ENTRY

    movq %rsp, %rdi
    call __visr_dispatcher

    SETUP_ISR_EXIT
    
    iretq
    int3

.align 64
__vemulate_interrupt:

    movzx %sil, %esi
    orl $(1 << 31), %esi

    pushfq 
    cli

    movq %rsp, %rcx
    movq %rdi, %rsp

    leaq .rt_ptr(%rip), %r8

    movl %ss, %r9d
    movl %cs, %r10d

    pushq %r9
    pushq %rcx
    pushfq
    pushq %r10
    pushq %r8
    pushq %rdx
    pushq %rsi

    jmp __vdo_isr_entry

.rt_ptr:
    
    popfq
    ret
    int3

GEN_ISR_ENTRIES VISR_ENTRY_PREFIX, __vdo_isr_entry

.align 64
.section .data

visr_entry_table: GEN_ISR_TABLE VISR_ENTRY_PREFIX

#endif