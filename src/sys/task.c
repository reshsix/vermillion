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

#include <vermillion/sys/task.h>
#include <vermillion/util/mem.h>
#include <vermillion/hal/timer.h>

/* Register state control */

struct state
{
    uint32_t gpr[17];
} __attribute__((packed, aligned(4)));

static void
state_save_irq(struct state *st)
{
    /* Saving original registers from gic_irq_regs */
    for (uint8_t i = 0; i < 17; i++)
        st->gpr[i] = gic_irq_regs[i];
}

__attribute__((naked, noreturn))
static void
state_load_irq(struct state *st)
{
    (void)st;

    /* Call parameters: st -> r0 */

    /* Loading registers */
    __asm__ __volatile__ ("add r0, r0, #16");
    __asm__ __volatile__ ("ldmia r0!, {r4-r12}");
    /* Including the actual sp and lr from the task */
    __asm__ __volatile__ ("ldmia r0,  {sp, lr}^");
    /* Setting lr to program counter */
    __asm__ __volatile__ ("add r0, r0, #8");
    __asm__ __volatile__ ("ldr lr, [r0]");
    /* Loading cpsr into spsr */
    __asm__ __volatile__ ("add r0, r0, #4");
    __asm__ __volatile__ ("ldr r1, [r0]");
    __asm__ __volatile__ ("msr spsr, r1");
    /* Loading original r0-r3 */
    __asm__ __volatile__ ("sub r0, r0, #60");
    __asm__ __volatile__ ("ldmia r0, {r1-r3}");
    __asm__ __volatile__ ("sub r0, r0, #4");
    __asm__ __volatile__ ("ldr r0, [r0]");
    /* Flushes all the changes */
    __asm__ __volatile__ ("dsb sy");
    __asm__ __volatile__ ("isb");
    /* Jumping to address (using exception return) */
    __asm__ __volatile__ ("subs pc, lr, #4");

    /* Supressing compiler warning */
    while (true);
}

/* Context switch control */

struct context
{
    void (*f)(void *), *arg;
    uint8_t stack[CONFIG_STACK_SIZE];
};

__attribute__((naked, noreturn))
static void
context_run_irq(struct context *ctx)
{
    /* Variable to the end/base of the stack */
    static uint32_t stack = 0;
    stack = ((uint32_t)ctx->stack) + CONFIG_STACK_SIZE;

    /* Setting stack to allocated address */
    __asm__ __volatile__ ("ldmia %0, {fp}^" :: "r"(&stack));
    __asm__ __volatile__ ("ldmia %0, {sp}^" :: "r"(&stack));

    /* Setting Saved CPSR to System mode, either ARM or Thumb state */
    if ((uintptr_t)ctx->f & 1)
    {
        __asm__ __volatile__ ("mov r0, #0x3f");
        __asm__ __volatile__ ("msr spsr_c, r0");
        ctx->f = (void *)((uintptr_t)ctx->f & ~1);
    }
    else
    {
        __asm__ __volatile__ ("mov r0, #0x1f");
        __asm__ __volatile__ ("msr spsr_c, r0");
    }

    /* Flushes all the changes */
    __asm__ __volatile__ ("dsb sy");
    __asm__ __volatile__ ("isb");

    /* Jumping to function (as an exception return) */
    __asm__ __volatile__ ("mov  r0, %0\n"
                          "movs pc, %1\n"
                          :
                          : "r"(ctx->arg), "r"(ctx->f)
                          : "r0");

    /* Supressing compiler warning */
    while (true);
}

/* Task implementation */

struct vrm_task
{
    uint8_t priority;
    enum vrm_task_st status;
    bool suspended;

    struct state caller, callee;
    struct context ctx;

    struct vrm_task *prev, *next;
} task;

static struct vrm_task *heads[32] = {NULL};
static struct vrm_task *tails[32] = {NULL};
static struct vrm_task *current   =  NULL ;

static void
task_insert(struct vrm_task *t)
{
    if (t)
    {
        t->prev = tails[t->priority];
        if (t->prev)
            t->prev->next = t;
        t->next = NULL;

        if (!(heads[t->priority]))
            heads[t->priority] = t;
        tails[t->priority] = t;
    }
}

