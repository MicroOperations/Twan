#define ASM_FILE 1

#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/varch.h>

// void __do_virtualise_core(u32 processor_id, u64 rip, u64 rsp, rflags_t rflags)
.extern __do_virtualise_core

// void vexit_mitigations(void)
.extern vexit_mitigations

// void ventry_mitigations(void)
.extern ventry_mitigations

// void vexit_dispatcher(struct regs *regs)
.extern vexit_dispatcher

// void __vreset_state(void)
.extern __vreset_state

// void vfailure_recover_running(void)
.extern vfailure_recover_running

// void __virtualise_core(u64 processor_id)
.globl __virtualise_core
.type __virtualise_core, @function

// void __vexit(void)
.globl __vexit
.type __vexit, @function 

// void __vlaunch(void)
.globl __vlaunch
.type __vlaunch, @function 

.align 64
.section .text

__virtualise_core:

    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    leaq .ret_addr(%rip), %rsi
    movq %rsp, %rdx

    pushfq
    pop %rcx

    call __do_virtualise_core

.ret_addr:

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret
    int3

.align 64
__vexit:

    PUSH_VREGS
    call __vreset_state

    call vexit_mitigations

    movq %rsp, %rdi
    call vexit_dispatcher

    call ventry_mitigations

    call __vreset_state
    POP_VREGS

    vmresume

    call __vreset_state
    jmp vfailure_recover_running
    int3

.align 64
__vlaunch:

    call ventry_mitigations
    call __vreset_state
    vmlaunch

    jmp vfailure_recover_running
    int3

#endif