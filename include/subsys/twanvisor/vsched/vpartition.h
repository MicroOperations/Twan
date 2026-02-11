#ifndef _VPARTITION_H_
#define _VPARTITION_H_

#include <include/subsys/twanvisor/vsched/vcpu.h>
#include <include/subsys/sync/mcslock.h>

struct vpartition;
typedef void (*vdestroy_partition_func_t)(struct vpartition *vpartition);

typedef union
{
    u32 val;
    
    struct
    {
        u32 read : 1;
        u32 write : 1;
        u32 min_criticality_writeable : 8;
        u32 max_criticality_writeable : 8;
        u32 reserved0 : 14;
    } fields;
} vcriticality_perm_t;

typedef union 
{
    u64 val;

    struct 
    {
        u64 ept_base_phys : 63;
        u64 ept_enabled : 1;
    } fields;

} ept_state_t;

struct vpartition_regions_arch
{
    ept_state_t ept_state;
    ept_caching_policy_t ept_caching_policy;
} __packed;

struct vpartition
{
    struct vpartition_regions_arch arch __aligned(4096);

    struct bmp256 ipi_receivers;
    struct bmp256 ipi_senders;

    struct bmp256 tlb_shootdown_receivers;
    struct bmp256 tlb_shootdown_senders;
    
    struct bmp256 read_vcpu_state_receivers;
    struct bmp256 read_vcpu_state_senders;

    struct bmp256 pv_spin_kick_receivers;
    struct bmp256 pv_spin_kick_senders;

    vcriticality_perm_t criticality_perms[NUM_CPUS];

    bool root;

    struct vcpu *vcpus;
    u32 vcpu_count;

    atomic32_t num_terminated;

    struct 
    {
        u32 processor_id;
        u8 vector;
        bool nmi;
    } terminate_notification;

    u8 vid;
};

struct vpartition_table_entry
{
    struct vpartition *vpartition;
    bool valid;
    struct mcslock_isr lock;
};

void vpartition_table_init(void);

int __vpartition_id_alloc(void);
int vpartition_id_alloc(void);

void vpartition_id_free(u8 vid);

void __vpartition_place(struct vpartition *vpartition, u8 vid);
void __vpartition_remove(u8 vid);

void vpartition_place(struct vpartition *vpartition, u8 vid);
void vpartition_remove(u8 vid);

struct vpartition *vpartition_get(u8 vid, struct mcsnode *mcsnode);
void vpartition_put(u8 vid, struct mcsnode *mcsnode);

void vpartition_start_exclusive(void);
struct vpartition *vpartition_get_exclusive(u8 vid);
void vpartition_stop_exclusive(void);

#endif