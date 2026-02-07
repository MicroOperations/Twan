#define ASM_FILE 1
#include <include/multiboot2.h>
#include <include/kernel/boot.h>
#include <include/arch.h>

.extern __bss_start
.extern __bss_end

.extern __kernel_start
.extern __kernel_end

// void __noreturn __start_twan(u64 multiboot_info_phys, bool is_bsp)
.extern __start_twan

.globl _start
.type _start, @function

.globl __trampoline
.type __trampoline, @function

.globl gdtr64
.type gdtr64, @object

.globl idtr
.type idtr, @object

.globl pml4
.type pml4, @object

.globl pdpt
.type pdpt, @object

.globl pd_kernel
.type pd_kernel, @object

.globl pt_kernel_start
.type pt_kernel_start, @object

.globl idt
.type idt, @object

.section .multiboot_header, "a", @progbits
.align 64
header_start:

    .long MULTIBOOT2_HEADER_MAGIC
    .long 0
    .long header_end - header_start
    .long 0x100000000 - (MULTIBOOT2_HEADER_MAGIC + 0 + (header_end - header_start))

    .align MULTIBOOT_TAG_ALIGN
    .word MULTIBOOT_TAG_TYPE_ELF_SECTIONS
    .word 0
    .long 16  
    .long 0 

    .align MULTIBOOT_TAG_ALIGN
    .word MULTIBOOT_TAG_TYPE_MMAP      
    .word 0
    .long 16
    .long 24     
    .long 0     

    .align MULTIBOOT_TAG_ALIGN
    .word MULTIBOOT_TAG_TYPE_END
    .word 0
    .long 8

header_end:

.align 64
.section .text

.code32

_start:

    cli
    cld

    /* sanity check multiboot */
    cmp $MULTIBOOT2_BOOTLOADER_MAGIC, %eax
    jne .halt32

    /* sanity check protected mode */
    movl %cr0, %eax
    test $PE_MASK, %eax
    jz .halt32
   
    /* zero out bss */
    movl $__bss_start, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    /* map first 4kb -> 2mb for the kernel & wakeup blob */
    movl $pt_kernel_start, %edi
    movl $0x1003, %esi
    
    movl $1, %ecx
.map_loop:
    
    movl %esi, (%edi, %ecx, 8)
    addl $PAGE_SIZE_4KB, %esi

    addl $1, %ecx
    test $0x200, %ecx
    jz .map_loop

    /* link pt kernel_start to pd_kernel */
    orl $0x3, %edi
    movl %edi, pd_kernel

    movl $pd_kernel, %edx

    /* map rest of kernel mem, assuming we are mapped from 1MB - __kernel_end */
    movl $__kernel_end, %ecx
    addl $(PAGE_SIZE_2MB - 1), %ecx
    shrl $PAGE_SHIFT_2MB, %ecx

    /* TODO: somehow log fail due to kernel being too large */
    cmp $512, %ecx
    jg .halt32

    movl $0x200083, %esi
    movl $1, %eax

.map_rest_loop:

    cmp %ecx, %eax
    je .map_rest_loop_done

    movl %esi, (%edx, %eax, 8)
    addl $PAGE_SIZE_2MB, %esi

    addl $1, %eax
    jmp .map_rest_loop

.map_rest_loop_done:

    /* link pd_kernel to pdpt */
    orl $0x3, %edx
    movl %edx, pdpt + (PD_KERNEL_PDPT_IDX * 8)

    /* link pdpt to pml4 */
    movl $pdpt, %eax
    orl $0x3, %eax
    movl %eax, pml4 + (PDPT_PML4_IDX * 8)

    /* setup cr0 */
    movl %cr0, %ecx
    orl $(MP_MASK | NE_MASK), %ecx
    andl $(~PG_MASK), %ecx
    movl %ecx, %cr0

    /* set it up into cr3 */
    movl $pml4, %edx
    movl %edx, %cr3

    /* enable pae */
    movl %cr4, %eax
    orl $(PAE_MASK | DE_MASK | OSFXSR_MASK | OSXMMEXCPT_MASK), %eax
    movl %eax, %cr4

    /* enable lme */
    movl $IA32_EFER, %ecx
    rdmsr
    orl $LME_MASK, %eax
    wrmsr

    /* enable paging and wp */
    movl %cr0, %eax
    orl $(PG_MASK | WP_MASK), %eax
    andl $(~(EM_MASK | NW_MASK | CD_MASK)), %eax
    movl %eax, %cr0

    /* setup gdtr and jump to longmode */
    lgdt gdtr64
    ljmp $0x8, $_start64

.halt32:

    hlt
    jmp .halt32

.code64

.align 64
_start64:

    /* reload data selectors */
    movl $0x10, %eax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %ss
    movl %eax, %fs
    movl %eax, %gs

    /* set because we are the bsp */
    movl $1, %esi

    /* pass multiboot info */
    movq %rbx, %rdi 

    leaq boot_stack_top(%rip), %rsp
    movq %rsp, %rbp

__trampoline:

    lidt idtr

    /* null the ldtr */
    xorl %eax, %eax
    lldt %ax

    fninit

    /* load the mxcsr with the recommended known good value */
    pushq 0x1f80
    ldmxcsr (%rsp)
    popq %rax

    /* jump to c entry */
    jmp __start_twan
    
.halt64:

    hlt
    jmp .halt64

.align 64
.section .data

gdt64_base:
    .quad 0
    .quad KERNEL_CS_DESC
    .quad KERNEL_DS_DESC
gdt64_end:

.align 64
gdtr64:
    .word gdt64_end - gdt64_base - 1
    .quad gdt64_base

idtr:
    .word 4095
    .quad idt

.section .bss
.align 4096

/* kernel page tables */
pml4:               .skip 4096

/* kernel image & kernel heap */
pdpt:               .skip 4096
pd_kernel:          .skip 4096
pt_kernel_start:    .skip 4096

idt:                .skip 4096

boot_stack_bottom:
    .skip BOOT_STACK_SIZE
boot_stack_top: