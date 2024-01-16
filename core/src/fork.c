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

#include <core/mem.h>
#include <core/fork.h>

extern fork *
fork_new(void (*f)(void *), void *arg)
{
    fork *ret = mem_new(sizeof(fork));

    if (ret)
    {
        ret->f = f;
        ret->arg = arg;

        ret->stack = mem_new(CONFIG_STACK_SIZE);
        if (!(ret->stack))
            ret = mem_del(ret);
    }

    return ret;
}

extern fork *
fork_del(fork *fk)
{
    if (fk)
        mem_del(fk->stack);

    return mem_del(fk);
}

extern void
fork_run(fork *fk)
{
    /* Saving both current and previous fork as static for recursion */
    static fork *sfk = NULL;
    fk->previous = sfk;
    sfk = fk;

    /* Pointer to the end/base of the stack */
    static void *stack = NULL;
    stack = (void*)((u32)sfk->stack + CONFIG_STACK_SIZE);

    #if defined(CONFIG_ARCH_ARM)

    /* Saving current stack */
    asm volatile ("mov %0, fp" : "=r"(sfk->fp));
    asm volatile ("mov %0, sp" : "=r"(sfk->sp));
    /* Setting stack to allocated address */
    asm volatile ("mov fp, %0" :: "r"(stack));
    asm volatile ("mov sp, fp");

    #elif defined(CONFIG_ARCH_I686)

    /* Saving current stack */
    asm volatile ("movl %%ebp, %0" : "=r"(sfk->ebp));
    asm volatile ("movl %%esp, %0" : "=r"(sfk->esp));
    /* Setting stack to allocated address */
    asm volatile ("movl %0, %%ebp" :: "r"(stack));
    asm volatile ("movl %ebp, %esp");

    #endif

    /* Jumping to function */
    sfk->f(sfk->arg);

    #if defined(CONFIG_ARCH_ARM)

    /* Turning the stack back to normal */
    asm volatile ("mov fp, %0" :: "r"(sfk->fp));
    asm volatile ("mov sp, %0" :: "r"(sfk->sp));

    #elif defined(CONFIG_ARCH_I686)

    /* Turning the stack back to normal */
    asm volatile ("movl %0, %%ebp" :: "r"(sfk->ebp));
    asm volatile ("movl %0, %%esp" :: "r"(sfk->esp));

    #endif

    /* Loading previous fork for next run */
    fork *prev = sfk->previous;
    sfk->previous = NULL;
    sfk = prev;
}
