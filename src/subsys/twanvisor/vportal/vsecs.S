#define ASM_FILE 1

#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

/* spectre-rsb mitigation not needed if uarch implements rsb stuffing */
#define SPECTRE_RSB_MITIGATION CONFIG_MITIGATION_SPECTRE_RSB
#define RSB_ENTRIES 32

/* 
    bhb clearing sequence not needed if BHI_DIS_S is enabled or if
    IA32_ARCH_CAPABILITIES.bhi_no is set

    pre alder lake - self explanatory 
    alder lake - clearing sequence for alder lake & saphire rapids */

#define BHB_CLEARING_SEQUENCE_NONE 0
#define BHB_CLEARING_SEQUENCE_PRE_ALDER_LAKE 1
#define BHB_CLEARING_SEQUENCE_ALDER_LAKE 2

#if CONFIG_MITIGATION_BHI

#define ACTIVE_BHB_CLEARING_SEQUENCE BHB_CLEARING_SEQUENCE_ALDER_LAKE

#else 

#define ACTIVE_BHB_CLEARING_SEQUENCE BHB_CLEARING_SEQUENCE_NONE

#endif

.macro IBHF 
    .byte 0xf3, 0x48, 0x0f, 0x1e, 0xf8
.endm

// void __vreset_state(void)
.globl __vreset_state
.type __vreset_state, @function

.align 64
.section .text

__vreset_state:

    /* prevent (speculative) reuse of architectural state */
    xorl %eax, %eax
    xorl %ebx, %ebx
    xorl %ecx, %ecx
    xorl %edx, %edx
    xorl %edi, %edi
    xorl %esi, %esi
    xorl %ebp, %ebp
    xorl %r8d, %r8d
    xorl %r9d, %r9d
    xorl %r10d, %r10d
    xorl %r11d, %r11d
    xorl %r12d, %r12d
    xorl %r13d, %r13d
    xorl %r14d, %r14d
    xorl %r15d, %r15d

    pushq 0x1f80
    ldmxcsr (%rsp)
    addq $8, %rsp

    pxor %xmm0, %xmm0 
    pxor %xmm1, %xmm1 
    pxor %xmm2, %xmm2 
    pxor %xmm3, %xmm3 
    pxor %xmm4, %xmm4 
    pxor %xmm5, %xmm5 
    pxor %xmm6, %xmm6 
    pxor %xmm7, %xmm7 
    pxor %xmm8, %xmm8 
    pxor %xmm9, %xmm9 
    pxor %xmm10, %xmm10 
    pxor %xmm11, %xmm11 
    pxor %xmm12, %xmm12 
    pxor %xmm13, %xmm13 
    pxor %xmm14, %xmm14 
    pxor %xmm15, %xmm15

    fninit
    fldz
    fst %st(1) 
    fst %st(2) 
    fst %st(3) 
    fst %st(4) 
    fst %st(5) 
    fst %st(6) 
    fst %st(7) 
    fstp %st(0)

    movq %rax, %cr8
    movq %rax, %cr2

    movq %rax, %dr6
    movq %rax, %dr3
    movq %rax, %dr2
    movq %rax, %dr1
    movq %rax, %dr0

#if ACTIVE_BHB_CLEARING_SEQUENCE == BHB_CLEARING_SEQUENCE_PRE_ALDER_LAKE

    mov $5, %ecx
    call 801f
    jmp 805f
    .align 64
801: 
    call 802f
    ret
    .align 64
802: 
    movl $5, %eax
803: 
    jmp 804f
    nop
804: 
    sub $1, %eax
    jnz 803b
    sub $1, %ecx
    jnz 801b
    ret
805:  
    lfence

#elif ACTIVE_BHB_CLEARING_SEQUENCE == BHB_CLEARING_SEQUENCE_ALDER_LAKE

    mov $12, %ecx
    call 801f
    jmp 805f
    .align 64
801:  
    call 802f
    ret
    .align 64
802:  
    movl $7, %eax
803: 
    jmp 804f
    nop
804:  
    sub $1, %eax
    jnz 803b
    sub $1, %ecx
    jnz 801b
    ret
805:  
    lfence

#endif

#if SPECTRE_RSB_MITIGATION

    mov $RSB_ENTRIES, %rcx

.fill_rsb:

    call .fill_rsb_gadget

.speculation_loop:
    pause
    lfence
    jmp .speculation_loop

.fill_rsb_gadget:
    sub $1, %rcx
    jnz .fill_rsb
    add $(RSB_ENTRIES*8), %rsp

#endif    

#if CONFIG_MITIGATION_INTRAMODE_BTI

    IBHF

#endif

    ret
    int3

#endif