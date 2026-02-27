#define ASM_FILE 1

#include <generated/autoconf.h>
#if CONFIG_DEMO_PV_TWANVISOR

.globl pv_twanvisor_demo_guest_start
.type pv_twanvisor_demo_guest_start, @object

.globl pv_twanvisor_demo_guest_end
.type pv_twanvisor_demo_guest_end, @object

.globl pv_shared_buf_start
.type pv_shared_buf_start, @object

.globl _guest_start
.type _guest_start, @object

.align 64
.section .text

.code16

pv_twanvisor_demo_guest_start:

/* shared data */

pv_shared_buf_start:
    .zero 64
pv_shared_buf_end:

/* guest code */

.align 64
_guest_start:

    xorw %bx, %bx
    
    movw %bx, %ds
 
    movb $'h', %ds:1(%bx)  
    movb $'e', %ds:2(%bx)  
    movb $'l', %ds:3(%bx)  
    movb $'l', %ds:4(%bx)  
    movb $'o', %ds:5(%bx)  
    movb $' ', %ds:6(%bx)  
    movb $'f', %ds:7(%bx)  
    movb $'r', %ds:8(%bx)  
    movb $'o', %ds:9(%bx)  
    movb $'m', %ds:10(%bx)  
    movb $' ', %ds:11(%bx)  
    movb $'g', %ds:12(%bx)  
    movb $'u', %ds:13(%bx)  
    movb $'e', %ds:14(%bx)  
    movb $'s', %ds:15(%bx)  
    movb $'t', %ds:16(%bx)  
    movb $'\n', %ds:17(%bx)  

    movb $1, %ds:(%bx)  

.meow:
    pause
    jmp .meow

pv_twanvisor_demo_guest_end:

#endif