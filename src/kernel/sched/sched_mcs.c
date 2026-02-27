#include <kernel/sched/sched_mcs.h>
#include <kernel/sched/sched.h>
#include <kernel/kapi.h>

u8 __sched_mcs_read_criticality_level(void)
{
    return this_sched_priorityq()->criticality_level;
}

int __sched_mcs_write_criticality_level(u8 criticality)
{
    if (criticality > SCHED_MAX_CRITICALITY)
        return -EINVAL;

    this_sched_priorityq()->criticality_level = criticality;
    return 0;
}

u8 sched_mcs_read_criticality_level(void)
{
    struct sched_priorityq *sched_priorityq = this_sched_priorityq();
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&sched_priorityq->lock, &node);
    u8 criticality_level = __sched_mcs_read_criticality_level();
    mcs_unlock_isr_restore(&sched_priorityq->lock, &node);

    return criticality_level;
}

int sched_mcs_write_criticality_level(u8 criticality)
{
    struct sched_priorityq *sched_priorityq = this_sched_priorityq();
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&sched_priorityq->lock, &node);
    int ret = __sched_mcs_write_criticality_level(criticality);
    mcs_unlock_isr_restore(&sched_priorityq->lock, &node);

    return ret;
}

bool sched_mcs_inc_criticality_level(void)
{
    bool ret = false;

    struct sched_priorityq *sched_priorityq = this_sched_priorityq();
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&sched_priorityq->lock, &node);

    u8 criticality_level = sched_priorityq->criticality_level;
    if (criticality_level < SCHED_MAX_CRITICALITY) {
        sched_priorityq->criticality_level++;
        ret = true;
    }

    mcs_unlock_isr_restore(&sched_priorityq->lock, &node);

    return ret;
}

bool sched_mcs_dec_criticality_level(void)
{
    bool ret = false;

    struct sched_priorityq *sched_priorityq = this_sched_priorityq();
    struct mcsnode node = INITIALIZE_MCSNODE();

    mcs_lock_isr_save(&sched_priorityq->lock, &node);

    u8 criticality_level = sched_priorityq->criticality_level;
    if (criticality_level > SCHED_MIN_CRITICALITY) {
        sched_priorityq->criticality_level--;
        ret = true;
    }

    mcs_unlock_isr_restore(&sched_priorityq->lock, &node);

    return ret;
}