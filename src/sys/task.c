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

#include <vermillion/sys/task.h>
#include <vermillion/util/mem.h>

/* Register state control */

struct state
{
    void *gpr[13];
} __attribute__((packed, aligned(4)));

__attribute__((naked, return_twice))
static void *
state_save(struct state *st)
{
    (void)st;

    /* Call parameters: st -> r0 */

    /* Saving low callee-saved registers in st->gpr[0 -> 3] */
    __asm__ __volatile__ ("stmia r0!, {r4-r7}");
    /* Saving high callee-saved registers in st->gpr[4 -> 10] */
    __asm__ __volatile__ ("mov r1,  r8\n" "mov r2,  r9\n" "mov r3, r10\n" \
                          "mov r4, r11\n" "mov r5, r12\n" "mov r6, r13\n" \
                          "mov r7, r14\n");
    __asm__ __volatile__ ("stmia r0!, {r1-r7}");
    /* Restoring low registers */
    __asm__ __volatile__ ("sub r0, r0, #44");
    __asm__ __volatile__ ("ldmia r0!, {r4-r7}");
    /* Setting r1 to st->gpr[12] */
    __asm__ __volatile__ ("mov r1, r0");
    __asm__ __volatile__ ("add r1, r1, #32");
    /* Saving cpsr in st->gpr[12] */
    __asm__ __volatile__ ("mrs r2, cpsr");
    __asm__ __volatile__ ("str r2, [r1]");
    /* Setting r1 to st->gpr[11] */
    __asm__ __volatile__ ("sub r1, r1, #4");
    /* Setting r0 to NULL for first return */
    __asm__ __volatile__ ("mov r0, $0");
    /* Saving program counter in st->gpr[11] */
    __asm__ __volatile__ ("str pc, [r1]");
    /* Return with NULL or the value from state_load */
    __asm__ __volatile__ ("bx lr");
    /* Again if coming from state_load */
    __asm__ __volatile__ ("bx lr");

    /* Supressing compiler warning */
    return NULL;
}

__attribute__((naked, noreturn))
static void
state_load(struct state *st, void *ret)
{
    (void)st, (void)ret;

    /* Call parameters: st -> r0, ret -> r1 */

    /* Loading cpsr */
    __asm__ __volatile__ ("add r0, r0, #48");
    __asm__ __volatile__ ("ldr r2, [r0]");
    __asm__ __volatile__ ("msr cpsr, r2");
    /* Loading high callee-saved registers */
    __asm__ __volatile__ ("sub r0, r0, #32");
    __asm__ __volatile__ ("ldmia r0!, {r2-r7}");
    __asm__ __volatile__ ("mov r8,  r2\n" "mov r9,  r3\n" "mov r10, r4\n" \
                          "mov r11, r5\n" "mov r12, r6\n" "mov r13, r7\n");
    __asm__ __volatile__ ("ldr r2, [r0]");
    __asm__ __volatile__ ("mov r14, r2");
    /* Loading low callee-saved registers */
    __asm__ __volatile__ ("sub r0, r0, #40");
    __asm__ __volatile__ ("ldmia r0!, {r4-r7}");
    /* Setting r2 to program counter */
    __asm__ __volatile__ ("add r0, r0, #28");
    __asm__ __volatile__ ("ldr r2, [r0]");
    /* Setting r0 to ret */
    __asm__ __volatile__ ("mov r0, r1");
    /* Jumping to address */
    __asm__ __volatile__ ("mov pc, r2");

    /* Supressing compiler warning */
    while (true);
}

/* Context switch control */

struct context
{
    void (*f)(void *), *arg;

    uint8_t stack[CONFIG_STACK_SIZE];
    void *fp, *sp;

    struct context *previous;
};

static void
context_run(struct context *ctx)
{
    /* Saving both current and previous context as static for recursion */
    static struct context *s_ctx = NULL;
    ctx->previous = s_ctx;
    s_ctx = ctx;

    /* Pointer to the end/base of the stack */
    static void *stack = NULL;
    stack = (void*)((uint32_t)s_ctx->stack + CONFIG_STACK_SIZE);

    /* Saving current stack */
    __asm__ __volatile__ ("mov %0, fp" : "=r"(s_ctx->fp));
    __asm__ __volatile__ ("mov %0, sp" : "=r"(s_ctx->sp));
    /* Setting stack to allocated address */
    __asm__ __volatile__ ("mov fp, %0" :: "r"(stack));
    __asm__ __volatile__ ("mov sp, fp");

    /* Jumping to function */
    s_ctx->f(s_ctx->arg);

    /* Turning the stack back to normal */
    __asm__ __volatile__ ("mov fp, %0" :: "r"(s_ctx->fp));
    __asm__ __volatile__ ("mov sp, %0" :: "r"(s_ctx->sp));

    /* Loading previous context for next run */
    struct context *prev = s_ctx->previous;
    s_ctx->previous = NULL;
    s_ctx = prev;
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
    t->prev = tails[t->priority];
    if (t->prev)
        t->prev->next  = t;
    t->next = NULL;

    if (!(heads[t->priority]))
        heads[t->priority] = t;
    tails[t->priority] = t;
}

static void
task_remove(struct vrm_task *t)
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
    if (state_save(&(current->callee)) == NULL)
        state_load(&(current->caller), (void *)0x1);
}

extern void
vrm_task_scheduler(void)
{
    while (true)
    {
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
                        if (state_save(&(current->caller)) == NULL)
                            context_run(&(current->ctx));
                        current->status = VRM_TASK_READY;
                        found = true;
                        break;

                    case VRM_TASK_READY:
                        if (state_save(&(current->caller)) == NULL)
                            state_load(&(current->callee), (void*)0x1);
                        found = true;
                        break;

                    case VRM_TASK_DELETED:
                        vrm_task_remove(current);
                        break;

                    default:
                        break;
                }

                if (found)
                {
                    task_remove(current);
                    task_insert(current);
                }
                else
                    current = current->next;
            }
        }
    }
}
