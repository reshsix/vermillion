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

#ifndef _GENERATOR_H
#define _GENERATOR_H

#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <utils.h>

/*
Usage:

generator (range, int arg, int *i)
{
    for (*i = 0; *i < arg; (*i)++)
        yield;
} finish;

extern void
test(void)
{
    int n = 0;

    instance_new (inst);
    for (int i = 0; i < 5; i++)
    {
        instance_init (inst, range, 10, &n);
        while (next(&inst))
        {
            print_uint((10 * i) + n);
            print("\r\n");
        }
    }
    instance_del (inst);
}

*/

#define __yield(x) \
{ \
    registers_save(__registers); \
    __label = &&__UNIQUE(__yield); \
    return x; \
    __UNIQUE(__yield): \
} \

#define yield __yield(1)

#define generator(id, ...) \
static int \
__attribute__((__no_inline__, __noclone__)) \
__attribute__((target("general-regs-only"))) \
__gen_##id(__VA_ARGS__) \
{ \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\""); \
    registers __registers; \
    volatile void *__label = __label; \
    if (__label != NULL) \
    { \
        registers_load(__registers); \
        goto *__label; \
    } \
    __yield((int)__registers);
#define finish \
    _Pragma("GCC diagnostic pop") \
    __label = NULL; \
    return 0; \
}

typedef struct
{
    void *stack;
    registers *regs;
    void *func;
} instance;

#define instance_new(x) \
static instance x = {0}; \
(x).stack = stack_new();

#define instance_init(x, f, ...) \
{ \
    stack_init((x).stack); \
    (x).func = __gen_##f; \
\
    volatile void *__stack = stack_get(); \
    stack_set((void*)((x).stack)); \
    (x).regs = (registers*)(__gen_##f)(__VA_ARGS__); \
    stack_set((void*)__stack); \
}

#define instance_del(x) \
{ \
    free((x).stack); \
    memset(&(x), 0, sizeof(instance)); \
}

extern bool
next(instance *i)
{
    static int ret = 0;

    if (i != NULL && i->func != NULL && i->stack != NULL)
    {
        registers regs = {0};
        registers_save(regs);
        void *stack = stack_get();

        stack_set(i->stack);
        registers_load(*(i->regs));
        ret = quick_call(i->func);

        stack_set(stack);
        registers_load(regs);
    }

    return ret;
}

#endif
