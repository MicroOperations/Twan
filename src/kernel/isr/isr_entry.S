#define ASM_FILE 1
#include <include/kernel/isr/isr_index.h>
#include <include/arch.h>

#define ISR_ENTRY_PREFIX __isr_entry_

// void __isr_dispatcher(struct interrupt_info *stack_trace);
.extern __isr_dispatcher

// u64 get_int_stack(u8 vector)
.extern get_int_stack

.globl isr_entry_table
.type isr_entry_table, @object

// void __emulate_interrupt(u64 rsp, u8 vector, u64 errcode)
.globl __emulate_interrupt
.type __emulate_interrupt, @function

.align 64
.section .text

__do_isr_entry:

    cld

    SETUP_ISR_ENTRY

    movq %rsp, %rbp
    movq %rsp, %rdi

    call get_int_stack

    movq %rax, %rsp
    movq %rax, %rbp
    movq %rax, %rdi
    
    call __isr_dispatcher

    SETUP_ISR_EXIT
    
    iretq
    int3

.align 64
__emulate_interrupt:

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

    jmp __do_isr_entry

.rt_ptr:
    
    popfq
    ret
    int3

GEN_ISR_ENTRIES ISR_ENTRY_PREFIX, __do_isr_entry

.align 64
.section .data

isr_entry_table: GEN_ISR_TABLE ISR_ENTRY_PREFIX