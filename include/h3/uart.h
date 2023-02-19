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

#ifndef H3_UART_H
#define H3_UART_H

#include <types.h>

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

enum uart_baud
{
    UART_BAUD_25      = 60000,
    UART_BAUD_50      = 30000,
    UART_BAUD_75      = 20000,
    UART_BAUD_110     = 13636,
    UART_BAUD_300     = 5000,
    UART_BAUD_600     = 2500,
    UART_BAUD_1200    = 1250,
    UART_BAUD_2400    = 625,
    UART_BAUD_4800    = 312,
    UART_BAUD_9600    = 156,
    UART_BAUD_14400   = 104,
    UART_BAUD_19200   = 78,
    UART_BAUD_38400   = 39,
    UART_BAUD_57600   = 26,
    UART_BAUD_115200  = 13,
    UART_BAUD_128K    = 12,
    UART_BAUD_256K    = 6,
    UART_BAUD_300K    = 5,
    UART_BAUD_375K    = 4,
    UART_BAUD_500K    = 3,
    UART_BAUD_750K    = 2,
    UART_BAUD_1500K   = 1
};

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
uart_config(enum uart p, enum uart_baud b, enum uart_char c,
            enum uart_parity i, enum uart_stop s, enum uart_flags f)
{
    UART_LCR(p) |= (1 << 7);
    UART_DLH(p) = (b >> 8) & 0xff;
    UART_DLL(p) = (b >> 0) & 0xff;
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

#endif
