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

#ifdef CONFIG_TIMER_SUNXI_TIMER

#include <_types.h>

#define TIMERS 0x01C20C00
#define TMR_IRQ_EN  *(volatile u32*)(TIMERS + 0x0)
#define TMR_IRQ_STA *(volatile u32*)(TIMERS + 0x4)
#define TMR_CTRL(n) *(volatile u32*)(TIMERS + ((n + 1) * 0x10))
#define TMR_INTV(n) *(volatile u32*)(TIMERS + ((n + 1) * 0x10) + 0x4)
#define TMR_CUR(n)  *(volatile u32*)(TIMERS + ((n + 1) * 0x10) + 0x8)

enum timer
{
    TIMER0 = 0,
    TIMER1
};

static inline void
timer_disable(enum timer n)
{
    TMR_IRQ_EN &= ~(1 << n);
}

static inline void
timer_enable(enum timer n)
{
    TMR_IRQ_EN |= 1 << n;
}

static inline void
timer_ack(enum timer n)
{
    TMR_IRQ_STA |= 1 << n;
}

enum timerclk
{
    TIMER_CLK_32KHZ = 0,
    TIMER_CLK_16KHZ,
    TIMER_CLK_8KHZ,
    TIMER_CLK_4KHZ,
    TIMER_CLK_2KHZ,
    TIMER_CLK_1KHZ,
    TIMER_CLK_500HZ,
    TIMER_CLK_250HZ,
    TIMER_CLK_24MHZ
};

static inline void
timer_config(enum timer n, bool single, enum timerclk clock)
{
    bool t24 = clock >= TIMER_CLK_24MHZ;
    u8 divider = clock & ~TIMER_CLK_24MHZ;
    TMR_CTRL(n) &= ~0xFFFFFFFC;
    TMR_CTRL(n) |= (single << 7) | (divider << 4) | (t24 << 2);
}

static inline u32
timer_interval_get(enum timer n)
{
    return TMR_INTV(n);
}

static inline void
timer_interval_set(enum timer n, u32 data)
{
    TMR_INTV(n) = data;
}

static inline u32
timer_current_get(enum timer n)
{
    return TMR_CUR(n);
}

static inline void
timer_current_set(enum timer n, u32 data)
{
    TMR_CUR(n) = data;
}

static inline void
timer_reload(enum timer n)
{
    TMR_CTRL(n) |= 0x2;
    while (TMR_CTRL(n) & 0x2);
}

static inline void
timer_start(enum timer n)
{
    TMR_CTRL(n) |= 0x1;
}

static inline void
timer_stop(enum timer n)
{
    TMR_CTRL(n) &= ~0x1;
}

#endif

#ifdef CONFIG_TIMER_SUNXI_TIMER

#include <_types.h>
#include <_utils.h>

#include <interface/gic.h>

static void
irq_timer(void)
{
    timer_ack(TIMER0);
}

extern bool
_timer_init(void)
{
    gic_config(CONFIG_SUNXI_TIMER_IRQ, irq_timer, true, 0);
    return true;
}

extern void
_timer_clean(void)
{
    gic_config(CONFIG_SUNXI_TIMER_IRQ, NULL, false, 0);
}

extern void
timer_csleep(const u32 n)
{
    timer_enable(TIMER0);
    timer_stop(TIMER0);

    timer_interval_set(TIMER0, n);
    timer_config(TIMER0, true, TIMER_CLK_24MHZ);
    timer_reload(TIMER0);

    timer_start(TIMER0);
    while (1)
    {
        gic_wait();
        u32 x = timer_current_get(TIMER0);
        if (x == 0 || x > n)
            break;
    }

    timer_stop(TIMER0);
    timer_disable(TIMER0);
}

extern void
timer_usleep(const u32 n)
{
    for (s64 a = n * 24; a > 0; a -= UINT32_MAX)
        timer_csleep((a < UINT32_MAX) ? a : UINT32_MAX);
}

extern void
timer_msleep(const u32 n)
{
    for (s64 a = n * 1000; a > 0; a -= UINT32_MAX)
        timer_usleep((a < UINT32_MAX) ? a : UINT32_MAX);
}

extern void
timer_sleep(const u32 n)
{
    for (s64 a = n * 1000; a > 0; a -= UINT32_MAX)
        timer_msleep((a < UINT32_MAX) ? a : UINT32_MAX);
}

#endif
