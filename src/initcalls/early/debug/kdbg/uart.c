#include <initcalls/early_initcalls_conf.h>
#if EARLY_DEBUG_KDBG_UART

#include <lib/uart.h>
#include <subsys/debug/kdbg/kdbg.h>
#include <kernel/kapi.h>

/* ports */
#define COM1_PORT 0x3F8
#define COM2_PORT 0x2F8
#define COM3_PORT 0x3E8
#define COM4_PORT 0x2E8
#define COM5_PORT 0x5F8
#define COM6_PORT 0x4F8
#define COM7_PORT 0x5E8
#define COM8_PORT 0x4E8

#define KDBG_COM_PORT COM1_PORT 

static void __kdbg_uart(const char *str)
{
    __uart_transmit_str(KDBG_COM_PORT, str);
} 

static struct kdbg_interface kdbg_interface = {
    .__kdbg_func = __kdbg_uart
};

static __early_initcall void kdbg_uart_init(void)
{
    if (uart_init(KDBG_COM_PORT) == 0)
        kdbg_init(&kdbg_interface);
}

REGISTER_EARLY_INITCALL(kdbg_uart, kdbg_uart_init, 
                        EARLY_DEBUG_KDBG_UART_ORDER);
#endif