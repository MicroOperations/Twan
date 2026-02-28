#include <generated/autoconf.h>
#if CONFIG_SUBSYS_TWANVISOR

#include <subsys/twanvisor/vsched/vpartition.h>
#include <subsys/twanvisor/twanvisor.h>
#include <subsys/twanvisor/vemulate/vemulate_utils.h>
#include <subsys/sync/rwlock.h>
#include <lib/dsa/bmp256.h>

static struct vpartition_table_entry vpartition_table[256];
static struct rwlock_isr vpartition_table_rwlock = INITIALIZE_RWLOCK_ISR();

static struct bmp256 vids;
static struct mcslock_isr vids_lock = INITIALIZE_MCSLOCK_ISR();

void vpartition_table_init(void)
{
    bmp256_set_all(&vids);
    for (u32 i = 0; i < ARRAY_LEN(vpartition_table); i++)
        mcslock_isr_init(&vpartition_table->lock);
}

int __vpartition_id_alloc(void)
{
    int ret = bmp256_ffs(&vids);

    if (ret != -1)
        bmp256_unset(&vids, ret);

    return ret;
}

int vpartition_id_alloc(void)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vids_lock, &node);
    int ret = __vpartition_id_alloc();
    vmcs_unlock_isr_restore(&vids_lock, &node);

    return ret;
}

void vpartition_id_free(u8 vid)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vids_lock, &node);
    bmp256_set(&vids, vid);
    vmcs_unlock_isr_restore(&vids_lock, &node);
}

void __vpartition_place(struct vpartition *vpartition, u8 vid)
{
    vpartition_table[vid].vpartition = vpartition;
    vpartition_table[vid].valid = true;
}

void __vpartition_remove(u8 vid)
{
    vpartition_table[vid].vpartition = NULL;
    vpartition_table[vid].valid = false;
}

void vpartition_place(struct vpartition *vpartition, u8 vid)
{
    struct mcsnode node = INITIALIZE_MCSNODE();

    vmcs_lock_isr_save(&vpartition_table[vid].lock, &node);
    __vpartition_place(vpartition, vid);
    vmcs_unlock_isr_restore(&vpartition_table[vid].lock, &node);
}

void vpartition_remove(u8 vid)
{
    struct mcsnode node = INITIALIZE_MCSNODE();
    vmcs_lock_isr_save(&vpartition_table[vid].lock, &node);

    __vpartition_remove(vid);

    vmcs_unlock_isr_restore(&vpartition_table[vid].lock, &node);
}

struct vpartition *vpartition_get(u8 vid, struct mcsnode *mcsnode)
{
    vrw_lock_read_isr_save(&vpartition_table_rwlock);

    struct vpartition_table_entry *entry = &vpartition_table[vid];
    vmcs_lock_isr_save(&entry->lock, mcsnode);

    if (!entry->valid) {
        vmcs_unlock_isr_restore(&entry->lock, mcsnode);
        vrw_unlock_read_isr_restore(&vpartition_table_rwlock);
        return NULL;
    }

    return entry->vpartition;
}

void vpartition_put(u8 vid, struct mcsnode *mcsnode)
{
    vmcs_unlock_isr_restore(&vpartition_table[vid].lock, mcsnode);
    vrw_unlock_read_isr_restore(&vpartition_table_rwlock);
}

void vpartition_start_exclusive(void)
{
    vrw_lock_write_isr_save(&vpartition_table_rwlock);
}

struct vpartition *vpartition_get_exclusive(u8 vid)
{
    struct vpartition_table_entry *entry = &vpartition_table[vid];
    return entry->valid ? entry->vpartition : NULL;   
}

void vpartition_stop_exclusive(void)
{
    vrw_unlock_write_isr_restore(&vpartition_table_rwlock);    
}

#endif