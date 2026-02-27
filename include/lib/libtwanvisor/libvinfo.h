#ifndef _LIBVINFO_H_
#define _LIBVINFO_H_

#include <subsys/twanvisor/vemulate/vinfo.h>
#include <lib/x86_index.h>

inline bool tv_detect(void)
{
    u32 regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    __cpuid(&regs[0], &regs[1], &regs[2], &regs[3]);

    feature_bits_c_t ecx = {.val = regs[2]};

    if (ecx.fields.hypervisor == 0)
        return false;

    u32 hv_regs[4] = {VCPUID_BASE, 0, 0, 0};
    __cpuid(&hv_regs[0], &hv_regs[1], &hv_regs[2], &hv_regs[3]);

    return hv_regs[1] == VCPUID_BASE_B && 
           hv_regs[2] == VCPUID_BASE_C &&
           hv_regs[3] == VCPUID_BASE_D;
}

#endif