/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <arch/gic.h>

#include <vermillion/util/mem.h>
#include <vermillion/util/debug.h>
#include <vermillion/util/types.h>

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

#define INTERRUPT(type) \
    __attribute__((target("general-regs-only"))) \
    __attribute__((isr(#type))) \
    static void

#define ICCICR(cpu)  *(volatile uint32_t*)(cpu + 0x0)
#define ICCPMR(cpu)  *(volatile uint32_t*)(cpu + 0x4)
#define ICCIAR(cpu)  *(volatile uint32_t*)(cpu + 0xC)
#define ICCEOIR(cpu) *(volatile uint32_t*)(cpu + 0x10)
#define ICCHPIR(cpu) *(volatile uint32_t*)(cpu + 0x18)

#define ICDDCR(dist)     *(volatile uint32_t*)(dist + 0x0)
#define ICDISER(dist, n) *(volatile uint32_t*)(dist + 0x100 + (n * 4))
#define ICDICER(dist, n) *(volatile uint32_t*)(dist + 0x180 + (n * 4))
#define ICDIPR(dist, n)  *(volatile uint32_t*)(dist + 0x400 + (n * 4))
#define ICDIPTR(dist, n) *(volatile uint32_t*)(dist + 0x800 + (n * 4))
#define ICDICFR(dist, n) *(volatile uint32_t*)(dist + 0xC00 + (n * 4))

/* Driver definition */

enum intr_core
{
    INTR_CORE0 = 0,
    INTR_CORE1,
    INTR_CORE2,
    INTR_CORE3,

    INTR_CORE_NONE = 255
};

static inline uint16_t
intr_info(uint32_t cpu, enum intr_core *c)
{
    uint32_t info = ICCIAR(cpu);
    *c = info >> 10;
    return info & 0x3FF;
}

static inline void
intr_ack(uint32_t cpu, enum intr_core c, uint16_t n)
{
    ICCEOIR(cpu) = (c << 10) | (n & 0x3FF);
}

static inline void
arm_disable_irq(void)
{
    uint32_t cpsr = 0;
    __asm__ __volatile__ ("mrs %0, cpsr\n" : "=r" (cpsr));
    cpsr |= 0x80;
    __asm__ __volatile__ ("msr cpsr, %0\n" : : "r" (cpsr));
}

static inline void
arm_enable_irq(void)
{
    uint32_t cpsr = 0;
    __asm__ __volatile__ ("mrs %0, cpsr\n" : "=r" (cpsr));
    cpsr &= 0xFFFFFF7F;
    __asm__ __volatile__ ("msr cpsr, %0\n" : : "r" (cpsr));
}

static inline void
arm_wait_interrupts(void)
{
    __asm__ __volatile__ ("wfi");
}

static inline void
gic_enable(uint32_t cpu)
{
    ICCICR(cpu) = 0x3;
}

static inline void
gic_disable(uint32_t cpu)
{
    ICCICR(cpu) = 0;
}

static inline void
gic_priority(uint32_t cpu, uint32_t value)
{
    ICCPMR(cpu) = value;
}

static inline void
gic_enable_dist(uint32_t dist)
{
    ICDDCR(dist) = 0x3;
}

static inline void
gic_disable_dist(uint32_t dist)
{
    ICDDCR(dist) = 0;
}

static inline void
gic_intr_activity(uint32_t dist, uint16_t n, bool enable)
{
    uint8_t reg = n / 32;
    uint8_t off = n % 32;

    if (enable)
        ICDISER(dist, reg) = (1 << off);
    else
        ICDICER(dist, reg) = (1 << off);
}

static inline void
gic_intr_target(uint32_t dist, uint16_t n, enum intr_core c)
{
    uint8_t reg = n / 4;
    uint8_t off = n % 4;

    ICDIPTR(dist, reg) &= ~(0xFF << (off * 8));
    if (c != INTR_CORE_NONE)
        ICDIPTR(dist, reg) |= (1 << ((off * 8) + c));
}

static inline void
gic_intr_priority(uint32_t dist, uint16_t n, uint8_t priority)
{
    uint8_t reg = n / 4;
    uint8_t off = n % 4;

    ICDIPTR(dist, reg) &= ~(0xFF << (off * 8));
    ICDIPTR(dist, reg) |= priority << (off * 8);
}

static inline void
gic_intr_sensitivity(uint32_t dist, uint16_t n, bool edge, bool high)
{
    uint8_t reg = n / 16;
    uint8_t off = n % 16;

    ICDICFR(dist, reg) &= ~(0x3 << (off * 2));
    ICDICFR(dist, reg) |= ((edge << 1) | high) << (off * 2);
}

struct gic
{
    bool enabled;
    uint32_t cpu, dist;

    void (*handler[256])(void *), *arg[256];
    uint8_t stack[CONFIG_STACK_SIZE];
};

static struct gic gic = {0};

INTERRUPT(undef) handler_undef(void)
{
    vrm_debug_string("Undefined Instruction\r\n");
    for (;;)
        arm_wait_interrupts();
}

INTERRUPT(swi) handler_swi(void)
{
    vrm_debug_string("Unhandled supervisor call\r\n");
    for (;;)
        arm_wait_interrupts();
}

INTERRUPT(abort) handler_prefetch(void)
{
    vrm_debug_string("Prefetch Abort\r\n");
    for (;;)
        arm_wait_interrupts();
}

INTERRUPT(abort) handler_data(void)
{
    vrm_debug_string("Data Abort\r\n");
    for (;;)
        arm_wait_interrupts();
}

INTERRUPT(irq) handler_irq(void)
{
    enum intr_core c = 0;
    uint16_t n = intr_info(gic.cpu, &c);
    intr_ack(gic.cpu, c, n);

    if (gic.handler[n])
        gic.handler[n](gic.arg[n]);
}

INTERRUPT(fiq) handler_fiq(void)
{
    vrm_debug_string("Unexpected FIQ\r\n");
    for (;;)
        arm_wait_interrupts();
}

/* External functions */

extern void
gic_init(uint32_t cpu, uint32_t dist)
{
    gic.cpu  = cpu;
    gic.dist = dist;

    __ivt[IVT_UNDEF]    = handler_undef;
    __ivt[IVT_SWI]      = handler_swi;
    __ivt[IVT_PREFETCH] = handler_prefetch;
    __ivt[IVT_DATA]     = handler_data;
    __ivt[IVT_IRQ]      = handler_irq;
    __ivt[IVT_FIQ]      = handler_fiq;

    void *addr = &(gic.stack[CONFIG_STACK_SIZE]);
    __asm__ __volatile__ ("msr CPSR_c, #0b11010010\n"
                          "mov sp, %0\n"
                          "msr CPSR_c, #0b11010011\n"
                          :
                          : "r"(addr)
                          : "memory");

    gic_priority(gic.cpu, 0xFF);
}

extern void
gic_clean(void)
{
    arm_disable_irq();
    gic_disable_dist(gic.dist);
    gic_disable(gic.cpu);

    __ivt[IVT_UNDEF]    = NULL;
    __ivt[IVT_SWI]      = NULL;
    __ivt[IVT_PREFETCH] = NULL;
    __ivt[IVT_DATA]     = NULL;
    __ivt[IVT_IRQ]      = NULL;
    __ivt[IVT_FIQ]      = NULL;
}

extern void
gic_state(bool enabled)
{
    gic.enabled = enabled;

    if (gic.enabled)
    {
        gic_enable(gic.cpu);
        gic_enable_dist(gic.dist);
        arm_enable_irq();
    }
    else
    {
        arm_disable_irq();
        gic_disable_dist(gic.dist);
        gic_disable(gic.cpu);
    }
}

extern void
gic_config(uint8_t n, void (*handler)(void *), void *arg, bool edge, bool high)
{
    if (gic.enabled)
    {
        arm_disable_irq();
        gic_disable_dist(gic.dist);
        gic_disable(gic.cpu);
    }

    gic_intr_target(gic.dist, n, INTR_CORE_NONE);
    gic_intr_activity(gic.dist, n, false);
    gic_intr_priority(gic.dist, n, 0);
    gic_intr_sensitivity(gic.dist, n, edge, high);

    gic.handler[n] = handler;
    gic.arg    [n] = arg;

    if (handler)
    {
        gic_intr_activity(gic.dist, n, true);
        gic_intr_target(gic.dist, n, gic.cpu);
    }

    if (gic.enabled)
    {
        gic_enable(gic.cpu);
        gic_enable_dist(gic.dist);
        arm_enable_irq();
    }
}

extern void
gic_wait(void)
{
    arm_wait_interrupts();
}
