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
#include <types.h>
#include <utils.h>

/*
Usage:

generator (range, int arg)
{
    for (int i = 0; i < arg; i++)
        yield(i);
} finish;

extern void
test(void)
{
    bool finished = false;

    int n = 0;
    while (1)
    {
        // Treat next as a statement
        next (range, n, finished, 10);
        if (finished)
            break;

        print_uint(n);
        print("\r\n");
    }
}
*/

typedef struct
{
    int data;
    bool finished;
} generator_t;

#define generator(id, ...) \
static volatile void *__stack_##id = NULL; \
static generator_t \
__attribute__((__no_inline__, __noclone__)) \
__attribute__((target("general-regs-only"))) \
id(__VA_ARGS__) \
{ \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\""); \
    _Pragma("GCC diagnostic ignored \"-Wdangling-pointer\""); \
    static registers __registers; \
    static void *__label = NULL; \
    if (__label != NULL) \
        goto *__label;
#define finish \
    _Pragma("GCC diagnostic pop") \
    __label = NULL; \
    return (generator_t){.finished = true}; \
}

#define yield(x) \
{ \
    registers_save(__registers); \
    __label = &&__yield##__LINE__; \
    return (generator_t){.data = x, .finished = false}; \
    __yield##__LINE__: \
    registers_load(__registers); \
} \


#define next(f, x, flag, ...) \
{ \
    static volatile generator_t __result##__LINE__ = {0}; \
    if (__stack_##f == NULL) \
        __stack_##f = (void*)(((u32)calloc(0x1000, 1)) + 0x1000); \
\
    volatile void *__stack = stack_get(); \
    stack_set((void*)__stack_##f); \
    __result##__LINE__ = f(__VA_ARGS__); \
    stack_set((void*)__stack); \
\
    if (!(__result##__LINE__.finished)) \
        x = __result##__LINE__.data; \
    else \
    { \
        free((void*)((u32)__stack_##f - 0x1000)); \
        __stack_##f = NULL; \
    } \
\
    flag = __result##__LINE__.finished; \
}

#endif
