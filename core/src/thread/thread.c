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

#include <general/types.h>
#include <general/mem.h>

#include <environ/generator.h>

#include <thread/thread.h>

struct thread_list thread_list = {0};

extern thread *
thread_new(thread_task f, void *arg, bool persistent, u8 priority)
{
    thread *ret = mem_new(sizeof(thread));

    if (ret)
    {
        ret->stepping = true;
        ret->priority = priority;
        ret->persistent = persistent;
        ret->gen = generator_new(f, arg);
        if (!(ret->gen))
            ret = mem_del(ret);
    }

    if (ret)
    {
        if (thread_list.tail)
            thread_list.tail->next = ret;
        else
        {
            thread_list.head = ret;
            thread_list.current = ret;
        }

        ret->prev = thread_list.tail;
        thread_list.tail = ret;
    }

    if (ret && !persistent)
        ret = (thread *)0x1;

    return ret;
}

extern thread *
thread_del(thread *t)
{
    if ((u32)t > 0x1)
    {
        if (t->prev)
            t->prev->next = t->next;
        if (t->next)
            t->next->prev = t->prev;

        if (thread_list.head == t)
            thread_list.head = (t->prev) ? t->prev : t->next;
        if (thread_list.current == t)
            thread_list.current = (t->next) ? t->next : thread_list.head;
        if (thread_list.tail == t)
            thread_list.tail = (t->next) ? t->next : thread_list.head;

        mem_del(t);
    }

    return NULL;
}

extern size_t
thread_sync(thread *t, size_t step)
{
    size_t ret = 0;

    if (thread_list.current->stepping)
        thread_list.current->step++;

    if (!thread_list.blocked && thread_list.current != t &&
        (u32)t > 0x1 && t->persistent)
    {
        while (t->step < step && !(t->gen->finished))
            generator_yield(thread_list.current->gen);

        ret = t->step;
    }

    return ret;
}

extern size_t
thread_wait(thread *t)
{
    size_t ret = 0;

    if (thread_list.current->stepping)
        thread_list.current->step++;

    if (!thread_list.blocked && thread_list.current != t &&
        (u32)t > 0x1 && t->persistent)
    {
        while (!(t->gen->finished))
            generator_yield(thread_list.current->gen);

        ret = t->step;
    }

    return ret;
}

extern bool
thread_rewind(thread *t)
{
    bool ret = false;

    if (!thread_list.blocked && thread_list.current != t &&
        (u32)t > 0x1 && t->persistent)
    {
        t->step = 0;
        generator_rewind(t->gen);
        ret = true;
    }

    return ret;
}

extern void *
thread_arg(void)
{
    return thread_list.current->gen->arg;
}

extern void
thread_yield(void)
{
    if (thread_list.current->stepping)
        thread_list.current->step++;
    if (!thread_list.blocked)
        generator_yield(thread_list.current->gen);
}

[[noreturn]]
extern void
thread_loop(void)
{
    if (thread_list.current->stepping)
        thread_list.current->step = 0;
    generator_rewind(thread_list.current->gen);

    thread_list.blocked = false;
    generator_yield(thread_list.current->gen);

    while (true);
}

[[noreturn]]
extern void
thread_finish(void)
{
    if (thread_list.current->stepping)
        thread_list.current->step++;
    thread_list.blocked = false;
    generator_finish(thread_list.current->gen);
}

extern void devtree_init(void);
extern void devtree_clean(void);

extern void main(void);
thread_decl (static, thread_main)
{
    main();
    thread_finish();
}

[[noreturn]]
extern void
thread_scheduler(void)
{
    _mem_init();
    devtree_init();

    thread_new(thread_main, NULL, false, 255);
    while (thread_list.current)
    {
        thread *cur = thread_list.current;

        if (!((cur->counter * cur->priority) % 255))
        {
            if (generator_next(cur->gen))
                thread_list.current = (cur->next) ? cur->next :
                                      thread_list.head;
            else
            {
                if (cur->persistent)
                    thread_list.current = (cur->next) ? cur->next :
                                          thread_list.head;
                else
                    cur = thread_del(cur);
            }
        }

        if (cur)
            cur->counter++;
    }

    devtree_clean();
    _mem_clean();

    while (true);
}
