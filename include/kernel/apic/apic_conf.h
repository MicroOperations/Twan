#ifndef _APIC_CONF_H_
#define _APIC_CONF_H_

#include <std.h>

#define NUM_IOAPICS CONFIG_NUM_IOAPICS

/* each ioapic's mmio region will have to be mapped, the max should be enough
   to guarantee every mmio region we need to map early on can be satisfied */
STATIC_ASSERT(NUM_IOAPICS < 16U);

#endif