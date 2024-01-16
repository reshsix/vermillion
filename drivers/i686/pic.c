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

#include <i686/env.h>

#include <general/types.h>
#include <general/mem.h>

#include <thread/critical.h>

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/classes/pic.h>

#include <system/log.h>

struct [[gnu::packed]] descriptor
{
    u16 limit;
    u32 base;
};

static u64 gdt[] = {0x0, 0x00CF9A000000FFFF, 0x00CF92000000FFFF};
static struct descriptor gdtr = { .limit = sizeof(gdt) - 1, .base = (u32)gdt};
static u64 idt[256] = {0};
static struct descriptor idtr = { .limit = sizeof(idt) - 1, .base = (u32)idt};

static void
load_gdt(void)
{
    asm volatile ("lgdt %0" :: "m"(gdtr));

    asm volatile ("jmp $0x08, $cs_reload");
    asm volatile ("cs_reload:");
    asm volatile ("movw $0x10, %ax");
    asm volatile ("mov %ax, %ds");
    asm volatile ("mov %ax, %es");
    asm volatile ("mov %ax, %fs");
    asm volatile ("mov %ax, %gs");
    asm volatile ("mov %ax, %ss");
}

static void
load_idt(void)
{
    asm volatile ("lidt %0" :: "m"(idtr));
}

static void
idt_entry(u8 id, void *offset, bool trap)
{
    /* Segment select 1 | Type: 32-bit trap/interrupt | Present bit */
    u64 result = ((1 << 3) << 16) | (((trap) ? 0xFULL : 0xEULL) << 40)
                 | (1ULL << 47);

    /* Offset lower, offset higher */
    u16 low  = (u32)offset & 0xFFFF;
    result |= low;
    u16 high = (u32)offset >> 16;
    result |= (u64)high << 48;

    idt[id] = result;
}

#define PIC1_CMD  0x20
#define PIC2_CMD  0xA0
#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

static void
pic_remap(u8 master, u8 slave)
{
    u8 m1 = in8(PIC1_DATA);
    u8 m2 = in8(PIC2_DATA);

    /* ICW1 Starts initialization */
    out8(PIC1_CMD, 0x11);
    io_wait();
    out8(PIC2_CMD, 0x11);
    io_wait();

    /* ICW2: Send offsets */
    out8(PIC1_DATA, master);
    io_wait();
    out8(PIC2_DATA, slave);
    io_wait();

    /* ICW3: Cascade mode */
    out8(PIC1_DATA, 0x4);
    io_wait();
    out8(PIC2_DATA, 0x2);
    io_wait();

    /* ICW4: 8086 mode */
    out8(PIC1_DATA, 0x1);
    io_wait();
    out8(PIC2_DATA, 0x1);
    io_wait();

    /* Restore masks */
    out8(PIC1_DATA, m1);
    io_wait();
    out8(PIC2_DATA, m2);
    io_wait();
}

static void
pic_mask(u8 id, bool state)
{
    u16 port = 0;

    if (id < 8)
        port = PIC1_DATA;
    else if (id < 16)
    {
        port = PIC2_DATA;
        id -= 8;
    }

    if (port)
    {
        if (state)
            out8(port, in8(port) |  (1 << id));
        else
            out8(port, in8(port) & ~(1 << id));
    }
}

[[maybe_unused]]
static u16
pic_irr(void)
{
    /* OCW3: Get irr value */
    out8(PIC1_CMD, 0x0a);
    out8(PIC2_CMD, 0x0a);

    return (in8(PIC2_CMD) << 8) | in8(PIC1_CMD);
}

static u16
pic_isr(void)
{
    /* OCW3: Get isr value */
    out8(PIC1_CMD, 0x0b);
    out8(PIC2_CMD, 0x0b);

    return (in8(PIC2_CMD) << 8) | in8(PIC1_CMD);
}

static void
pic_eoi(u8 irq)
{
    if (irq < 8)
        out8(PIC1_CMD, 0x20);
    else if (irq < 16)
    {
        out8(PIC1_CMD, 0x20);
        out8(PIC2_CMD, 0x20);
    }
}

struct interrupt_frame
{
    int ip;
    int cs;
    int flags;
    int sp;
    int ss;
};

struct handler
{
    void (*f)(void *, struct interrupt_frame *), *arg;
};

struct handler_err
{
    void (*f)(void *, struct interrupt_frame *, int), *arg;
};

struct handler_irq
{
    void (*f)(void *), *arg;
};

struct pic
{
    bool enabled;
    struct pic_irq irqs[16];
    struct pic_swi swis[256];

    u32 swi_id;
    void *swi_data;

    u8 m1, m2;
    struct handler nmi;
    struct handler debug;
    struct handler traps[2];
    struct handler faults[8];
    struct handler_err faults_err[9];
};
struct pic *pic = NULL;

