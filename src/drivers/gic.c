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
#include <signal.h>
#include <vermillion/drivers.h>

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

#define GIC_CPU    CONFIG_GIC_CPU
#define ICCICR     *(volatile u32*)(GIC_CPU + 0x0)
#define ICCPMR     *(volatile u32*)(GIC_CPU + 0x4)
#define ICCIAR     *(volatile u32*)(GIC_CPU + 0xC)
#define ICCEOIR    *(volatile u32*)(GIC_CPU + 0x10)
#define ICCHPIR    *(volatile u32*)(GIC_CPU + 0x18)

#define GIC_DIST   CONFIG_GIC_DIST
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

static inline u16
intr_irq_info(enum intr_core *c)
{
    u32 info = ICCIAR;
    *c = info >> 10;
    return info & 0x3FF;
}

static inline u16
intr_fiq_info(enum intr_core *c)
{
    return intr_irq_info(c);
}

static inline void
intr_irq_ack(enum intr_core c, u16 n)
{
    ICCEOIR = (c << 10) | (n & 0x3FF);
}

static inline void
intr_fiq_ack(enum intr_core c, u16 n)
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
gic_intr_activity(u16 n, bool enable)
{
    u8 reg = n / 32;
    u8 off = n % 32;

    if (enable)
        ICDISER(reg) = (1 << off);
    else
        ICDICER(reg) = (1 << off);
}

static inline void
gic_intr_target(u16 n, enum intr_core c)
{
    u8 reg = n / 4;
    u8 off = n % 4;

    ICDIPTR(reg) &= ~(0xFF << (off * 8));
    if (c != INTR_CORE_NONE)
        ICDIPTR(reg) |= (1 << ((off * 8) + c));
}

static inline void
gic_intr_priority(u16 n, u8 priority)
{
    u8 reg = n / 4;
    u8 off = n % 4;

    ICDIPTR(reg) &= ~(0xFF << (off * 8));
    ICDIPTR(reg) |= priority << (off * 8);
}

static inline void
gic_intr_sensitivity(u16 n, bool edge, bool high)
{
    u8 reg = n / 16;
    u8 off = n % 16;

    ICDICFR(reg) &= ~(0x3 << (off * 2));
    ICDICFR(reg) |= ((edge << 1) | high) << (off * 2);
}

static void (*irqs[256])(void) = {NULL};

static interrupt(undef)
handler_undef(void)
{
    raise(SIGILL);
}

static interrupt(swi)
handler_swi(void)
{
    raise(SIGINT);
}

static interrupt(abort)
handler_prefetch(void)
{
    raise(SIGSEGV);
}

static interrupt(abort)
handler_data(void)
{
    raise(SIGSEGV);
}

static interrupt(irq)
handler_irq(void)
{
    enum intr_core c = 0;
    u16 n = intr_irq_info(&c);

    irqs[n]();

    intr_irq_ack(c, n);
}

static interrupt(fiq)
handler_fiq(void)
{
    enum intr_core c = 0;
    u16 n = intr_fiq_info(&c);

    intr_fiq_ack(c, n);
}

static bool
init(void)
{
    ivt[IVT_UNDEF]    = handler_undef;
    ivt[IVT_SWI]      = handler_swi;
    ivt[IVT_PREFETCH] = handler_prefetch;
    ivt[IVT_DATA]     = handler_data;
    ivt[IVT_IRQ]      = handler_irq;
    ivt[IVT_FIQ]      = handler_fiq;

    return true;
}

static void
clean(void)
{
    arm_disable_irq();
    gic_disable_dist();
    gic_disable();
}

static void
gic_config(u16 n, void (*f)(void), bool enable, u8 priority)
{
    arm_disable_irq();

    gic_disable_dist();
    gic_disable();
    gic_priority(0xFF);

    irqs[n] = f;

    gic_intr_target(n, INTR_CORE_NONE);
    gic_intr_activity(n, false);
    gic_intr_priority(n, priority);
    #if defined(CONFIG_GIC_TYPE_0)
    gic_intr_sensitivity(n, false, false);
    #elif defined(CONFIG_GIC_TYPE_1)
    gic_intr_sensitivity(n, false, true);
    #elif defined(CONFIG_GIC_TYPE_2)
    gic_intr_sensitivity(n, true, false);
    #elif defined(CONFIG_GIC_TYPE_3)
    gic_intr_sensitivity(n, true, true);
    #endif
    if (enable)
    {
        gic_intr_activity(n, true);
        gic_intr_target(n, INTR_CORE0);
    }

    gic_enable();
    gic_enable_dist();

    arm_enable_irq();
}

static void
gic_wait(void)
{
    arm_wait_interrupts();
}

static const struct driver gic =
{
    .name = "ARM General Interrupt Controller",
    .init = init, .clean = clean,
    .type = DRIVER_TYPE_GIC,
    .routines.gic.config = gic_config,
    .routines.gic.wait   = gic_wait
};
driver_register(gic);
