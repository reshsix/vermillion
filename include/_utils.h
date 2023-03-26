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

#ifndef _UTILS_H
#define _UTILS_H

#include <_types.h>
#include <stdlib.h>
#include <string.h>

#define ___CONCAT(x, y) x ## y
#define __CONCAT(x, y) ___CONCAT(x, y)
#define __UNIQUE(x) __CONCAT(x, __LINE__)

void halt(void);

void print(const char *s);
void print_hex(const u32 n);
void print_uint(const u32 n);

#define STACK_SIZE 0x1000

static inline void *
stack_new(void)
{
    return (void*)((u32)malloc(STACK_SIZE) + STACK_SIZE);
}

static inline void
stack_init(void *mem)
{
    memset((void*)((u32)mem - STACK_SIZE), 0, STACK_SIZE);
}

static inline void
stack_free(void *mem)
{
    free((void*)((u32)mem - STACK_SIZE));
}

static inline void * __attribute__((always_inline))
stack_get(void)
{
    void *ret = NULL;
    asm ("mov %[ret], sp" : [ret] "=r" (ret));
    return ret;
}

static inline void __attribute__((always_inline))
stack_set(void *mem)
{
    asm ("mov sp, %[mem]" :: [mem] "r" (mem));
}

typedef u32 registers[11];
static inline void __attribute__((always_inline))
registers_save(registers mem)
{
    register u32 *dest asm ("ip") = mem;
    asm ("stmia %[dest], {r0-r10}" :: [dest] "r" (dest));
}

static inline void __attribute__((always_inline))
registers_load(registers mem)
{
    register void *src asm ("ip") = mem;
    asm ("ldmia %[src], {r0-r10}" :: [src] "r" (src));
}

static inline int __attribute__((always_inline))
quick_call(void *mem)
{
    static u32 lr_s = 0;
    register u32 lr asm ("lr");
    lr_s = lr;

    int ret = 0;
    asm ("mov ip, %[mem]" :: [mem] "r" (mem));
    asm volatile ("blx ip");
    asm ("mov %[ret], r0" : [ret] "=r" (ret));

    lr = lr_s;
    return ret;
}

#endif
