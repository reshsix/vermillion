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
#include <core/thread.h>
#include <core/generator.h>

struct _threads _threads = {0};

extern thread *
thread_new(thread_task(f), void *arg, bool persistent, u8 priority)
{
    thread *ret = mem_new(sizeof(thread));

    if (ret)
    {
        ret->priority = priority;
        ret->persistent = persistent;
        ret->gen = generator_new(f, arg);
        if (!(ret->gen))
            ret = mem_del(ret);
    }

    if (ret)
    {
        if (_threads.tail)
            _threads.tail->next = ret;
        else
        {
            _threads.head = ret;
            _threads.cur = ret;
        }

        ret->prev = _threads.tail;
        _threads.tail = ret;
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

        if (_threads.head == t)
            _threads.head = (t->prev) ? t->prev : t->next;
        if (_threads.cur == t)
            _threads.cur = (t->next) ? t->next : _threads.head;
        if (_threads.tail == t)
            _threads.tail = (t->next) ? t->next : _threads.head;

        mem_del(t);
    }

    return NULL;
}

extern size_t
thread_sync(thread *t, size_t step)
{
    size_t ret = 0;

    _threads.cur->step++;
    if (!_threads.blocked && _threads.cur != t && (u32)t > 0x1 && t->persistent)
    {
        while (t->step < step && !(t->gen->finished))
            generator_yield(_threads.cur->gen);

        ret = t->step;
    }

    return ret;
}

extern size_t
thread_wait(thread *t)
{
    size_t ret = 0;

    _threads.cur->step++;
    if (!_threads.blocked && _threads.cur != t && (u32)t > 0x1 && t->persistent)
    {
        while (!(t->gen->finished))
            generator_yield(_threads.cur->gen);

        ret = t->step;
    }

    return ret;
}

extern bool
thread_rewind(thread *t)
{
    bool ret = false;

    if (!_threads.blocked && _threads.cur != t && (u32)t > 0x1 && t->persistent)
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
    return _threads.cur->gen->arg;
}

extern void
thread_block(bool state)
{
    _threads.blocked = state;
}

extern void
thread_yield(void)
{
    _threads.cur->step++;
    if (!_threads.blocked)
        generator_yield(_threads.cur->gen);
}

extern noreturn
thread_loop(void)
{
    _threads.cur->step = 0;
    generator_rewind(_threads.cur->gen);

    _threads.blocked = false;
    generator_yield(_threads.cur->gen);
}

extern noreturn
thread_finish(void)
{
    _threads.cur->step++;
    _threads.blocked = false;
    generator_finish(_threads.cur->gen);
}
