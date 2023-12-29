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

#include <core/types.h>
#include <core/utils.h>
#include <core/drivers.h>

#include <core/mem.h>

enum
{
    IVT_RESET = 0,
    IVT_UNDEF,
    IVT_SWI,
    IVT_PREFETCH,
    IVT_DATA,
    IVT_UNUSED,
    IVT_IRQ,
    IVT_FIQ
};

extern void *__ivt[8];

#define interrupt(type) \
    __attribute__((target("general-regs-only"))) \
    __attribute__((isr(#type))) void

#define ICCICR(cpu)  *(volatile u32*)(cpu + 0x0)
#define ICCPMR(cpu)  *(volatile u32*)(cpu + 0x4)
#define ICCIAR(cpu)  *(volatile u32*)(cpu + 0xC)
#define ICCEOIR(cpu) *(volatile u32*)(cpu + 0x10)
#define ICCHPIR(cpu) *(volatile u32*)(cpu + 0x18)

#define ICDDCR(dist)     *(volatile u32*)(dist + 0x0)
#define ICDISER(dist, n) *(volatile u32*)(dist + 0x100 + (n * 4))
#define ICDICER(dist, n) *(volatile u32*)(dist + 0x180 + (n * 4))
#define ICDIPR(dist, n)  *(volatile u32*)(dist + 0x400 + (n * 4))
#define ICDIPTR(dist, n) *(volatile u32*)(dist + 0x800 + (n * 4))
#define ICDICFR(dist, n) *(volatile u32*)(dist + 0xC00 + (n * 4))

enum intr_core
{
    INTR_CORE0 = 0,
    INTR_CORE1,
    INTR_CORE2,
    INTR_CORE3,

    INTR_CORE_NONE = 255
};

static inline u16
intr_info(u32 cpu, enum intr_core *c)
{
    u32 info = ICCIAR(cpu);
    *c = info >> 10;
    return info & 0x3FF;
}

static inline void
intr_ack(u32 cpu, enum intr_core c, u16 n)
{
    ICCEOIR(cpu) = (c << 10) | (n & 0x3FF);
}

static inline void
arm_disable_irq(void)
{
    u32 cpsr = 0;
    asm volatile ("mrs %0, cpsr\n" : "=r" (cpsr));
    cpsr |= 0x80;
    asm volatile ("msr cpsr, %0\n" : : "r" (cpsr));
}

static inline void
arm_enable_irq(void)
{
    u32 cpsr = 0;
    asm volatile ("mrs %0, cpsr\n" : "=r" (cpsr));
    cpsr &= 0xFFFFFF7F;
    asm volatile ("msr cpsr, %0\n" : : "r" (cpsr));
}

static inline void
arm_disable_fiq(void)
{
    u32 cpsr = 0;
    asm volatile ("mrs %0, cpsr\n" : "=r" (cpsr));
    cpsr |= 0x40;
    asm volatile ("msr cpsr, %0\n" : : "r" (cpsr));
}

static inline void
arm_enable_fiq(void)
{
    u32 cpsr = 0;
    asm volatile ("mrs %0, cpsr\n" : "=r" (cpsr));
    cpsr &= 0xFFFFFFBF;
    asm volatile ("msr cpsr, %0\n" : : "r" (cpsr));
}

static inline void
arm_wait_interrupts(void)
{
    asm volatile ("wfi");
}

static inline void
gic_enable(u32 cpu)
{
    ICCICR(cpu) = 0x3;
}

static inline void
gic_disable(u32 cpu)
{
    ICCICR(cpu) = 0;
}

static inline void
gic_priority(u32 cpu, u32 value)
{
    ICCPMR(cpu) = value;
}

static inline void
gic_enable_dist(u32 dist)
{
    ICDDCR(dist) = 0x3;
}

static inline void
gic_disable_dist(u32 dist)
{
    ICDDCR(dist) = 0;
}

static inline void
gic_intr_activity(u32 dist, u16 n, bool enable)
{
    u8 reg = n / 32;
    u8 off = n % 32;

    if (enable)
        ICDISER(dist, reg) = (1 << off);
    else
        ICDICER(dist, reg) = (1 << off);
}

static inline void
gic_intr_target(u32 dist, u16 n, enum intr_core c)
{
    u8 reg = n / 4;
    u8 off = n % 4;

    ICDIPTR(dist, reg) &= ~(0xFF << (off * 8));
    if (c != INTR_CORE_NONE)
        ICDIPTR(dist, reg) |= (1 << ((off * 8) + c));
}

static inline void
gic_intr_priority(u32 dist, u16 n, u8 priority)
{
    u8 reg = n / 4;
    u8 off = n % 4;

    ICDIPTR(dist, reg) &= ~(0xFF << (off * 8));
    ICDIPTR(dist, reg) |= priority << (off * 8);
}

static inline void
gic_intr_sensitivity(u32 dist, u16 n, bool edge, bool high)
{
    u8 reg = n / 16;
    u8 off = n % 16;

    ICDICFR(dist, reg) &= ~(0x3 << (off * 2));
    ICDICFR(dist, reg) |= ((edge << 1) | high) << (off * 2);
}

struct gic
{
    u32 cpu, dist;
    void (*irqs[256])(void);
};
static struct gic *gic = NULL;

static interrupt(undef)
handler_undef(void)
{
    log_s("Undefined Instruction");
    for (;;)
        arm_wait_interrupts();
}

static interrupt(swi)
handler_swi(void)
{
    // TODO add a handler for software interrupts
    log_s("Unhandled Supervisor Call");
    for (;;)
        arm_wait_interrupts();
}

static interrupt(abort)
handler_prefetch(void)
{
    log_s("Prefetch Abort");
    for (;;)
        arm_wait_interrupts();
}

static interrupt(abort)
handler_data(void)
{
    log_s("Data Abort");
    for (;;)
        arm_wait_interrupts();
}

static interrupt(irq)
handler_irq(void)
{
    if (gic)
    {
        enum intr_core c = 0;
        u16 n = intr_info(gic->cpu, &c);

        if (gic->irqs[n])
            gic->irqs[n]();

        intr_ack(gic->cpu, c, n);
    }
}

static interrupt(fiq)
handler_fiq(void)
{
    if (gic)
    {
        enum intr_core c = 0;
        u16 n = intr_info(gic->cpu, &c);

        intr_ack(gic->cpu, c, n);
    }
}

extern bool
pic_config(void *ctx, u16 n, void (*f)(void), bool enable, u8 priority,
           bool edge, bool high)
{
    bool ret = false;

    if (ctx && ctx == gic)
    {
        arm_disable_irq();

        gic_disable_dist(gic->dist);
        gic_disable(gic->cpu);
        gic_priority(gic->cpu, 0xFF);

        gic_intr_target(gic->dist, n, INTR_CORE_NONE);
        gic_intr_activity(gic->dist, n, false);
        gic_intr_priority(gic->dist, n, priority);
        gic_intr_sensitivity(gic->dist, n, edge, high);

        if (enable)
        {
            gic->irqs[n] = f;

            gic_intr_activity(gic->dist, n, true);
            gic_intr_target(gic->dist, n, INTR_CORE0);
        }
        else
            gic->irqs[n] = NULL;

        gic_enable(gic->cpu);
        gic_enable_dist(gic->dist);

        arm_enable_irq();

        ret = true;
    }

    return ret;
}

extern void
pic_wait(void *ctx)
{
    if (ctx && ctx == gic)
        arm_wait_interrupts();
}

static void
init(void **ctx, u32 cpu, u32 dist)
{
    if (!gic)
    {
        gic = mem_new(sizeof(struct gic));
        if (gic)
        {
            gic->cpu = cpu;
            gic->dist = dist;
            *ctx = gic;

            __ivt[IVT_UNDEF]    = handler_undef;
            __ivt[IVT_SWI]      = handler_swi;
            __ivt[IVT_PREFETCH] = handler_prefetch;
            __ivt[IVT_DATA]     = handler_data;
            __ivt[IVT_IRQ]      = handler_irq;
            __ivt[IVT_FIQ]      = handler_fiq;
        }
    }
}

static void
clean(void *ctx)
{
    if (ctx && ctx == gic)
    {
        arm_disable_irq();
        gic_disable_dist(gic->dist);
        gic_disable(gic->cpu);

        gic = mem_del(gic);
    }
}

static bool
config_get(void *ctx, union config *cfg)
{
    bool ret = false;

    if (ctx && ctx == gic)
    {
        cfg->pic.config = pic_config;
        cfg->pic.wait = pic_wait;

        ret = true;
    }

    return ret;
}

DECLARE_DRIVER(pic, arm_gic)
{
    .init = init, .clean = clean,
    .config.get = config_get
};
