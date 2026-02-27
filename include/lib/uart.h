#ifndef _UART_H_
#define _UART_H_

#include <std.h>
#include <errno.h>

/* register offsets */

/* dlab set to 0 */
#define RECV_BUF_OFFSET_R 0
#define TRANSMIT_BUF_OFFSET_W 0
#define INT_ENABLE_OFFSET_RW 1

/* dlab set to 1 */
#define DIVISOR_LSB_OFFSET_RW 0
#define DIVISOR_MSB_OFFSET_RW 1

#define INT_ID_OFFSET_R 2
#define FIFO_CTRL_OFFSET_W 2
#define LINE_CTRL_OFFSET_RW 3
#define MODEM_CTRL_OFFSET_RW 4
#define LINE_STATUS_OFFSET_R 5
#define MODEM_STATUS_OFFSET_R 6
#define SCRATCH_OFFSET_RW 7

#define CHAR_LEN_5 0
#define CHAR_LEN_6 1
#define CHAR_LEN_7 2
#define CHAR_LEN_8 3

typedef union
{
    u8 val;
    struct 
    {
        u8 data : 2;
        u8 stop : 1;
        u8 parity : 3;
        u8 break_enable : 1;
        u8 divisor_latch_access : 1;
    } fields;
} line_ctrl_t;

typedef union
{
    u8 val;
    struct 
    {
        u8 recv_data_available : 1;
        u8 trans_holding_empty : 1;
        u8 recv_line_status : 1;
        u8 modem_status : 1;
        u8 reserved0 : 4;
    } fields;
} int_enable_t;

typedef union
{
    u8 val;
    struct 
    {
        u8 enable_fifos : 1;
        u8 clear_recv_fifo : 1;
        u8 clear_trans_fifo : 1;
        u8 dma_mode_select_clear : 1;
        u8 level_reserved_dma : 2;
        u8 int_trigger_level : 2;
    } fields;
} fifo_ctrl_t;

#define INT_TRIGGER_LEVEL_1 0
#define INT_TRIGGER_LEVEL_4 1
#define INT_TRIGGER_LEVEL_8 2
#define INT_TRIGGER_LEVEL_14 3

typedef union
{
    u8 val;
    struct 
    {
        u8 int_pending : 1; /* pending if zero */
        u8 int_state : 2;
        u8 timeout_int_pending_uart16550 : 1;
        u8 reserved0 : 2;
        u8 fifo_buf_state : 2;
    } fields;
} int_id_t;

#define MODEM_STATUS_INT 0
#define TRANS_HOLDING_EMPTY_INT 1
#define RECV_DATA_AVAILABLE_INT 2
#define RECEIVER_LINE_STATUS_INT 3

#define MODEM_STATUS_PRIORITY 4
#define TRANS_HOLDING_EMPTY_PRIORITY 3
#define RECV_DATA_AVAILABLE_PRIORITY 2
#define RECEIVER_LINE_STATUS_PRIORITY 1

#define FIFO_BUF_STATE_NONE 0
#define FIFO_BUF_STATE_UNUSABLE 1
#define FIFO_BUF_STATE_ENABLED 2

typedef union
{
    u8 val;
    struct 
    {
        u8 data_terminal_ready : 1;
        u8 request_to_send : 1;
        u8 out1 : 1;
        u8 out2_enable_irq : 1;
        u8 loop : 1;
        u8 unused0 : 3;
    } fields;
} modem_ctrl_t;

typedef union
{
    u8 val;
    struct
    {
        u8 data_ready : 1;
        u8 overrun_error : 1;
        u8 parity_error : 1;
        u8 framing_error : 1;
        u8 break_indicator : 1;
        u8 trans_holding_empty : 1;
        u8 trans_empty : 1;
        u8 impending_error : 1;
    } fields;
} line_status_t;

typedef union
{
    u8 val;
    struct
    {
        u8 delta_clear_to_send : 1;
        u8 delta_data_set_ready : 1;
        u8 trailing_edge_ring_indicator : 1;
        u8 delta_data_carrier_detect : 1;
        u8 clear_to_send : 1;
        u8 data_set_ready : 1;
        u8 ring_indicator : 1;
        u8 data_carrier_detect : 1;
    } fields;
} modem_status_t;

inline bool __uart_is_received(u16 com_port)
{
    line_status_t status = {.val = inb(com_port + LINE_STATUS_OFFSET_R)};
    return status.fields.data_ready != 0;
}

inline char __uart_recv(u16 com_port)
{
    spin_until(__uart_is_received(com_port));
    return inb(com_port);
}

inline bool __uart_is_transmit_empty(u16 com_port)
{
    line_status_t status = {.val = inb(com_port + LINE_STATUS_OFFSET_R)};
    return status.fields.trans_holding_empty != 0;
}

inline void __uart_transmit(u16 com_port, char val)
{
    spin_until(__uart_is_transmit_empty(com_port));
    outb(com_port, val);
}

inline void __uart_transmit_str(u16 com_port, const char *str)
{
    while (*str != '\0') 
        __uart_transmit(com_port, *str++);
}

inline int uart_init(u16 com_port)
{
    /* disable interrupts */
    outb(com_port + INT_ENABLE_OFFSET_RW, 0);

    /* set the baud rate */
    line_ctrl_t line_ctrl = {.fields.divisor_latch_access = 1};

    outb(com_port + LINE_CTRL_OFFSET_RW, line_ctrl.val);

    outb(com_port + DIVISOR_LSB_OFFSET_RW, 3);
    outb(com_port + DIVISOR_MSB_OFFSET_RW, 0);

    line_ctrl.val = 0;
    line_ctrl.fields.data = CHAR_LEN_8;

    outb(com_port + LINE_CTRL_OFFSET_RW, line_ctrl.val);

    /* enable fifos */
    fifo_ctrl_t fifo_ctrl = {
        .fields = {
            .enable_fifos = 1,
            .clear_recv_fifo = 1,
            .clear_trans_fifo = 1,
            .int_trigger_level = INT_TRIGGER_LEVEL_14
        }
    };
    
    outb(com_port + FIFO_CTRL_OFFSET_W, fifo_ctrl.val);

    /* loopback test to check the device isnt fucked up */
    modem_ctrl_t modem_ctrl = {
        .fields = {
            .request_to_send = 1,
            .out1 = 1,
            .out2_enable_irq = 1,
            .loop = 1
        }
    };

    outb(com_port + MODEM_CTRL_OFFSET_RW, modem_ctrl.val);
    outb(com_port, TEST_BYTE);

    if (inb(com_port) != TEST_BYTE)
        return -EIO;

    /* set modem ctrl into regular operation, we know shit works */
    modem_ctrl.fields.data_terminal_ready = 1;
    modem_ctrl.fields.loop = 0;

    outb(com_port + MODEM_CTRL_OFFSET_RW, modem_ctrl.val);
    
    return 0;
}

#endif