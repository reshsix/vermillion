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

#ifndef CORE_THREAD_H
#define CORE_THREAD_H

#include <core/types.h>
#include <core/generator.h>

#define thread_task(id) \
    noreturn id(__attribute__((unused)) generator *___)
typedef struct _thread thread;
thread *thread_new(thread_task(f), void *arg, bool persistent, u8 priority);
thread *thread_del(thread *t);
size_t thread_sync(thread *t, size_t step);
size_t thread_wait(thread *t);
bool thread_rewind(thread *t);
void *thread_arg(void);
void thread_block(bool state);
void thread_yield(void);
noreturn thread_loop(void);
noreturn thread_finish(void);

#endif
