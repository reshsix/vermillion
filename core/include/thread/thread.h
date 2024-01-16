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

#pragma once

#include <general/types.h>

#include <environ/generator.h>

#define thread_decl(storage, id) \
    [[noreturn]] \
    storage void \
    id([[maybe_unused]] generator *___)
#define thread_incl(id) \
    [[noreturn]] void id(generator *___);

typedef void (*thread_task)(generator *);

typedef struct thread
{
    generator *gen;

    size_t step;
    bool persistent, stepping;
    u8 counter, priority;

    struct thread *prev, *next;
} thread;

thread *thread_new(thread_task f, void *arg, bool persistent, u8 priority);
thread *thread_del(thread *t);
size_t thread_sync(thread *t, size_t step);
size_t thread_wait(thread *t);
bool thread_rewind(thread *t);
void *thread_arg(void);
void thread_yield(void);
[[noreturn]] void thread_loop(void);
[[noreturn]] void thread_finish(void);

struct thread_list
{
    thread *head, *current, *tail;
    bool blocked;
};
extern struct thread_list thread_list;

[[noreturn]] void thread_scheduler(void);
