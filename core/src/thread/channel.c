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
#include <thread/channel.h>
#include <thread/implicit.h>

extern channel *
channel_new(size_t type, size_t size)
{
    channel *ret = mem_new(sizeof(channel));

    if (ret)
    {
        ret->type = type;
        ret->size = size + 1;

        ret->buffer = mem_new(type * ret->size);
        if (!(ret->buffer))
            ret = mem_del(ret);
    }

    return ret;
}

extern channel *
channel_del(channel *ch)
{
    if (ch)
        mem_del(ch->buffer);

    return mem_del(ch);
}

extern bool
channel_empty(channel *ch)
{
    return ch->count == 0;
}

extern bool
channel_full(channel *ch)
{
    return ch->count >= ch->size - 1;
}

extern size_t
channel_stat(channel *ch)
{
    return ch->size - ch->count - 1;
}

extern void
channel_read(channel *ch, void *data)
{
    thread_list.current->step++;
    implicit if (!(thread_list.blocked))
    {
        while (channel_empty(ch))
            thread_yield();

        mem_copy(data, &(ch->buffer[--(ch->count)]), ch->type);
        thread_yield();
    }
}

extern void
channel_write(channel *ch, void *data)
{
    thread_list.current->step++;
    implicit if (!(thread_list.blocked))
    {
        while (ch->count >= ch->size)
            thread_yield();

        mem_copy(&(ch->buffer[ch->count++]), data, ch->type);
        while (ch->count >= ch->size)
            thread_yield();
    }
}
