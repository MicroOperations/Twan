#include <include/kernel/apic/apic.h>
#include <include/kernel/isr/isr_index.h>
#include <uacpi/acpi.h>
#include <include/lib/x86_index.h>
#include <include/subsys/time/sleep.h>
#include <include/subsys/time/counter.h>
#include <include/kernel/kapi.h>
#include <include/kernel/acpi_api/acpi_api.h>
#include <include/kernel/isr/base_isrs.h>

static struct irq_line low_irq_map[16] = {
    INITIALIZE_IRQ_LINE_ISA(0, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(1, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(2, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(3, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(4, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(5, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(6, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(7, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(8, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(9, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(10, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(11, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(12, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(13, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(14, DM_NORMAL),
    INITIALIZE_IRQ_LINE_ISA(15, DM_NORMAL),
};

void remap_mask_8259pic(void)
{
    pic_icw1_t icw1 = {
        .fields = {
            .init = 1,
            .icw4_needed = 1
        }
    };

    outb(PIC1_COMMAND, icw1.val);
    outb(PIC2_COMMAND, icw1.val);
    
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    outb(PIC1_DATA, 0x4);
    outb(PIC2_DATA, 0x2);

    pic_icw4_t icw4 = {.fields.mode_8086 = 1};
    
    outb(PIC1_DATA, icw4.val);
    outb(PIC2_DATA, icw4.val);

    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}

volatile struct ioapic *__ioapic_gsi_to_mmio(u32 gsi, u32 *gsi_base)
{
    struct twan_kernel *kernel = twan();
    u32 num_ioapics = kernel->acpi.num_ioapics;

    u32 ret_gsi_base = 0;
    volatile struct ioapic *mmio = NULL;
    for (u32 i = 0; i < num_ioapics; i++) {

        u32 this_gsi_base = kernel->acpi.ioapic[i].ioapic_gsi_base;

        if (this_gsi_base >= ret_gsi_base && this_gsi_base <= gsi) {
            mmio = kernel->acpi.ioapic[i].ioapic_mmio;
            ret_gsi_base = this_gsi_base;
        }
    }

    if (mmio && gsi_base)
        *gsi_base = ret_gsi_base;

    return mmio;
}

u32 __ioapic_read(volatile struct ioapic *mmio, u32 reg)
{
    mmio->reg = reg;
    return mmio->data;
}

void __ioapic_write(volatile struct ioapic *mmio, u32 reg, u32 data)
{
    mmio->reg = reg;
    mmio->data = data;
}

u32 __ioapic_num_redirection_entries(volatile struct ioapic *mmio)
{
    ioapic_version_t version = {
        .val = __ioapic_read(mmio, IOAPIC_REG_VERSION)
    };
    
    return version.fields.redirection_entries_no + 1;
}

int __lookup_irq_line(u32 irq, struct irq_line *line, 
                     volatile struct ioapic **mmio, u32 *pin)
{
    struct irq_line _line = INITIALIZE_IRQ_LINE_PCI(irq, DM_NORMAL);
    if (irq < ARRAY_LEN(low_irq_map))
        _line = low_irq_map[irq];

    u32 gsi_base;
    volatile struct ioapic *_mmio = __ioapic_gsi_to_mmio(_line.gsi, &gsi_base);
    if (!_mmio)
        return -EINVAL;

    u32 _pin = _line.gsi - gsi_base;
    if (_pin >= __ioapic_num_redirection_entries(_mmio))
        return -EINVAL;

    u8 reg_low = ioapic_reg_low(_pin);
    ioapic_redirection_entry_low_t entry_low = {
        .val = __ioapic_read(_mmio, reg_low)
    };

    /* dont touch irq's that are likely to be handled by firmware, init/startup
       may be unusual to see however may be for platform reset */
    delivery_mode_t delivery_mode = entry_low.fields.delivery_mode;
    if (delivery_mode == DM_SMI || 
        delivery_mode == DM_INIT ||
        delivery_mode == DM_STARTUP) {
        return -EINVAL;
    }

    if (delivery_mode == DM_NMI) {
        _line.active_low = entry_low.fields.polarity;
        _line.level_trig = entry_low.fields.trigger_mode;
        _line.delivery_mode = delivery_mode;
    }

    if (line)
        *line = _line;
    
    if (mmio)
        *mmio = _mmio;

    if (pin)
        *pin = _pin;

    return 0;
}

int __ioapic_config_irq(bool mask, u32 dest, u32 irq, u8 vector, 
                        bool trig_explicit, bool trig)
{
    struct irq_line line;
    volatile struct ioapic *mmio;
    u32 pin;

    int ret = __lookup_irq_line(irq, &line, &mmio, &pin);
    if (ret < 0)
        return ret;

    bool normal = twan()->flags.fields.twanvisor_on &&
                  this_cpu_data()->flags.fields.nmis_as_normal != 0;

    ioapic_redirection_entry_low_t entry_low = {
        .fields = {
            .delivery_mode = vector == NMI && !normal ? DM_NMI : DM_NORMAL,
            .destination_mode = IOAPIC_DEST_MODE_PHYSICAL,
            .interrupt_mask = mask,
            .polarity = line.active_low,
            .trigger_mode = trig_explicit ? trig : line.level_trig,
            .vector = vector
        }
    };

    ioapic_redirection_entry_high_t entry_high = {
        .fields = {
            .dest = dest
        }
    };

    u32 reg_low = ioapic_reg_low(pin);
    u32 reg_high = ioapic_reg_high(pin);

    __ioapic_write(mmio, reg_low, entry_low.val);
    __ioapic_write(mmio, reg_high, entry_high.val);

    return ret;
}

int __ioapic_config(void)
{
    struct twan_kernel *kernel = twan();

    struct acpi_madt *madt = 
        __early_get_acpi_table_kernel(kernel, ACPI_MADT_SIGNATURE);

    if (!madt || madt->hdr.length > PAGE_SIZE)
        return -EFAULT;
   
    u8 *madt_entry = (void *)madt->entries;
    size_t madt_entries_size = madt->hdr.length - sizeof(struct acpi_madt);
    u8 *madt_entries_end = madt_entry + madt_entries_size;

    while (madt_entry < madt_entries_end) {

        struct acpi_entry_hdr *entry_hdr = (void *)madt_entry;
        if (madt_entry + entry_hdr->length > madt_entries_end)
            break;

        madt_entry += entry_hdr->length;

        u32 gsi;
        u32 polarity;
        u32 trig_mode;
        bool active_low;
        bool level_trig;
        delivery_mode_t delivery_mode;
        
        switch (entry_hdr->type) {
        
            case ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE:

                struct acpi_madt_interrupt_source_override *override =
                    (void *)entry_hdr;

                u32 src = override->source;

                gsi = override->gsi;
                polarity = override->flags & ACPI_MADT_POLARITY_MASK;
                trig_mode = override->flags & ACPI_MADT_TRIGGERING_MASK;
                active_low = polarity == ACPI_MADT_POLARITY_ACTIVE_LOW;
                level_trig = trig_mode == ACPI_MADT_TRIGGERING_LEVEL;
                delivery_mode = DM_NORMAL;

                if (src < ARRAY_LEN(low_irq_map)) {
                    low_irq_map[src].active_low = active_low;
                    low_irq_map[src].level_trig = level_trig;
                    low_irq_map[src].delivery_mode = delivery_mode;
                    low_irq_map[src].gsi = gsi;
                }

                break;

            case ACPI_MADT_ENTRY_TYPE_NMI_SOURCE:

                struct acpi_madt_nmi_source *nmi_src = (void *)entry_hdr;
                    
                gsi = nmi_src->gsi;
                polarity = nmi_src->flags & ACPI_MADT_POLARITY_MASK;
                trig_mode = nmi_src->flags & ACPI_MADT_TRIGGERING_MASK;
                active_low = polarity == ACPI_MADT_POLARITY_ACTIVE_LOW;
                level_trig = trig_mode == ACPI_MADT_TRIGGERING_LEVEL;
                delivery_mode = DM_NMI;
                
                u32 gsi_base;
                volatile struct ioapic *mmio = 
                    __ioapic_gsi_to_mmio(gsi, &gsi_base);

                if (!mmio)
                    continue;

                u32 pin = gsi - gsi_base;
                if (pin >= __ioapic_num_redirection_entries(mmio))
                    continue;

                u8 reg_low = ioapic_reg_low(pin);
                ioapic_redirection_entry_low_t entry_low = {
                    .val = __ioapic_read(mmio, reg_low)
                };

                /* mask the entry, unmask upon request for this irq */
                entry_low.fields.interrupt_mask = 1;
                entry_low.fields.vector = NMI;
                entry_low.fields.delivery_mode = delivery_mode;
                entry_low.fields.polarity = active_low;
                entry_low.fields.trigger_mode = level_trig;
        
                __ioapic_write(mmio, reg_low, entry_low.val);
                break;

            default:
                break;
        }
    }        

    __early_put_acpi_table(madt);
    return 0;
}

u64 lapic_read(u32 offset)
{
    if (this_cpu_data()->flags.fields.x2apic == 0) 
        return *(volatile u32 *)((u8 *)twan()->acpi.lapic_mmio + offset);

    u32 msr = IA32_X2APIC_BASE + (offset / 16);
    return __rdmsrl(msr);   
}

void lapic_write(u32 offset, u64 val)
{
    /* treat writes to ICR_HIGH as the entire ICR, and writes to ICR_LOW as
       writes to the low ICR to allow for deasserts in xapic mode */

    if (this_cpu_data()->flags.fields.x2apic == 0) {

        u32 lower = (u32)val;
        u32 upper = (u32)(val >> 32);

        u64 base = (u64)twan()->acpi.lapic_mmio;

        if (offset == LAPIC_ICR_HIGH_OFFSET) {
            *(volatile u32 *)(base + offset) = upper;
            *(volatile u32 *)(base + LAPIC_ICR_LOW_OFFSET) = lower;
            return;
        }

        *(volatile u32 *)(base + offset) = lower;
        return;
    }

    if (offset == LAPIC_ICR_HIGH_OFFSET)
        offset = LAPIC_ICR_LOW_OFFSET;

    u32 msr = IA32_X2APIC_BASE + (offset / 16);

    __wrmsrl(msr, val);
}

void disable_lapic(void)
{
    lapic_sivr_t sivr = {.val = lapic_read(LAPIC_SIVR_OFFSET)};
    sivr.fields.lapic_enable = 0;
    lapic_write(LAPIC_SIVR_OFFSET, sivr.val);
}

void enable_lapic(u8 spurious_vector)
{
    lapic_sivr_t sivr = {.val = lapic_read(LAPIC_SIVR_OFFSET)};
    sivr.fields.vector = spurious_vector;
    sivr.fields.lapic_enable = 1;
    lapic_write(LAPIC_SIVR_OFFSET, sivr.val);
}

void mask_lapic_lint(void)
{
    lapic_lint_t lint0 = {.val = lapic_read(LAPIC_LINT0_OFFSET)};
    lapic_lint_t lint1 = {.val = lapic_read(LAPIC_LINT1_OFFSET)};

    lint0.fields.mask = 1;
    lint1.fields.mask = 1;

    lapic_write(LAPIC_LINT0_OFFSET, lint0.val);
    lapic_write(LAPIC_LINT1_OFFSET, lint1.val);
}

void lapic_sync(void)
{
    u32 regs[4] = {CPUID_FEATURE_BITS, 0, 0, 0};
    feature_bits_c_t feature_bits_c = {.val = regs[2]};

    if (feature_bits_c.fields.x2apic == 0) {

        ia32_apic_base_t base = {.val = __rdmsrl(IA32_APIC_BASE)};

        if (base.fields.apic_global_enable != 0 || 
            base.fields.enable_x2apic_mode == 0) {

            base.fields.apic_global_enable = 1;
            base.fields.enable_x2apic_mode = 1;
            __wrmsrl(IA32_APIC_BASE, base.val);
        }
        
        this_cpu_data()->flags.fields.x2apic = 1;
        return;
    } 

    u32 ext_regs0[4] = {CPUID_EXTENDED_FEATURES, 0, 0, 0};
    __cpuid(&ext_regs0[0], &ext_regs0[1], &ext_regs0[2], &ext_regs0[3]);
    extended_features0_d_t ext_regs0_d = {.val = ext_regs0[3]};

    if (ext_regs0_d.fields.ia32_arch_capabilities == 0) {

        ia32_arch_capabilities_t cap = {
            .val = __rdmsrl(IA32_ARCH_CAPABILITIES)
        };

        if (cap.fields.xapic_disable_status != 0) {

            ia32_xapic_disable_status_t stat = {
                .val = __rdmsrl(IA32_XAPIC_DISABLE_STATUS)
            };

            if (stat.fields.legacy_xapic_disabled != 0)
                __early_kpanic("xapic disabled\n");
        }
    }

    ia32_apic_base_t base = {.val = __rdmsrl(IA32_APIC_BASE)};

    u64 phys = twan()->acpi.lapic_mmio_phys >> 12;;

    if (base.fields.apic_global_enable == 0 || 
        base.fields.enable_x2apic_mode != 0 || 
        base.fields.apic_base_phys != phys) {

        base.fields.apic_global_enable = 1;
        base.fields.enable_x2apic_mode = 0;
        base.fields.apic_base_phys = phys; 
        __wrmsrl(IA32_APIC_BASE, base.val);
    }
}

/* apply this per lapic and when the idt is setup */
int __config_lapic_nmis(void)
{
    struct twan_kernel *kernel = twan(); 
   
    struct acpi_madt *madt = 
        __early_get_acpi_table_kernel(kernel, ACPI_MADT_SIGNATURE);

    if (!madt || madt->hdr.length > PAGE_SIZE)
        return -EFAULT;
   
    u8 *madt_entry = (void *)madt->entries;
    size_t madt_entries_size = madt->hdr.length - sizeof(struct acpi_madt);
    u8 *madt_entries_end = madt_entry + madt_entries_size;

    u32 acpi_uid = this_cpu_data()->acpi_uid;

    while (madt_entry < madt_entries_end) {

        struct acpi_entry_hdr *entry_hdr = (void *)madt_entry;
        if (madt_entry + entry_hdr->length > madt_entries_end)
            break;

        madt_entry += entry_hdr->length;
        if (entry_hdr->type != ACPI_MADT_ENTRY_TYPE_LAPIC_NMI)
            continue;

        struct acpi_madt_lapic_nmi *lapic_nmi = (void *)entry_hdr;
        
        if (lapic_nmi->uid != acpi_uid && lapic_nmi->uid != 0xff)
            continue;

        u32 polarity = lapic_nmi->flags & ACPI_MADT_POLARITY_MASK;
        u32 trigger_mode = lapic_nmi->flags & ACPI_MADT_TRIGGERING_MASK;

        bool active_low = polarity == ACPI_MADT_POLARITY_ACTIVE_LOW;
        bool level_triggered = trigger_mode == ACPI_MADT_TRIGGERING_LEVEL;

        u32 lint_offset = 0;

        if (lapic_nmi->lint == 0) 
            lint_offset = LAPIC_LINT0_OFFSET;
        else if (lapic_nmi->lint == 1)
            lint_offset = LAPIC_LINT1_OFFSET;
        else
            continue;

        lapic_lint_t lint = {.val = lapic_read(lint_offset)};
        lint.fields.vector = NMI;
        lint.fields.delivery_mode = DM_NMI;
        lint.fields.polarity = active_low;
        lint.fields.trigger_mode = level_triggered;
        lapic_write(lint_offset, lint.val);
    }

    __early_put_acpi_table(madt);
    return 0;
}

void lapic_wait_delivery_complete(void)
{
    if (this_cpu_data()->flags.fields.x2apic != 0)
        return;

    lapic_icr_low_t icr_low;

    do {
        icr_low.val = lapic_read(LAPIC_ICR_LOW_OFFSET);
        cpu_relax();
    } while (icr_low.fields.delivery_status != 0);
}

void lapic_send_ipi(u32 dest, u32 delivery_mode, u32 dest_mode, 
                    u32 dest_type, u32 vector)
{
    /* wont be any harm to wait for anything pending to complete first */
    lapic_wait_delivery_complete();

    lapic_icr_low_t icr_low = {
        .fields = {
            .vector = vector,
            .delivery_mode = delivery_mode,
            .level = LAPIC_ASSERT,
            .destination_mode = dest_mode,
            .trigger_mode = LAPIC_TRIGGER_EDGE,
            .destination_type = dest_type
        }
    };

    if (this_cpu_data()->flags.fields.x2apic == 0)
        dest <<= 24;

    lapic_icr_high_t icr_high = {
        .fields = {
            .destination = dest
        }
    };

    u64 val = ((u64)icr_high.val << 32) | icr_low.val;
    lapic_write(LAPIC_ICR_HIGH_OFFSET, val);

    lapic_wait_delivery_complete();
}

void lapic_send_ipi_targetted(u32 dest, u32 delivery_mode, u32 vector)
{
    lapic_send_ipi(dest, delivery_mode, LAPIC_DEST_PHYSICAL, 
                   SINGLE_TARGET, vector);
}

void lapic_wakeup_ap(u32 lapic_id, u32 vector)
{
    lapic_icr_low_t init_assert = {
        .fields = {
            .delivery_mode = DM_INIT,
            .level = LAPIC_ASSERT,
            .destination_mode = LAPIC_DEST_PHYSICAL,
            .trigger_mode = LAPIC_TRIGGER_LEVEL,
            .destination_type = SINGLE_TARGET
        }
    };

    u32 dest = lapic_id;

    if (this_cpu_data()->flags.fields.x2apic == 0)
        dest <<= 24;

    lapic_icr_high_t init_dest = {
        .fields = {
            .destination = dest
        }
    };
   
    u64 val = ((u64)init_dest.val << 32) | init_assert.val;
    lapic_write(LAPIC_ICR_HIGH_OFFSET, val);
    
    lapic_wait_delivery_complete();

    if (this_cpu_data()->flags.fields.x2apic == 0) {

        init_assert.fields.level = LAPIC_DEASSERT;
        lapic_write(LAPIC_ICR_LOW_OFFSET, init_assert.val);

        lapic_wait_delivery_complete();
    }

    lapic_send_ipi_targetted(lapic_id, DM_STARTUP, vector);
    lapic_send_ipi_targetted(lapic_id, DM_STARTUP, vector);
}

void lapic_eoi(void)
{
    lapic_write(LAPIC_EOI_OFFSET, 0);
}

struct lapic_calibration calibrate_lapic_timer(u8 spurious_vector, u32 ms, 
                                               lapic_dcr_config_t dcr)
{
    lapic_timer_t timer = {
        .fields = {
            .vector = spurious_vector,
            .mask = 1,
            .timer_mode = LAPIC_ONESHOT
        }
    };

    lapic_write(LAPIC_DCR_OFFSET, dcr);
    lapic_write(LAPIC_TIMER_OFFSET, timer.val);

    u64 period_fs = counter_period_fs();

    u64 calibration_time_fs = ms * 1000ULL * 1000ULL * 1000ULL;
    u64 ticks = calibration_time_fs / period_fs;

    lapic_write(LAPIC_INITIAL_COUNT_OFFSET, UINT32_MAX);

    u64 start = read_counter();
    u64 target = start + ticks;

    spin_until(read_counter() >= target);

    u64 lapic_cur = lapic_read(LAPIC_CUR_COUNT_OFFSET) & 0xffffffff;
    u64 lapic_ticks = ~0U - lapic_cur;
    
    u64 divisor = 0;
    switch (dcr) {
    
        case DIV_2:
            divisor = 2;
            break;

        case DIV_4:
            divisor = 4;
            break;

        case DIV_8:
            divisor = 8;
            break;

        case DIV_16:
            divisor = 16;
            break;

        case DIV_32:
            divisor = 32;
            break;

        case DIV_64:
            divisor = 64;
            break;

        case DIV_128:
            divisor = 128;
            break;

        case DIV_1:
            divisor = 1;
            break;

        default:
            break;
    }

    KBUG_ON(divisor == 0);

    u64 lapic_frequency = (lapic_ticks * 1000 * divisor) / ms;

    struct lapic_calibration calibration = {
        .lapic_frequency_hz = lapic_frequency,
        .lapic_ms_count = lapic_ticks
    };

    return calibration;
}

u32 lapic_timer_init(u8 vector, u32 ms)
{
    struct lapic_calibration calibration = 
        calibrate_lapic_timer(SPURIOUS_INT_VECTOR, ms, DIV_16);

    lapic_write(LAPIC_INITIAL_COUNT_OFFSET, 0);
    lapic_timer_t timer = {
        .fields = {
            .vector = vector,
            .timer_mode = LAPIC_PERIODIC
        }
    };

    lapic_write(LAPIC_DCR_OFFSET, DIV_16);
    lapic_write(LAPIC_TIMER_OFFSET, timer.val);
    lapic_write(LAPIC_INITIAL_COUNT_OFFSET, calibration.lapic_ms_count);

    return calibration.lapic_ms_count;
}

void set_lapic_oneshot(u8 vector, u64 ticks, lapic_dcr_config_t dcr)
{
    lapic_write(LAPIC_INITIAL_COUNT_OFFSET, 0);
    lapic_timer_t timer = {
        .fields = {
            .vector = vector,
            .timer_mode = LAPIC_ONESHOT
        }
    };

    lapic_write(LAPIC_DCR_OFFSET, dcr);
    lapic_write(LAPIC_TIMER_OFFSET, timer.val);
    lapic_write(LAPIC_INITIAL_COUNT_OFFSET, ticks);
}

void mask_lapic_timer(bool mask)
{
    lapic_timer_t timer = {.val = lapic_read(LAPIC_TIMER_OFFSET)};
    timer.fields.mask = mask;
    lapic_write(LAPIC_TIMER_OFFSET, timer.val);
}

void lapic_timer_reset(void)
{
    u32 initial = lapic_read(LAPIC_INITIAL_COUNT_OFFSET);
    lapic_write(LAPIC_INITIAL_COUNT_OFFSET, initial);
}

u32 lapic_timer_disable(void)
{
    u32 initial = lapic_read(LAPIC_INITIAL_COUNT_OFFSET);
    lapic_write(LAPIC_INITIAL_COUNT_OFFSET, 0);
    return initial;
}

void lapic_timer_enable(u32 initial)
{
    lapic_write(LAPIC_INITIAL_COUNT_OFFSET, initial);
}

bool is_lapic_irr_set(u8 vector)
{
    u32 idx = vector / 32;
    u32 bit_pos = vector % 32;
    
    u32 irr_offset = LAPIC_IRR_OFFSET + (idx * 16);
    return ((lapic_read(irr_offset) >> bit_pos) & 1) != 0;
}

bool is_lapic_isr_set(u8 vector)
{
    u32 idx = vector / 32;
    u32 bit_pos = vector % 32;
    
    u32 isr_offset = LAPIC_ISR_OFFSET + (idx * 16);
    return ((lapic_read(isr_offset) >> bit_pos) & 1) != 0;
}

bool is_lapic_timer_pending(u8 vector)
{
    return is_lapic_irr_set(vector);
}

bool is_lapic_oneshot_done(void)
{
    return lapic_read(LAPIC_CUR_COUNT_OFFSET) == 0;
}