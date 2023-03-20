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

#ifndef H3_INTERRUPTS_H
#define H3_INTERRUPTS_H

#include <types.h>

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

extern void *ivt[8];

#define interrupt(type) \
    __attribute__((target("general-regs-only"))) \
    __attribute__((isr(#type))) void

#define GIC_CPU    CONFIG_ARM_GIC_CPU
#define ICCICR     *(volatile u32*)(GIC_CPU + 0x0)
#define ICCPMR     *(volatile u32*)(GIC_CPU + 0x4)
#define ICCIAR     *(volatile u32*)(GIC_CPU + 0xC)
#define ICCEOIR    *(volatile u32*)(GIC_CPU + 0x10)
#define ICCHPIR    *(volatile u32*)(GIC_CPU + 0x18)

#define GIC_DIST   CONFIG_ARM_GIC_DIST
#define ICDDCR     *(volatile u32*)(GIC_DIST + 0x0)
#define ICDISER(n) *(volatile u32*)(GIC_DIST + 0x100 + (n * 4))
#define ICDICER(n) *(volatile u32*)(GIC_DIST + 0x180 + (n * 4))
#define ICDIPR(n)  *(volatile u32*)(GIC_DIST + 0x400 + (n * 4))
#define ICDIPTR(n) *(volatile u32*)(GIC_DIST + 0x800 + (n * 4))
#define ICDICFR(n) *(volatile u32*)(GIC_DIST + 0xC00 + (n * 4))

enum intr_core
{
    INTR_CORE0 = 0,
    INTR_CORE1,
    INTR_CORE2,
    INTR_CORE3,

    INTR_CORE_NONE = 255
};

enum intr_number
{
    IRQ_TIMER0 = 50,
    IRQ_TIMER1
};

static inline u32
intr_swi_number(void)
{
    u32 number = 0;
    asm volatile ("ldr %0, [lr, #-4]\n" : "=r"(number));
    return number & 0x00ffffff;
}

static inline u32
intr_abort_prefetch_address(void)
{
    u32 addr = 0;
    asm volatile ("mov %0, [lr, #-4]\n" : "=r"(addr));
    return addr;
}

static inline u32
intr_abort_data_address(void)
{
    u32 addr = 0;
    asm volatile ("mov %0, [lr, #-8]\n" : "=r"(addr));
    return addr;
}

static inline u32
intr_undef_address(void)
{
    return intr_abort_prefetch_address();
}

static inline enum intr_number
intr_irq_info(enum intr_core *c)
{
    u32 info = ICCIAR;
    *c = info >> 10;
    return info & 0x3FF;
}

static inline enum intr_number
intr_fiq_info(enum intr_core *c)
{
    return intr_irq_info(c);
}

static inline void
intr_irq_ack(enum intr_core c, enum intr_number n)
{
    ICCEOIR = (c << 10) | (n & 0x3FF);
}

static inline void
intr_fiq_ack(enum intr_core c, enum intr_number n)
{
    intr_irq_ack(c, n);
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
gic_enable(void)
{
    ICCICR = 0x3;
}

static inline void
gic_disable(void)
{
    ICCICR = 0;
}

static inline void
gic_enable_dist(void)
{
    ICDDCR = 0x3;
}

static inline void
gic_disable_dist(void)
{
    ICDDCR = 0;
}

static inline void
gic_priority(u32 value)
{
    ICCPMR = value;
}

static inline void
gic_intr_activity(enum intr_number n, bool enable)
{
    u8 reg = n / 32;
    u8 off = n % 32;

    if (enable)
        ICDISER(reg) = (1 << off);
    else
        ICDICER(reg) = (1 << off);
}

static inline void
gic_intr_target(enum intr_number n, enum intr_core c)
{
    u8 reg = n / 4;
    u8 off = n % 4;

    ICDIPTR(reg) &= ~(0xFF << (off * 8));
    if (c != INTR_CORE_NONE)
        ICDIPTR(reg) |= (1 << ((off * 8) + c));
}

static inline void
gic_intr_priority(enum intr_number n, u8 priority)
{
    u8 reg = n / 4;
    u8 off = n % 4;

    ICDIPTR(reg) &= ~(0xFF << (off * 8));
    ICDIPTR(reg) |= priority << (off * 8);
}

static inline void
gic_intr_sensitivity(enum intr_number n, bool edge, bool high)
{
    u8 reg = n / 16;
    u8 off = n % 16;

    ICDICFR(reg) &= ~(0x3 << (off * 2));
    ICDICFR(reg) |= ((edge << 1) | high) << (off * 2);
}

#endif
