#ifndef _APIC_H_
#define _APIC_H_

#include <kernel/kernel.h>
#include <lib/x86_index.h>
#include <types.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

typedef union 
{
    u8 val;
    struct 
    {
        u8 icw4_needed : 1;
        u8 single_mode : 1;    
        u8 interval4 : 1;       
        u8 trigger_mode : 1;   
        u8 init : 1;           
        u8 reserved0 : 3; 
    } fields;
} pic_icw1_t;

typedef union 
{
    u8 val;
    struct 
    {
        u8 mode_8086 : 1;      
        u8 auto_eoi : 1;       
        u8 buf_mode : 2; 
        u8 sfnm : 1;        
        u8 reserved0 : 3;
    } fields;
} pic_icw4_t;

struct lapic_calibration
{
    u64 lapic_frequency_hz;
    u64 lapic_ms_count;
};

struct irq_line
{
    bool active_low;
    bool level_trig;
    delivery_mode_t delivery_mode;
    u32 gsi;
};

#define INITIALIZE_IRQ_LINE_ISA(irq, _delivery_mode)    \
{                                                       \
    .active_low = false,                                \
    .level_trig = false,                                \
    .delivery_mode = (_delivery_mode),                  \
    .gsi = (irq)                                        \
}

#define INITIALIZE_IRQ_LINE_PCI(irq, _delivery_mode)    \
{                                                       \
    .active_low = true,                                 \
    .level_trig = true,                                 \
    .delivery_mode = (_delivery_mode),                  \
    .gsi = (irq)                                        \
}

#define EDGE_TRIGGERED 0
#define LEVEL_TRIGGERED 1

void remap_mask_8259pic(void);

volatile struct ioapic *__ioapic_gsi_to_mmio(u32 gsi, u32 *gsi_base);

u32 __ioapic_read(volatile struct ioapic *mmio, u32 reg);
void __ioapic_write(volatile struct ioapic *mmio, u32 reg, u32 data);

u32 __ioapic_num_redirection_entries(volatile struct ioapic *mmio);

int __lookup_irq_line(u32 irq, struct irq_line *line, 
                      volatile struct ioapic **mmio, u32 *pin);

int __ioapic_config_irq(bool mask, u32 dest, u32 irq, u8 vector, 
                        bool trig_explicit, bool trig);

int __ioapic_config(void);

u64 lapic_read(u32 offset);
void lapic_write(u32 offset, u64 val);

void enable_lapic(u8 spurious_vector);
void disable_lapic(void);
void mask_lapic_lint(void);

void lapic_sync(void);

int __config_lapic_nmis(void);

void lapic_wait_delivery_complete(void);

void lapic_send_ipi(u32 dest, u32 delivery_mode, u32 dest_mode, 
                    u32 dest_type, u32 vector);

void lapic_send_ipi_targetted(u32 dest, u32 delivery_mode, u32 vector);

void lapic_wakeup_ap(u32 lapic_id, u32 vector);

void lapic_eoi(void);

struct lapic_calibration calibrate_lapic_timer(u8 spurious_vector, u32 ms, 
                                              lapic_dcr_config_t dcr);
                                              
u32 lapic_timer_init(u8 vector, u32 ms);
void set_lapic_oneshot(u8 vector, u64 ticks, lapic_dcr_config_t dcr);

void mask_lapic_timer(bool mask);

void lapic_timer_reset(void);
u32 lapic_timer_disable(void);
void lapic_timer_enable(u32 initial);

bool is_lapic_irr_set(u8 vector);
bool is_lapic_isr_set(u8 vector);

bool is_lapic_timer_pending(u8 vector);
bool is_lapic_oneshot_done(void);

#endif