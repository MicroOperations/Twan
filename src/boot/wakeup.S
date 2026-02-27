#define ASM_FILE 1
#include <kernel/boot.h>

.extern pml4
.extern gdtr64
.extern __trampoline

.globl __wakeup
.type __wakeup, @function

.globl ap_rsp
.type ap_rsp, @object

.globl __wakeup_end
.type __wakeup_end, @notype

.globl __wakeup_blob_size
.type __wakeup_blob_size, @object

.globl __wakeup_blob_save_area
.type __wakeup_blob_save_area, @object

.align 64
.section .text

.code16

__wakeup:

    cli

    lgdt WAKEUP_ADDR + (gdtr32 - __wakeup)

    movl %cr0, %eax
    orl $(PE_MASK), %eax
    movl %eax, %cr0

    ljmp $0x8, $(WAKEUP_ADDR + (__wakeup32 - __wakeup))

.code32

.align 64
__wakeup32:

    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %ss

    /* disable paging and enable mp */
    movl %cr0, %eax
    orl $(MP_MASK | NE_MASK), %eax
    andl $(~PG_MASK), %eax
    movl %eax, %cr0

    /* load our pml4 */
    movl $pml4, %ecx
    movl %ecx, %cr3

    /* enable pae */
    movl %cr4, %edx
    orl $(PAE_MASK | OSFXSR_MASK | OSXMMEXCPT_MASK), %edx
    movl %edx, %cr4

    /* enable longmode */
    movl $IA32_EFER, %ecx
    rdmsr
    orl $LME_MASK, %eax
    wrmsr

    /* enable paging and wp */
    movl %cr0, %eax
    orl $(PG_MASK | WP_MASK), %eax
    movl %eax, %cr0

    /* load our longmode gdt */
    lgdt gdtr64

    /* jump to longmode */
    ljmp $0x8, $(WAKEUP_ADDR + (__wakeup64 - __wakeup))

.code64

.align 64
__wakeup64:

    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %ss
    movl %eax, %fs
    movl %eax, %gs

    /* pass NULL to multiboot info and false as bsp */
    xorl %edi, %edi
    xorl %esi, %esi

    /* gotta load the stack ptr */
    leaq ap_rsp(%rip), %rax
    movq (%rax), %rsp
    movq %rsp, %rbp

    /* absolute jump to the trampoline */
    movabsq $__trampoline, %rcx
    jmpq *%rcx

/* gotta keep shit in .text so the assembler doesnt relocate them */

.align 64
gdt32_base:
    .quad 0
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt32_end:

.align 64
gdtr32:
    .word gdt32_end - gdt32_base - 1
    .long WAKEUP_ADDR + (gdt32_base - __wakeup)

.align 64
ap_rsp: .quad 0

__wakeup_end:

.align 64
.section .data
__wakeup_blob_size: .quad __wakeup_end - __wakeup

.align 64
.section .bss
__wakeup_blob_save_area: .skip __wakeup_end - __wakeup