static void
pic_enable(void)
{
    out8(PIC1_DATA, pic->m1);
    out8(PIC2_DATA, pic->m2);

    asm volatile ("sti");
}

static void
pic_disable(void)
{
    pic->m1 = in8(PIC1_DATA);
    pic->m2 = in8(PIC2_DATA);

    out8(PIC1_DATA, 0xFF);
    out8(PIC2_DATA, 0xFF);

    asm volatile ("cli");
}

#define interrupt \
    [[gnu::target("general-regs-only")]] [[gnu::interrupt]] \
    static void

#define TRAP(id, name) \
    interrupt name(struct interrupt_frame *frame) \
    { \
        log ("trap\r\n"); \
        if (pic->traps[id].f) \
            pic->traps[id].f(pic->traps[id].arg, frame); \
    }

#define FAULT(id, name) \
    interrupt name(struct interrupt_frame *frame) \
    { \
        log ("fault\r\n"); \
        if (pic->faults[id].f) \
            pic->faults[id].f(pic->faults[id].arg, frame); \
    }

#define FAULT_ERR(id, name) \
    interrupt name(struct interrupt_frame *frame, int code) \
    { \
        log (#name); \
        log (code); \
        log ("\r\n"); \
        if (pic->faults_err[id].f) \
            pic->faults_err[id].f(pic->faults_err[id].arg, frame, code); \
    }

#define NMI(name) \
    interrupt name(struct interrupt_frame *frame) \
    { \
        log ("nmi\r\n"); \
        if (pic->nmi.f) \
            pic->nmi.f(pic->nmi.arg, frame); \
    }

#define DEBUG(name) \
    interrupt name(struct interrupt_frame *frame) \
    { \
        log ("debug\r\n"); \
        if (pic->debug.f) \
            pic->debug.f(pic->debug.arg, frame); \
    }

#define IRQ(id, name) \
    interrupt name(struct interrupt_frame *frame) \
    { \
        (void)frame; \
\
        critical \
        { \
            bool spurious = false; \
            if (id == 7) \
            { \
                u16 isr = pic_isr(); \
                spurious = !(isr & (1 << 7)); \
            } \
            else if (id == 15) \
            { \
                u16 isr = pic_isr(); \
                spurious = !(isr & (1 << 15)); \
                if (spurious) \
                    out8(PIC1_CMD, 0x20); \
            } \
\
            if (!spurious) \
            { \
                if (pic->irqs[id].enabled && pic->irqs[id].handler) \
                    pic->irqs[id].handler(pic->irqs[id].arg); \
                pic_eoi(id); \
            } \
        } \
    }

#define SWI(name) \
    interrupt name(struct interrupt_frame *frame) \
    { \
        (void)frame; \
        critical if (pic->swis[pic->swi_id].enabled && \
                     pic->swis[pic->swi_id].handler) \
            pic->swis[pic->swi_id].handler(pic->swis[pic->swi_id].arg, \
                                           pic->swi_data); \
    }

#define ABORT(name) \
    interrupt name(struct interrupt_frame *frame) \
    { \
        (void)frame; \
        log ("Aborted: "); \
        log (#name); \
        log ("\r\n"); \
        asm volatile ("cli"); \
        asm volatile ("hlt"); \
    }

TRAP(0, breakpoint)
TRAP(1, overflow)

FAULT(0, division_error)
FAULT(1, bound_range_exceeded)
FAULT(2, invalid_opcode)
FAULT(3, device_not_avaliable)
FAULT(4, x87_fp)
FAULT(5, simd_fp)
FAULT(6, virtualization)
FAULT(7, hypervisor_injection)

FAULT_ERR(0, invalid_tss)
FAULT_ERR(1, segment_not_present)
FAULT_ERR(2, stack_segment_fault)
FAULT_ERR(3, general_protection_fault)
FAULT_ERR(4, page_fault)
FAULT_ERR(5, alignment_check)
FAULT_ERR(6, control_protection)
FAULT_ERR(7, vmm_communication)
FAULT_ERR(8, security)

NMI(non_maskable_interrupt)
DEBUG(debug_exception)

IRQ(0,  irq0)
IRQ(1,  irq1)
IRQ(2,  irq2)
IRQ(3,  irq3)
IRQ(4,  irq4)
IRQ(5,  irq5)
IRQ(6,  irq6)
IRQ(7,  irq7)
IRQ(8,  irq8)
IRQ(9,  irq9)
IRQ(10, irq10)
IRQ(11, irq11)
IRQ(12, irq12)
IRQ(13, irq13)
IRQ(14, irq14)
IRQ(15, irq15)

SWI(swi_handler)

ABORT(double_fault)
ABORT(machine_check)

static void
initialize(void)
{
    load_gdt();
    load_idt();

    idt_entry(0x00, division_error,           true);
    idt_entry(0x01, debug_exception,          true);
    idt_entry(0x02, non_maskable_interrupt,   true);
    idt_entry(0x03, breakpoint,               true);
    idt_entry(0x04, overflow,                 true);
    idt_entry(0x05, bound_range_exceeded,     true);
    idt_entry(0x06, invalid_opcode,           true);
    idt_entry(0x07, device_not_avaliable,     true);
    idt_entry(0x08, double_fault,             true);

    idt_entry(0x0A, invalid_tss,              true);
    idt_entry(0x0B, segment_not_present,      true);
    idt_entry(0x0C, stack_segment_fault,      true);
    idt_entry(0x0D, general_protection_fault, true);
    idt_entry(0x0E, page_fault,               true);

    idt_entry(0x10, x87_fp,                   true);
    idt_entry(0x11, alignment_check,          true);
    idt_entry(0x12, machine_check,            true);
    idt_entry(0x13, simd_fp,                  true);
    idt_entry(0x14, virtualization,           true);
    idt_entry(0x15, control_protection,       true);

    idt_entry(0x1C, hypervisor_injection,     true);
    idt_entry(0x1D, vmm_communication,        true);
    idt_entry(0x1E, security,                 true);

    pic_remap(0x20, 0x28);
    idt_entry(0x20, irq0,  false);
    idt_entry(0x21, irq1,  false);
    idt_entry(0x22, irq2,  false);
    idt_entry(0x23, irq3,  false);
    idt_entry(0x24, irq4,  false);
    idt_entry(0x25, irq5,  false);
    idt_entry(0x26, irq6,  false);
    idt_entry(0x27, irq7,  false);
    idt_entry(0x28, irq8,  false);
    idt_entry(0x29, irq9,  false);
    idt_entry(0x2A, irq10, false);
    idt_entry(0x2B, irq11, false);
    idt_entry(0x2C, irq12, false);
    idt_entry(0x2D, irq13, false);
    idt_entry(0x2E, irq14, false);
    idt_entry(0x2F, irq15, false);

    idt_entry(0x80, swi_handler, true);
}

static void
init(void **ctx)
{
    if (!pic)
    {
        pic = mem_new(sizeof(struct pic));
        if (pic)
        {
            initialize();
            *ctx = pic;
        }
    }
}

static void
clean(void *ctx)
{
    if (ctx == pic)
        pic = mem_del(pic);
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *length)
{
    bool ret = true;

    if (ctx == pic)
    {
        switch (idx)
        {
            case 0:
                *width = sizeof(bool);
                *length = 1;
                break;

            case 1:
                *width = sizeof(struct pic_irq);
                *length = 16;
                break;

            case 2:
                *width = sizeof(struct pic_swi);
                *length = 0;
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

    if (ctx == pic)
    {
         switch (idx)
        {
            case 0:
                ret = (block == 0);

                if (ret)
                    mem_copy(buffer, &(pic->enabled), sizeof(bool));
                break;

            case 1:
                ret = (block < 16);

                if (ret)
                    mem_copy(buffer, &(pic->irqs[block]),
                             sizeof(struct pic_irq));
                break;

            case 2:
                ret = (block < 256);

                if (ret)
                    mem_copy(buffer, &(pic->swis[block]),
                             sizeof(struct pic_swi));
                break;

            case 3:
                ret = (block == 0);

                if (ret)
                    asm volatile ("hlt");
                break;
        }
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    if (ctx == pic)
    {
        switch (idx)
        {
            case 0:
                ret = (block == 0);

                if (ret)
                {
                    mem_copy(&(pic->enabled), buffer, sizeof(bool));
                    if (pic->enabled)
                        pic_enable();
                    else
                        pic_disable();
                }
                break;

            case 1:
                ret = (block < 16);

                if (ret)
                {
                    struct pic_irq irq = {0};
                    mem_copy(&irq, buffer, sizeof(struct pic_irq));

                    if (irq.level == PIC_EDGE_H)
                    {
                        mem_copy(&(pic->irqs[block]), &irq,
                                 sizeof(struct pic_irq));

                        if (pic->enabled)
                            pic_mask(block, false);
                        else
                        {
                            if (block < 8)
                                pic->m1 &= ~(1 << block);
                            else
                                pic->m2 &= ~(1 << (block - 8));
                        }
                    }
                    else
                        ret = false;
                }
                break;

            case 2:
                ret = (block < 256);

                if (ret)
                    mem_copy(&(pic->swis[block]), buffer,
                             sizeof(struct pic_swi));
                break;

            case 3:
                ret = (block == 0);

                if (ret)
                    asm volatile ("hlt");
                break;

            case 4:
                ret = (block < 256);

                if (ret)
                {
                    pic->swi_id = block;
                    mem_copy(&(pic->swi_data), buffer, sizeof(void *));
                    asm volatile ("int $0x80");
                }
                break;
        }
    }

    return ret;
}

drv_decl (pic, i686_pic)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
