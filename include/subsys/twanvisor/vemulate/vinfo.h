#ifndef _VINFO_H_
#define _VINFO_H_

#include <include/subsys/twanvisor/varch.h>
#include <include/lib/x86_index.h>

/* would probably have to do something similar to L4/re's Kip on architectures
   without hardware assisted virt */

#define VCPUID_BASE_B 0x5477616e
#define VCPUID_BASE_C 0x48765477
#define VCPUID_BASE_D 0x616e4876

typedef enum 
{
    VCPUID_BASE = CPUID_HV_MIN,
    VCPUID_COMPAT,
    VCPUID_ROOT_INFO,
    VCPUID_EMULATION_FEATURES,
    VCPUID_VPARTITION_INFO,
    VCPUID_VCPU_INFO
} twanvisor_cpuid_leaves_t;

#define VCPUID_MAX_LEAF VCPUID_VCPU_INFO

typedef union 
{
    u32 val;
    struct 
    {
        u32 max_leaf : 32;
    } fields;
} vcpuid_base_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 id_b : 32;
    } fields;
} vcpuid_base_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 id_c : 32;
    } fields;
} vcpuid_base_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 id_d : 32;
    } fields;
} vcpuid_base_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_compat_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_compat_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_compat_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_compat_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 max_subleaf : 32;
    } fields;
} vcpuid_root_info0_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 num_physical_processors : 32;
    } fields;
} vcpuid_root_info0_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 physical_processor_id : 32;
    } fields;
} vcpuid_root_info0_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 physical_nmis_disabled : 1;
        u32 route_nmis_as_normal : 1;
        u32 reserved0 : 30;
    } fields;
} vcpuid_root_info0_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vsched_timer_frequency_hz_low32 : 32;
    } fields;
} vcpuid_root_info1_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vsched_timer_frequency_hz_high32 : 32;
    } fields;
} vcpuid_root_info1_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vsched_timer_period_fs_low32 : 32;
    } fields;
} vcpuid_root_info1_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vsched_timer_period_fs_high32 : 32;
    } fields;
} vcpuid_root_info1_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector_0_to_31 : 32;
    } fields;
} vcpuid_root_info2_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector_32_to_63 : 32;
    } fields;
} vcpuid_root_info2_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector_64_to_95 : 32;
    } fields;
} vcpuid_root_info2_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector_96_to_127 : 32;
    } fields;
} vcpuid_root_info2_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector_128_to_159 : 32;
    } fields;
} vcpuid_root_info3_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector_160_to_191 : 32;
    } fields;
} vcpuid_root_info3_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector_192_to_223 : 32;
    } fields;
} vcpuid_root_info3_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vector_224_to_255 : 32;
    } fields;
} vcpuid_root_info3_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 max_subleaf : 32;
    } fields;
} vcpuid_emulation_features0_a_t;

/*  
    VCPUID_EMULATION_FEATURES[0].EBX

-   task_gate_emulation:
*   task gates are emulated otherwise #GP(0) on invocation

-   ia32_feature_control_unconditionally_rw:
*   ia32_feature_control is readable and writeable regardless of msr 
    permissions, when this is set, ia32_feature_control is guaranteed to be
    locked, none of its bits will be changeable

-   ia32_arch_capabilities_unconditionally_r:
*   ia32_arch_capabilities is readable regardless of msr permissions
            
-   disallowed_io_emulation:
*   io instructions where the port is disallowed by the IO bitmap store an 0xff
    (dummy value) otherwise will #GP(0) into the guest

-   ept_violations_cause_ve:
*   ept violations inject a #VE into the guest otherwise they cause guest 
    failure

-   suppress_ve_supported:
*   if set, suppressed ve's will be emulated by the vmm, otherwise it'll cause
    guest failure

*/

typedef union 
{
    u32 val;
    struct 
    {
        u32 task_gate_emulation : 1;
        u32 ia32_feature_control_unconditionally_rw : 1;
        u32 ia32_arch_capabilities_unconditionally_r : 1;
        u32 disallowed_io_emulation : 1;
        u32 ept_violations_cause_ve : 1;
        u32 suppress_ve_supported : 1;
        u32 reserved0 : 27;
    } fields;
} vcpuid_emulation_features0_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 num_vtimers : 8;
        u32 reserved0 : 24;
    } fields;
} vcpuid_emulation_features0_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 ept_optional : 1;
        u32 ept : 1;
        u32 ept_pml4 : 1;
        u32 ept_pml5 : 1;
        u32 ept_2mb : 1;
        u32 ept_1gb : 1;
        u32 ept_wb : 1;
        u32 ept_uc : 1;
        u32 ept_accessed_dirty : 1;
        u32 vpid : 1;
        u32 unrestricted_guest : 1;
        u32 reserved0 : 21;
    } fields;
} vcpuid_emulation_features0_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vtimer_frequency_hz_low32 : 32;
    } fields;
} vcpuid_emulation_features1_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vtimer_frequency_hz_high32 : 32;
    } fields;
} vcpuid_emulation_features1_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vtimer_period_fs_low32 : 32;
    } fields;
} vcpuid_emulation_features1_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vtimer_period_fs_high32 : 32;
    } fields;
} vcpuid_emulation_features1_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_vpartition_info_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vid : 8;
        u32 root : 1;
        u32 reserved0 : 23; 
    } fields;
} vcpuid_vpartition_info_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 vcpu_count : 32;
    } fields;
} vcpuid_vpartition_info_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_vpartition_info_d_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_vcpu_info_a_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 processor_id : 32;
    } fields;
} vcpuid_vcpu_info_b_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_vcpu_info_c_t;

typedef union 
{
    u32 val;
    struct 
    {
        u32 reserved0 : 32;
    } fields;
} vcpuid_vcpu_info_d_t;

void vinfo_dispatcher(struct vregs *vregs);

void vinfo_base(struct vregs *vregs);
void vinfo_compat(struct vregs *vregs);
void vinfo_root_info(struct vregs *vregs);
void vinfo_emulation_features(struct vregs *vregs);
void vinfo_vpartition_info(struct vregs *vregs);
void vinfo_vcpu_info(struct vregs *vregs);

#endif