#ifndef _VCPU_H_
#define _VCPU_H_

#include <include/subsys/twanvisor/vsched/vsched_conf.h>
#include <include/subsys/twanvisor/vsched/vsync.h>
#include <include/subsys/twanvisor/varch.h>
#include <include/lib/dsa/dq.h>
#include <include/lib/dsa/bmp256.h>
#include <include/lib/dsa/delta_chain.h>
#include <include/kernel/isr/isr_index.h>

#if CONFIG_TWANVISOR_ON

#define VNUM_VTIMERS CONFIG_TWANVISOR_NUM_VTIMERS

#else

#define VNUM_VTIMERS 1

#endif

STATIC_ASSERT(VNUM_VTIMERS <= UINT8_MAX);

typedef enum
{
    VREADY,
    VRUNNING,
    VPAUSED,
    VPV_SPINNING,
    VTRANSITIONING,
    VTERMINATED,
    VINITIALIZING
} vcpu_state_t;

typedef enum
{
    VBOOT_NONE,
    VBOOT_READY,
    VBOOT_PAUSED
} vcpu_vboot_state_t;

typedef enum
{
    VSPIN_NONE,
    VSPIN_PAUSED,
    VSPIN_KICKED
} vcpu_pv_spin_state_t;

typedef union
{
    u8 val;

    struct 
    {
        u8 preempted : 1;
        u8 yield_request : 1;
        u8 reserved0 : 6;
    } fields;
} vcpu_flags_t;

typedef void (*vput_callback_func_t)(void);
typedef void (*vset_callback_func_t)(void);

/* worth noting that root currently doesnt use msr load save area, devs should
   disallow access to msr's used by root */

struct msr_area
{
    struct msr_entry entry[512];
} __packed;

SIZE_ASSERT(struct msr_area, 512 * sizeof(struct msr_entry));

struct vcpu_regions_arch
{
    char io_bitmap_a[4096];
    char io_bitmap_b[4096];
    char msr_bitmap[4096];
    struct ve_info_area ve_info_area;
    struct vmcs_region vmcs;

    struct msr_area vexit_msr_load_area;
    struct msr_area msr_load_save_area;

    u64 io_bitmap_a_phys;
    u64 io_bitmap_b_phys;
    u64 msr_bitmap_phys;
    u64 vmcs_phys;
    u64 ve_info_area_phys;
    
    u64 vexit_msr_load_area_phys;
    u64 msr_load_save_area_phys;

    u32 vexit_load_count;
    u32 msr_load_save_count;

} __packed;

SIZE_ASSERT(struct vcpu_regions_arch, 
            sizeof(struct vmcs_region) + sizeof(struct ve_info_area) + 
            (sizeof(struct msr_area) * 2) +
            (sizeof(u64) * 7) + (sizeof(u32) * 2) + (4096 * 3));

struct vtimer
{
    union 
    {
        u32 val;
        struct 
        {
            u32 vector : 8;
            u32 periodic : 1;
            u32 armed : 1;
            u32 nmi : 1;
            u32 reserved0 : 21;
        } fields;
    } timer;

    u32 ticks;
    struct delta_node node;
};

struct vlaunch_regs
{
    u64 rip;
    selector_t cs;
};

typedef union 
{
    u32 val;
    struct 
    {
        u32 in_intl : 16;
        u32 in_nmi : 1;
        u32 nmi_pending : 1;
        u32 gp0_pending : 1;
        u32 ud_pending : 1;
        u32 db_pending : 1;
        u32 ac0_pending : 1;
        u32 intl : 4;
        u32 int_window_exit : 1;
        u32 nmi_window_exit : 1;
        u32 int_type : 3;
        u32 reserved0 : 1;
    } fields;
} vinterrupt_delivery_data_t;

typedef union 
{
    u8 val;
    struct 
    {
        u8 vmwrite_cr4 : 1;
        u8 should_advance : 1;
        u8 reserved0 : 6;
    } fields;
} voperations_pending_t;

typedef union
{
    u16 val;
    struct 
    {
        u16 vpid : 15;
        u16 enabled : 1;
    } fields;
} vpid_t;

struct vcpu
{
    struct vcpu_regions_arch arch __aligned(4096);
    bool root;

    struct 
    {
        /* must grab schedulers lock before touching any of these fields */
        vcpu_state_t state;
        bool tlb_flush_pending;
        u8 criticality;
        u32 time_slice_ticks;
        u32 current_time_slice_ticks;
        bool rearm;
        bool terminate;
        vcpu_pv_spin_state_t pv_spin_state;
    } vsched_metadata;

    u32 preemption_count;
    vcpu_flags_t flags;

    struct vtimer timers[VNUM_VTIMERS];
    struct delta_chain timer_chain;
    
    char vexit_stack[VEXIT_STACK_SIZE] __aligned(16);

    u32 processor_id;
    vpid_t vpid;
    u32 vid;
    u32 vpartition_num_cpus;
    struct vpartition *vpartition;

    struct context context;

    vput_callback_func_t put_callback_func;
    vset_callback_func_t set_callback_func;

    u32 vqueue_id;
    struct list_double vsched_nodes[VSCHED_NUM_CRITICALITIES];
    
    struct list_double vdispatch_nodes[NUM_VECTORS];

    struct list_double ipi_nodes[NUM_CPUS];

    struct 
    {
        /* grab isr dispatcher lock for these */
        struct bmp256 allowed_external_vectors;
        struct bmp256 subscribed_vectors;
    } visr_metadata;

    struct 
    {
        /* grab visr metadata lock for these */
        vinterrupt_delivery_data_t delivery;
        struct bmp256 pending_external_interrupts;
        struct mcslock_isr lock;
    } visr_pending;

    struct 
    {
        voperations_pending_t pending;
        cr4_t cr4;
    } voperation_queue;

    struct vlaunch_regs vlaunch;
    vcpu_vboot_state_t vboot_state;
};

#define vqueue_to_vprocessor_id(vqueue_id) (vqueue_id) 
#define vprocessor_to_vqueue_id(vprocessor_id) (vprocessor_id) 

#define vsched_node_to_vcpu(ptr, criticality) \
    container_of((ptr), struct vcpu, vsched_nodes[criticality])

#define dispatch_node_to_vcpu(ptr, vector) \
    container_of((ptr), struct vcpu, vdispatch_nodes[vector])

#define delta_node_to_vtimer(ptr) \
    container_of((ptr), struct vtimer, node)

#endif