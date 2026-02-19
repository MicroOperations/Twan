#ifndef _VARCH_H_
#define _VARCH_H_

#include <include/arch.h>

#ifdef ASM_FILE

.macro PUSH_VREGS

    PUSH_REGS

    push %rax
    movq %cr2, %rax
    xchgq %rax, (%rsp)

    push %rax
    movq %dr6, %rax
    xchgq %rax, (%rsp)

    push %rax
    movq %dr3, %rax
    xchgq %rax, (%rsp)

    push %rax
    movq %dr2, %rax
    xchgq %rax, (%rsp)

    push %rax
    movq %dr1, %rax
    xchgq %rax, (%rsp)

    push %rax
    movq %dr0, %rax
    xchgq %rax, (%rsp)

    lfence

.endm

.macro POP_VREGS

    popq %rax
    movq %rax, %dr0

    popq %rax
    movq %rax, %dr1

    popq %rax
    movq %rax, %dr2

    popq %rax
    movq %rax, %dr3

    popq %rax
    movq %rax, %dr6

    popq %rax
    movq %rax, %cr2
    
    POP_REGS

.endm

#else

struct vregs
{
    u64 dr0;
    u64 dr1;
    u64 dr2;
    u64 dr3;
    u64 dr6;
    u64 cr2;
    struct regs regs;
} __packed;

SIZE_ASSERT(struct vregs, sizeof(struct regs) + (sizeof(u64) * 6));

#endif


#endif