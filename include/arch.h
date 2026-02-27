#ifndef _ARCH_H_
#define _ARCH_H_

#include <kernel/boot.h>

#ifdef ASM_FILE

#define PAGE_SHIFT_4KB 12
#define PAGE_SIZE_4KB (1 << PAGE_SHIFT_4KB)
#define PAGE_SHIFT_2MB 21
#define PAGE_SIZE_2MB (1 << PAGE_SHIFT_2MB)

#define DEFAULT_MXCSR 0x1f80
#define NUM_VECTORS 256

.macro GEN_ISR_ENTRY, ISR_NO, prefix, entry_func
    
    .align 16
    \prefix\ISR_NO:
        pushq $0
        pushq $\ISR_NO
        jmp \entry_func

.endm

.macro GEN_ISR_ENTRY_ERRCODE, ISR_NO, prefix, entry_func
    .align 16
    \prefix\ISR_NO:
        pushq $\ISR_NO
        jmp \entry_func
.endm

.altmacro
.macro GEN_ISR_ENTRIES, prefix, entry_func

    LOCAL i
    .set i, 0

    .rept NUM_VECTORS
        .if i == 8 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 17 || i == 21
            GEN_ISR_ENTRY_ERRCODE %(i), \prefix, \entry_func
        .else
            GEN_ISR_ENTRY %(i), \prefix, \entry_func
        .endif

        .set i, i+1
    .endr

.endm

.macro GEN_ISR_TABLE_ENTRY, ISR_NO, prefix
    .quad \prefix\ISR_NO
.endm

.altmacro
.macro GEN_ISR_TABLE, prefix

    LOCAL i
    .set i, 0

    .rept NUM_VECTORS
        GEN_ISR_TABLE_ENTRY %(i), \prefix
        .set i, i+1
    .endr
    
.endm

/* stack must be 16 byte aligned before calling, this will ;eave the stack in a 
   misaligned state */
.macro PUSH_REGS

    subq $512, %rsp
    fxsave (%rsp)

    pushq %rbp
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rdi
    pushq %rsi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    pushq %rax
    movq %cr8, %rax
    xchg %rax, (%rsp)

.endm

.macro POP_REGS

    popq %rax
    movq %rax, %cr8

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rsi
    popq %rdi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    popq %rbp

    fxrstor (%rsp)
    addq $512, %rsp

.endm

.macro SETUP_ISR_ENTRY 
    push $0
    PUSH_REGS
.endm

.macro SETUP_ISR_EXIT
    POP_REGS
    addq $24, %rsp
.endm

#else 

#include <lib/x86_index.h>

typedef union
{
    u64 val;
    struct 
    {
        u64 vector : 8;
        u64 reserved0 : 23;
        u64 emulated : 1;
        u64 reserved1 : 32;
    } fields;
} dispatch_info_t;

struct regs 
{   
    u64 cr8;
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rsi;
    u64 rdi;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;
    u64 rbp;
    struct fxsave64 fxsave_region;
} __packed;

struct interrupt_info
{
    struct regs regs;
    u64 alignment;
    
    dispatch_info_t dispatch_info;
    u64 errcode;
    u64 rip;
    u64 cs;
    rflags_t rflags;
    u64 rsp;
    u64 ss;
} __packed;

struct context 
{
    u64 cr8;
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rsi;
    u64 rdi;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;
    u64 rbp; 
    u64 rsp;
    rflags_t rflags;
    u64 rip;
    struct fxsave64 fp_context;
};

#endif

#endif