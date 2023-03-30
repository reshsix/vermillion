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
#include <_utils.h>
#include <vermillion/drivers.h>

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

/* Exported functions */

static const struct driver *gic0 = NULL;
static bool
timer_init(u16 irq, void (*f)(void))
{
    gic0 = driver_find(DRIVER_TYPE_GIC, 0);
    gic0->routines.gic.config(irq, f, true, 0);
    return true;
}

static void
timer_clean(u16 irq)
{
    gic0->routines.gic.config(irq, NULL, false, 0);
}

static u32
timer_clock(void)
{
    return 24000000;
}

static void
timer_csleep(enum timer t, const u32 n)
{
    timer_enable(t);
    timer_stop(t);

    timer_interval_set(t, n);
    timer_config(t, true, TIMER_CLK_24MHZ);
    timer_reload(t);

    timer_start(t);
    while (1)
    {
        gic0->routines.gic.wait();
        u32 x = timer_current_get(t);
        if (x == 0 || x > n)
            break;
    }

    timer_stop(t);
    timer_disable(t);
}

static void
timer_usleep(enum timer t, const u32 n)
{
    for (s64 a = n * 24; a > 0; a -= UINT32_MAX)
        timer_csleep(t, (a < UINT32_MAX) ? a : UINT32_MAX);
}

static void
timer_msleep(enum timer t, const u32 n)
{
    for (s64 a = n * 1000; a > 0; a -= UINT32_MAX)
        timer_usleep(t, (a < UINT32_MAX) ? a : UINT32_MAX);
}

static void
timer_sleep(enum timer t, const u32 n)
{
    for (s64 a = n * 1000; a > 0; a -= UINT32_MAX)
        timer_msleep(t, (a < UINT32_MAX) ? a : UINT32_MAX);
}

static void timer0_ack(void){ timer_ack(TIMER0); }
static void timer1_ack(void){ timer_ack(TIMER1); }
static bool timer0_init(void){ return timer_init(CONFIG_SUNXI_TIMER0_IRQ,
                                                 timer0_ack); }
static bool timer1_init(void){ return timer_init(CONFIG_SUNXI_TIMER1_IRQ,
                                                 timer1_ack); }
static void timer0_clean(void) { timer_clean(CONFIG_SUNXI_TIMER0_IRQ); }
static void timer1_clean(void) { timer_clean(CONFIG_SUNXI_TIMER1_IRQ); }
static void timer0_csleep(const u32 n) { timer_csleep(TIMER0, n); }
static void timer1_csleep(const u32 n) { timer_csleep(TIMER1, n); }
static void timer0_usleep(const u32 n) { timer_usleep(TIMER0, n); }
static void timer1_usleep(const u32 n) { timer_usleep(TIMER1, n); }
static void timer0_msleep(const u32 n) { timer_msleep(TIMER0, n); }
static void timer1_msleep(const u32 n) { timer_msleep(TIMER1, n); }
static void timer0_sleep(const u32 n)  { timer_sleep(TIMER0, n); }
static void timer1_sleep(const u32 n)  { timer_sleep(TIMER1, n); }

static const struct driver sunxi_timer1 =
{
    .name = "Sunxi Timer 1",
    .init = timer1_init, .clean = timer1_clean,
    .type = DRIVER_TYPE_TIMER,
    .routines.timer.clock  = timer_clock,
    .routines.timer.csleep = timer1_csleep,
    .routines.timer.usleep = timer1_usleep,
    .routines.timer.msleep = timer1_msleep,
    .routines.timer.sleep =  timer1_sleep
};
driver_register(sunxi_timer1);

static const struct driver sunxi_timer0 =
{
    .name = "Sunxi Timer 0",
    .init = timer0_init, .clean = timer0_clean,
    .type = DRIVER_TYPE_TIMER,
    .routines.timer.clock  = timer_clock,
    .routines.timer.csleep = timer0_csleep,
    .routines.timer.usleep = timer0_usleep,
    .routines.timer.msleep = timer0_msleep,
    .routines.timer.sleep =  timer0_sleep
};
driver_register(sunxi_timer0);
