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

#include <general/types.h>
#include <general/mem.h>

#include <thread/critical.h>

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/classes/pic.h>

#include <system/log.h>

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
    [[gnu::target("general-regs-only")]] [[gnu::isr(#type)]] \
    static void

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
    bool enabled;
    u32 cpu, dist;

    struct pic_irq irqs[256];
    struct pic_swi swis[256];

    u8 irq_stack[CONFIG_STACK_SIZE];
    u8 swi_id;
    void *swi_data;
};
static struct gic *gic = NULL;

interrupt(undef) handler_undef(void)
{
    log("Undefined Instruction");
    for (;;)
        arm_wait_interrupts();
}

interrupt(swi) handler_swi(void)
{
    critical if (gic)
    {
        u8 n = *((u32*)(__builtin_return_address(0) - 4)) % 256;
        if (n == 0x0)
        {
            struct pic_swi *swi = &(gic->swis[gic->swi_id]);
            if (gic->enabled && swi->enabled && swi->handler)
                swi->handler(swi->arg, gic->swi_data);
        }
        else
            log("Unhandled supervisor call");
    }
}

interrupt(abort) handler_prefetch(void)
{
    log("Prefetch Abort");
    for (;;)
        arm_wait_interrupts();
}

interrupt(abort) handler_data(void)
{
    log("Data Abort");
    for (;;)
        arm_wait_interrupts();
}

interrupt(irq) handler_irq(void)
{
    critical if (gic)
    {
        enum intr_core c = 0;
        u16 n = intr_info(gic->cpu, &c);
        intr_ack(gic->cpu, c, n);

        struct pic_irq *irq = &(gic->irqs[n]);
        if (irq->enabled && irq->handler)
            irq->handler(irq->arg);
    }
}

interrupt(fiq) handler_fiq(void)
{
    critical if (gic)
    {
        enum intr_core c = 0;
        u16 n = intr_info(gic->cpu, &c);

        intr_ack(gic->cpu, c, n);
    }
}

static void [[gnu::naked]]
stack_irq(void *addr)
{
    (void)addr;

    asm volatile ("msr CPSR_c, #0b11010010");
    asm volatile ("mov sp, r0");

    asm volatile ("msr CPSR_c, #0b11010011");
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

            stack_irq((void*)((u32)gic->irq_stack + CONFIG_STACK_SIZE));
            gic_priority(gic->cpu, 0xFF);

        }
    }
}

static void
clean(void *ctx)
{
    if (ctx == gic)
    {
        arm_disable_irq();
        gic_disable_dist(gic->dist);
        gic_disable(gic->cpu);

        __ivt[IVT_UNDEF]    = NULL;
        __ivt[IVT_SWI]      = NULL;
        __ivt[IVT_PREFETCH] = NULL;
        __ivt[IVT_DATA]     = NULL;
        __ivt[IVT_IRQ]      = NULL;
        __ivt[IVT_FIQ]      = NULL;

        gic = mem_del(gic);
    }
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *length)
{
    bool ret = true;

    if (ctx == gic)
    {
        switch (idx)
        {
            case 0:
                *width = sizeof(bool);
                *length = 1;
                break;

            case 1:
                *width = sizeof(struct pic_irq);
                *length = 256;
                break;

            case 2:
                *width = sizeof(struct pic_swi);
                *length = 256;
                break;

            case 3:
                *width = 0;
                *length = 1;
                break;

            case 4:
                *width = sizeof(void *);
                *length = 256;
                break;

            default:
                ret = false;
                break;
        }
    }
    else
        ret = false;

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    if (ctx == gic)
    {
        switch (idx)
        {
            case 0:
                ret = (block == 0);

                if (ret)
                    mem_copy(buffer, &(gic->enabled), sizeof(bool));
                break;

            case 1:
                ret = (block < 256);

                if (ret)
                    mem_copy(buffer, &(gic->irqs[block]),
                             sizeof(struct pic_irq));
                break;

            case 2:
                ret = (block < 256);

                if (ret)
                    mem_copy(buffer, &(gic->swis[block]),
                             sizeof(struct pic_swi));
                break;

            case 3:
                ret = (block == 0);

                if (ret)
                    arm_wait_interrupts();
                break;
        }
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    if (ctx == gic)
    {
        switch (idx)
        {
            case 0:
                ret = (block == 0);

                if (ret)
                {
                    mem_copy(&(gic->enabled), buffer, sizeof(bool));
                    if (gic->enabled)
                    {
                        gic_enable(gic->cpu);
                        gic_enable_dist(gic->dist);
                        arm_enable_irq();
                    }
                    else
                    {
                        arm_disable_irq();
                        gic_disable_dist(gic->dist);
                        gic_disable(gic->cpu);
                    }
                }
                break;

            case 1:
                ret = (block < 256);

                if (ret)
                {
                    struct pic_irq irq = {0};
                    mem_copy(&irq, buffer, sizeof(struct pic_irq));

                    if (gic->enabled)
                    {
                        arm_disable_irq();
                        gic_disable_dist(gic->dist);
                        gic_disable(gic->cpu);
                    }

                    gic_intr_target(gic->dist, block, INTR_CORE_NONE);
                    gic_intr_activity(gic->dist, block, false);
                    gic_intr_priority(gic->dist, block, 0);

                    switch (irq.level)
                    {
                        case PIC_LEVEL_L:
                            gic_intr_sensitivity(gic->dist, block,
                                                 false, false);
                            break;

                        case PIC_LEVEL_H:
                            gic_intr_sensitivity(gic->dist, block,
                                                 false, true);
                            break;

                        case PIC_EDGE_L:
                            gic_intr_sensitivity(gic->dist, block,
                                                 true, false);
                            break;

                        case PIC_EDGE_H:
                            gic_intr_sensitivity(gic->dist, block,
                                                 true, true);
                            break;

                        default:
                            ret = false;
                            break;
                    }

                    if (ret)
                    {
                        mem_copy(&(gic->irqs[block]), &irq,
                                 sizeof(struct pic_irq));

                        if (irq.enabled)
                        {
                            gic_intr_activity(gic->dist, block, true);
                            gic_intr_target(gic->dist, block, gic->cpu);
                        }
                    }

                    if (gic->enabled)
                    {
                        gic_enable(gic->cpu);
                        gic_enable_dist(gic->dist);
                        arm_enable_irq();
                    }
                }
                break;

            case 2:
                ret = (block < 256);

                if (ret)
                    mem_copy(&(gic->swis[block]), buffer,
                             sizeof(struct pic_swi));
                break;

            case 3:
                ret = (block == 0);

                if (ret)
                    arm_wait_interrupts();
                break;

            case 4:
                ret = (block < 256);

                if (ret)
                {
                    gic->swi_id = block;
                    mem_copy(&(gic->swi_data), buffer, sizeof(void *));
                    asm volatile ("svc #0x0");
                }
        }
    }

    return ret;
}

drv_decl (pic, arm_gic)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
