#ifndef _LIBVC_H_
#define _LIBVC_H_

#include <include/lib/libtwanvisor/libvcalls.h>

struct vsub_ipi_arg
{
    long ret;
    u8 vector;
};

struct varm_timer_ipi_arg
{
    long ret;
    u8 vector;
    u8 timer_n;
    bool nmi;
    u32 ticks;
    bool periodic;
};

struct vdisarm_timer_ipi_arg
{
    long ret;
    u8 timer_n;
};

void vsubscribe_ipi(u64 arg);
void vunsubscribe_ipi(u64 arg);

long tv_vsubscribe_on_cpu(u32 processor_id, u8 vector);
long tv_vunsubscribe_on_cpu(u32 processor_id, u8 vector);

void varm_timer_ipi(u64 arg);
void vdisarm_timer_ipi(u64 arg);

long tv_varm_timer_on_cpu(u32 processor_id, u8 vector, u8 timer_n, u32 ticks, 
                          bool periodic, bool nmi);

long tv_vdisarm_timer_on_cpu(u32 processor_id, u8 timer_n);

bool tv_vintc_is_serviced(u8 vector);
bool tv_vintc_is_pending(u8 vector);
void tv_vintc_eoi(void);

bool tv_vis_vcpu_active(u32 processor_id);

#endif