/*
This file is part of vermillion.

Vermillion is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3.

Vermillion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <_types.h>
#include <vermillion/drivers.h>

enum uart
{
    UART0 = 0x01c28000,
    UART1 = 0x01c28400,
    UART2 = 0x01c28800,
    UART3 = 0x01c28c00,
    R_UART = 0x01f02800
};

#define UART_BUF(p) *(volatile u32*)(p + 0x00)
#define UART_DLL(p) *(volatile u32*)(p + 0x00)
#define UART_DLH(p) *(volatile u32*)(p + 0x04)
#define UART_IER(p) *(volatile u32*)(p + 0x04)
#define UART_IIR(p) *(volatile u32*)(p + 0x08)
#define UART_FCR(p) *(volatile u32*)(p + 0x08)
#define UART_LCR(p) *(volatile u32*)(p + 0x0C)
#define UART_MCR(p) *(volatile u32*)(p + 0x10)
#define UART_LSR(p) *(volatile u32*)(p + 0x14)
#define UART_MSR(p) *(volatile u32*)(p + 0x18)
#define UART_SCH(p) *(volatile u32*)(p + 0x1C)
#define UART_USR(p) *(volatile u32*)(p + 0x7C)
#define UART_TFL(p) *(volatile u32*)(p + 0x80)
#define UART_RFL(p) *(volatile u32*)(p + 0x84)
#define UART_HLT(p) *(volatile u32*)(p + 0xA4)

static inline char
uart_read(enum uart p)
{
    while (!(UART_LSR(p) & (1 << 0)));
    return UART_BUF(p);
}

static inline void
uart_write(enum uart p, char c)
{
    while (!(UART_LSR(p) & (1 << 5)));
    UART_BUF(p) = c;
}

enum uart_char
{
    UART_CHAR_5B,
    UART_CHAR_6B,
    UART_CHAR_7B,
    UART_CHAR_8B
};

enum uart_parity
{
    UART_PARITY_NONE,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
};

enum uart_stop
{
    UART_STOP_1B,
    UART_STOP_1HB,
    UART_STOP_2B
};

enum uart_flags
{
    UART_FLAG_NONE,
    UART_FLAG_AFC,
    UART_FLAG_LOOP
};

static inline void
uart_config(enum uart p, u16 divider, enum uart_char c,
            enum uart_parity i, enum uart_stop s, enum uart_flags f)
{
    UART_LCR(p) |= (1 << 7);
    UART_DLH(p) = (divider >> 8) & 0xff;
    UART_DLL(p) = (divider >> 0) & 0xff;
    UART_LCR(p) &= ~(1 << 7);

    UART_LCR(p) &= ~0x3;
    UART_LCR(p) |= c & 0x3;

    switch (i)
    {
        case UART_PARITY_NONE:
            UART_LCR(p) &= ~(1 << 3);
            break;
        case UART_PARITY_ODD:
            UART_LCR(p) &= ~0x38;
            UART_LCR(p) |= (1 << 3);
            break;
        case UART_PARITY_EVEN:
            UART_LCR(p) &= ~0x38;
            UART_LCR(p) |= (1 << 3) | (1 << 4);
            break;
        default:
            break;
    }

    switch (s)
    {
        case UART_STOP_1B:
            UART_LCR(p) &= ~(1 << 2);
            break;
        case UART_STOP_1HB:
        case UART_STOP_2B:
            UART_LCR(p) |= (1 << 2);
            break;
    }

    if (f & UART_FLAG_AFC)
        UART_MCR(p) |= (1 << 5);
    else
        UART_MCR(p) &= ~(1 << 5);

    if (f & UART_FLAG_LOOP)
        UART_MCR(p) |= (1 << 4);
    else
        UART_MCR(p) &= ~(1 << 4);
}

enum intr
{
    UART_INTR_MODEM_ST = 0,
    UART_INTR_NONE = 1,
    UART_INTR_EMPTY_TX = 2,
    UART_INTR_DATA_RX = 4,
    UART_INTR_LINE_ST = 6,
    UART_INTR_BUSY = 7,
    UART_INTR_CHAR = 12
};

static inline void
uart_intr_gate(enum uart p, bool data_rx, bool empty_tx, bool line_st,
               bool modem_st, bool prog_empty_tx)
{
    UART_IER(p) &= ~0x8f;
    UART_IER(p) |= data_rx | empty_tx << 1 | line_st << 2 | modem_st << 3 |
                   prog_empty_tx << 7;
}

static inline void
uart_intr_info(enum uart p, enum intr *i, bool *fifo)
{
    *i = UART_IIR(p) & 0xf;
    *fifo = UART_IIR(p) & (1 << 6);
}

static inline void
uart_break(enum uart p, bool status)
{
    if (status)
        UART_LCR(p) |= (1 << 6);
    else
        UART_LCR(p) &= ~(1 << 6);
}

static inline void
uart_fifo_enable(enum uart p)
{
    UART_FCR(p) |= 0x1;
}

static inline void
uart_fifo_disable(enum uart p)
{
    UART_FCR(p) &= ~0x1;
}

static inline void
uart_fifo_reset_rx(enum uart p)
{
    UART_FCR(p) |= 0x2;
}

static inline void
uart_fifo_reset_tx(enum uart p)
{
    UART_FCR(p) |= 0x4;
}

enum uart_fifo_rx_trig
{
    UART_FIFO_RX_TRIG_1CHAR = 0,
    UART_FIFO_RX_TRIG_QUARTER = 1,
    UART_FIFO_RX_TRIG_HALF = 2,
    UART_FIFO_RX_TRIG_2LESS = 3
};

enum uart_fifo_tx_trig
{
    UART_FIFO_TX_TRIG_EMPTY = 0,
    UART_FIFO_TX_TRIG_2CHARS = 1,
    UART_FIFO_TX_TRIG_QUARTER = 2,
    UART_FIFO_TX_TRIG_HALF = 3
};

enum uart_fifo_dma_mode
{
    UART_FIFO_DMA_MODE_0 = 0,
    UART_FIFO_DMA_MODE_1 = 1,
};

static inline void
uart_fifo_config(enum uart p, enum uart_fifo_rx_trig r,
                 enum uart_fifo_tx_trig t, enum uart_fifo_dma_mode d)
{
    UART_FCR(p) &= ~0xf8;
    UART_FCR(p) |= r << 6 | t << 4 | d << 3;
}

static inline u8
uart_fifo_level_tx(enum uart p)
{
    return UART_TFL(p) & 0x7f;
}

static inline u8
uart_fifo_level_rx(enum uart p)
{
    return UART_RFL(p) & 0x7f;
}

static inline void
uart_flow_control(enum uart p, bool rts, bool dtr)
{
    UART_MCR(p) &= ~0x3;
    UART_MCR(p) |= !rts << 1 | !dtr;
}

enum uart_status
{
    UART_STATUS_READY = 0,
    UART_STATUS_EOVERRUN,
    UART_STATUS_EPARITY,
    UART_STATUS_EFRAMING,
    UART_STATUS_BREAK,
    UART_STATUS_THRE,
    UART_STATUS_TXEMPTY,
    UART_STATUS_ERXFIFO,

    UART_STATUS_CTS_D = 8,
    UART_STATUS_DSR_D,
    UART_STATUS_RING_D,
    UART_STATUS_DCD_D,
    UART_STATUS_CTS,
    UART_STATUS_DSR,
    UART_STATUS_RING,
    UART_STATUS_DCD,

    UART_STATUS_BUSY = 16,
    UART_STATUS_TXFIFO_NF,
    UART_STATUS_TXFIFO_E,
    UART_STATUS_RXFIFO_NE,
    UART_STATUS_RXFIFO_F
};

static inline bool
uart_status(enum uart p, enum uart_status s)
{
    bool ret = false;

    if (s >= UART_STATUS_BUSY)
        ret = UART_USR(p) & (1 << (s - 16));
    else if (s >= UART_STATUS_CTS_D)
        ret = UART_MSR(p) & (1 << (s - 8));
    else
        ret = UART_LSR(p) & (1 << s);

    return ret;
}

static inline void
uart_halt_config(enum uart p, bool active, bool busy_change,
                 bool tx_invert, bool rx_invert)
{
    UART_HLT(p) &= 0x33;
    UART_HLT(p) |= rx_invert << 5 | tx_invert << 4 |
                   busy_change << 2 | active;
}

static inline void
uart_halt_change_update(enum uart p)
{
    UART_HLT(p) |= 1 << 2;
}

static u32 ports[] = {UART0, UART1, UART2, UART3, R_UART};
static bool
serial_config(u8 port, u32 baud, u8 ch, u8 parity, u8 stop)
{
    bool ret = true;

    ret = port < (sizeof(ports) / sizeof(u32));

    u32 divider = 1500000 / baud;
    if (ret)
        ret = (divider && divider <= UINT16_MAX);

    enum uart_char uc = UART_CHAR_5B;
    if (ret)
    {
        switch (ch)
        {
            case DRIVER_SERIAL_CHAR_5B:
                break;
            case DRIVER_SERIAL_CHAR_6B:
                uc = UART_CHAR_6B;
                break;
            case DRIVER_SERIAL_CHAR_7B:
                uc = UART_CHAR_7B;
                break;
            case DRIVER_SERIAL_CHAR_8B:
                uc = UART_CHAR_8B;
                break;
            default:
                ret = false;
                break;
        }
    }

    enum uart_parity up = UART_PARITY_NONE;
    if (ret)
    {
        switch (parity)
        {
            case DRIVER_SERIAL_PARITY_NONE:
                break;
            case DRIVER_SERIAL_PARITY_ODD:
                up = UART_PARITY_ODD;
                break;
            case DRIVER_SERIAL_PARITY_EVEN:
                up = UART_PARITY_EVEN;
                break;
            default:
                ret = false;
                break;
        }
    }

    enum uart_stop us = UART_STOP_1B;
    if (ret)
    {
        switch (stop)
        {
            case DRIVER_SERIAL_STOP_1B:
                break;
            case DRIVER_SERIAL_STOP_1HB:
                us = UART_STOP_1HB;
                break;
            case DRIVER_SERIAL_STOP_2B:
                us = UART_STOP_2B;
                break;
            default:
                ret = false;
                break;
        }
    }

    if (ret)
        uart_config(ports[port], divider, uc, up, us, UART_FLAG_NONE);

    return ret;
}

static u8
serial_read(u8 port)
{
    u8 ret = 0;

    if (port < (sizeof(ports) / sizeof(u32)))
        ret = uart_read(ports[port]);

    return ret;
}

static void
serial_write(u8 port, u16 data)
{
    if (port < (sizeof(ports) / sizeof(u32)))
        uart_write(ports[port], data);
}

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)
static bool ruart_init(void){ return (APB0_GATE = 1); }
static bool uart0_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(0, baud, ch, parity, stop); }
static bool uart1_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(1, baud, ch, parity, stop); }
static bool uart2_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(2, baud, ch, parity, stop); }
static bool uart3_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(3, baud, ch, parity, stop); }
static bool ruart_config(u32 baud, u8 ch, u8 parity, u8 stop)
{ return serial_config(4, baud, ch, parity, stop); }
static u8 uart0_read(void){ return serial_read(0); }
static u8 uart1_read(void){ return serial_read(1); }
static u8 uart2_read(void){ return serial_read(2); }
static u8 uart3_read(void){ return serial_read(3); }
static u8 ruart_read(void){ return serial_read(4); }
static void uart0_write(u16 data){ return serial_write(0, data); }
static void uart1_write(u16 data){ return serial_write(1, data); }
static void uart2_write(u16 data){ return serial_write(2, data); }
static void uart3_write(u16 data){ return serial_write(3, data); }
static void ruart_write(u16 data){ return serial_write(4, data); }

static const struct driver sunxi_ruart =
{
    .name = "Sunxi R_UART",
    .init = ruart_init, .clean = NULL,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = ruart_config,
    .routines.serial.read   = ruart_read,
    .routines.serial.write  = ruart_write
};
driver_register(sunxi_ruart);

static const struct driver sunxi_uart3 =
{
    .name = "Sunxi UART3",
    .init = NULL, .clean = NULL,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = uart3_config,
    .routines.serial.read   = uart3_read,
    .routines.serial.write  = uart3_write
};
driver_register(sunxi_uart3);

static const struct driver sunxi_uart2 =
{
    .name = "Sunxi UART2",
    .init = NULL, .clean = NULL,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = uart2_config,
    .routines.serial.read   = uart2_read,
    .routines.serial.write  = uart2_write
};
driver_register(sunxi_uart2);

static const struct driver sunxi_uart1 =
{
    .name = "Sunxi UART1",
    .init = NULL, .clean = NULL,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = uart1_config,
    .routines.serial.read   = uart1_read,
    .routines.serial.write  = uart1_write
};
driver_register(sunxi_uart1);

static const struct driver sunxi_uart0 =
{
    .name = "Sunxi UART0",
    .init = NULL, .clean = NULL,
    .type = DRIVER_TYPE_SERIAL,
    .routines.serial.config = uart0_config,
    .routines.serial.read   = uart0_read,
    .routines.serial.write  = uart0_write
};
driver_register(sunxi_uart0);