static void
task_remove(struct vrm_task *t)
{
    if (t)
    {
        if (t->priority < 32)
        {
            if (heads[t->priority] == t)
                heads[t->priority] =  t->next;
            if (tails[t->priority] == t)
                tails[t->priority] =  t->prev;
        }

        if (t->prev)
            t->prev->next = t->next;
        if (t->next)
            t->next->prev = t->prev;
    }
}

extern struct vrm_task *
vrm_task_create(void (*f)(void *), void *arg, uint8_t priority)
{
    struct vrm_task *ret = NULL;

    if (f && priority < 32)
        ret = vrm_mem_new(sizeof(struct vrm_task));

    if (ret)
    {
        vrm_mem_fill(ret, 0, sizeof(struct vrm_task));

        ret->ctx.f    = f;
        ret->ctx.arg  = arg;
        ret->priority = priority;

        task_insert(ret);
    }

    return (ret);
}

extern struct vrm_task *
vrm_task_remove(struct vrm_task *t)
{
    if (t)
    {
        task_remove(t);

        if (current && current == t)
            current = NULL;

        vrm_mem_del(t);
    }
    else
    {
        current->status = VRM_TASK_DELETED;
        vrm_task_yield();
    }

    return NULL;
}

extern bool
vrm_task_block(struct vrm_task *t)
{
    bool ret = false;

    t = (!t) ? current : t;
    if (t && (t->status == VRM_TASK_READY ||
              t->status == VRM_TASK_BLOCKED))
    {
        t->status = VRM_TASK_BLOCKED;
        ret = true;
    }

    return ret;
}

extern bool
vrm_task_unblock(struct vrm_task *t)
{
    bool ret = false;

    t = (!t) ? current : t;
    if (t && t->status == VRM_TASK_BLOCKED)
    {
        t->status = VRM_TASK_READY;
        ret = true;
    }

    return ret;
}

extern bool
vrm_task_suspend(struct vrm_task *t)
{
    bool ret = false;

    t = (!t) ? current : t;
    if (t)
    {
        t->suspended = true;
        ret          = true;
    }

    return ret;
}

extern bool
vrm_task_resume(struct vrm_task *t)
{
    bool ret = false;

    t = (!t) ? current : t;
    if (t)
    {
        t->suspended = false;
        ret          = true;
    }

    return ret;
}

extern bool
vrm_task_priority(struct vrm_task *t, uint8_t priority)
{
    bool ret = false;

    t = (!t) ? current : t;
    if (t)
    {
        task_remove(t);
        t->priority = priority;
        task_insert(t);
        ret = true;
    }

    return ret;
}

extern void
vrm_task_yield(void)
{
    gic_wait();
}

static void
task_next(void)
{
    task_remove(current);
    task_insert(current);

    bool found = false;
    for (uint8_t i = 0; !found && i < 32; i++)
    {
        uint8_t j = 32 - i - 1;

        current = heads[j];
        while (current && !found)
        {
            switch (current->status)
            {
                case VRM_TASK_NEW:
                case VRM_TASK_READY:
                    found = true;
                    break;

                case VRM_TASK_DELETED:
                    vrm_task_remove(current);
                    break;

                default:
                    break;
            }

            if (!found)
                current = current->next;
        }
    }
}

static void
task_preempt(void *arg)
{
    (void)arg;

    if (current)
        state_save_irq(&(current->caller));

    task_next();

    if (current)
    {
        switch (current->status)
        {
            case VRM_TASK_NEW:
                current->status = VRM_TASK_READY;
                context_run_irq(&(current->ctx));
                break;

            case VRM_TASK_READY:
                state_load_irq(&(current->caller));
                break;

            default:
                break;
        }
    }
}

extern void
vrm_task_scheduler(uint8_t timer, uint32_t us, uint32_t flags)
{
    (void)flags;

    vrm_timer_alarm(timer, us, true, task_preempt, NULL);
    while (true)
        gic_wait();
}
