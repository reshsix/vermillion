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

    /* Clears IRQ stack without using r0 */
    __asm__ __volatile__ ("mov sp, %0"
                          :
                          : "r"(gic_irq_stack)
                          : "memory", "r0");
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

    /* Clear IRQ stack */
    __asm__ __volatile__ ("mov sp, %0"
                          :
                          : "r"(gic_irq_stack)
                          : "memory");

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

/* Tick counter */

static uint64_t ticks = 0;

extern uint64_t
vrm_task_ticks(void)
{
    return ticks;
}

/* Critical sections */

static uint32_t critical = 0;

static bool
outside_irq(void)
{
    uint32_t cpsr;
    __asm__ __volatile__ ("mrs %0, cpsr" : "=r"(cpsr));
    return (cpsr & 0xF) != 0x2;
}

extern void
vrm_task_crit_in(void)
{
    if (ticks && outside_irq())
    {
        critical++;
        gic_state(false);
    }
}

extern void
vrm_task_crit_out(void)
{
    if (ticks && outside_irq())
    {
        if (critical > 0)
            critical--;

        if (critical == 0)
            gic_state(true);
    }
}

/* Task implementation */

struct vrm_task
{
    uint8_t priority;
    enum
    {
        VRM_TASK_NEW,
        VRM_TASK_READY,
        VRM_TASK_DELETED
    } status;
    uint32_t delay;

    uint32_t mutexes;
    uint8_t priority0;

    struct state state;
    struct context ctx;

    struct vrm_task *prev, *next;
    struct vrm_task_list *list;
} task;

static struct vrm_task_list active[32] = {NULL};
static struct vrm_task_list sleeping   = {NULL};
static struct vrm_task    *current     =  NULL;

static void
task_insert(struct vrm_task *t, struct vrm_task_list *list)
{
    if (t && !(t->list))
    {
        t->next = NULL;
        t->prev = list->tail;
        if (t->prev)
            t->prev->next = t;

        if (!(list->head))
            list->head = t;
        list->tail = t;

        t->list = list;
    }
}

static void
task_remove(struct vrm_task *t, struct vrm_task_list *list)
{
    if (t && (t->list == list))
    {
        if (list->head == t)
            list->head =  t->next;
        if (list->tail == t)
            list->tail =  t->prev;

        if (t->prev)
            t->prev->next = t->next;
        if (t->next)
            t->next->prev = t->prev;

        t->prev = NULL;
        t->next = NULL;
        t->list = NULL;
    }
}

extern struct vrm_task *
vrm_task_create(void (*f)(void *), void *arg, uint8_t priority)
{
    struct vrm_task *ret = NULL;

    VRM_TASK_CRITICAL
    {
        if (f && priority < 32)
            ret = vrm_mem_new(sizeof(struct vrm_task));

        if (ret)
        {
            vrm_mem_fill(ret, 0, sizeof(struct vrm_task));

            ret->ctx.f    = f;
            ret->ctx.arg  = arg;
            ret->priority = priority;

            task_insert(ret, &(active[ret->priority]));
        }
    }

    return (ret);
}

extern struct vrm_task *
vrm_task_remove(struct vrm_task *t)
{
    VRM_TASK_CRITICAL
    {
        if (t)
        {
            task_remove(t, &(active[t->priority]));
            if (current && current == t)
                current = NULL;

            vrm_mem_del(t);
        }
        else if (current)
        {
            current->status = VRM_TASK_DELETED;

            VRM_TASK_NONCRITICAL
                vrm_task_yield();
        }
    }

    return NULL;
}

extern void
vrm_task_block(struct vrm_task *t, struct vrm_task_list *list)
{
    bool yield = false;

    VRM_TASK_CRITICAL
    {
        t = (!t) ? current : t;
        if (t)
        {
            task_remove(t, &(active[t->priority]));
            if (list)
                task_insert(t, list);

            if (current && current == t)
                yield = true;
        }
    }

    if (yield)
        vrm_task_yield();
}

extern void
vrm_task_unblock(struct vrm_task *t, struct vrm_task_list *list)
{
    VRM_TASK_CRITICAL
    {
        t = (!t) ? current : t;
        if (t)
        {
            if (list)
                task_remove(t, list);

            task_insert(t, &(active[t->priority]));
        }
    }
}

extern void
vrm_task_priority(struct vrm_task *t, uint8_t priority)
{
    VRM_TASK_CRITICAL
    {
        t = (!t) ? current : t;

        if (t)
        {
            if (t->mutexes)
                t->priority0 = priority;
            else
            {
                vrm_task_block(t, NULL);
                t->priority = priority;
                vrm_task_unblock(t, NULL);
            }
        }
    }
}

extern void
vrm_task_yield(void)
{
    if (ticks && !critical)
        gic_wait();
}

static void
task_next(void)
{
    if (current)
    {
        task_remove(current, &(active[current->priority]));
        task_insert(current, &(active[current->priority]));
    }

    bool found = false;
    for (uint8_t i = 0; !found && i < 32; i++)
    {
        uint8_t j = 32 - i - 1;

        current = active[j].head;
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
    ticks++;
    (void)arg;

    /* Saving state */
    if (current)
        state_save_irq(&(current->state));

    /* Waking up delayed tasks */
    for (struct vrm_task *t = sleeping.head; t; t = t->next)
    {
        if (t->delay)
            t->delay--;

        if (t->delay == 0)
            vrm_task_unblock(t, &sleeping);
    }

    /* Choosing next task */
    task_next();

    /* Jumping to the choosen task */
    if (current)
    {
        switch (current->status)
        {
            case VRM_TASK_NEW:
                current->status = VRM_TASK_READY;
                gic_irq_ack();
                context_run_irq(&(current->ctx));
                break;

            case VRM_TASK_READY:
                gic_irq_ack();
                state_load_irq(&(current->state));
                break;

            default:
                break;
        }
    }
}

static void
task_idle(void *arg)
{
    (void)arg;

    while (true)
        vrm_task_yield();
}

extern void
vrm_task_scheduler(uint8_t timer, uint32_t us, uint32_t flags)
{
    (void)flags;
    vrm_task_create(task_idle, NULL, 0);

    vrm_timer_alarm(timer, us, true, task_preempt, NULL);
    while (true)
        gic_wait();
}

/* Synchronization primitives */

extern void
vrm_task_delay(uint32_t ticks)
{
    if (ticks && current)
    {
        current->delay = ticks;
        vrm_task_block(current, &sleeping);
    }
}

extern bool
vrm_task_sem_take(struct vrm_task_sem *s, uint32_t timeout)
{
    bool ret = true;

    if (ticks)
    {
        VRM_TASK_CRITICAL
        {
            for (; s->count == 0 && timeout; timeout--)
            {
                VRM_TASK_NONCRITICAL
                    vrm_task_delay(1);
            }

            if (s->count != 0)
                s->count--;
            else
                ret = false;
        }
    }

    return ret;
}

extern void
vrm_task_sem_give(struct vrm_task_sem *s)
{
    if (ticks)
    {
        VRM_TASK_CRITICAL
        {
            s->count++;
        }
    }
}

extern bool
vrm_task_mut_lock(struct vrm_task_mut *m, uint32_t timeout)
{
    bool ret = true;

    if (ticks && current)
    {
        VRM_TASK_CRITICAL
        {
            if (m->owner && m->owner->priority < current->priority)
            {
                vrm_task_block(m->owner, NULL);
                m->owner->priority = current->priority;
                vrm_task_unblock(m->owner, NULL);
            }

            for (; m->owner != NULL && timeout; timeout--)
            {
                VRM_TASK_NONCRITICAL
                    vrm_task_delay(1);
            }

            if (m->owner == NULL)
            {
                m->owner = current;

                if (!m->owner->mutexes)
                    m->owner->priority0 = m->owner->priority;
                m->owner->mutexes++;
            }
            else
                ret = false;
        }
    }

    return ret;
}

extern void
vrm_task_mut_unlock(struct vrm_task_mut *m)
{
    if (ticks && current)
    {
        VRM_TASK_CRITICAL
        {
            if (m->owner && current == m->owner)
            {
                m->owner->mutexes--;
                if (!m->owner->mutexes)
                {
                    vrm_task_block(m->owner, NULL);
                    m->owner->priority = m->owner->priority0;
                    vrm_task_unblock(m->owner, NULL);
                }

                m->owner = NULL;
            }
        }
    }
}
