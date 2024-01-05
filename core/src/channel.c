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
#include <core/channel.h>
#include <core/generator.h>

struct _channel
{
    size_t type, size, count;
    u8 *buffer;
};

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

extern void
channel_read(channel *ch, void *data)
{
    _threads.cur->step++;
    if (!(_threads.blocked))
    {
        while (ch->count == 0)
            generator_yield(_threads.cur->gen);

        mem_copy(data, &(ch->buffer[--(ch->count)]), ch->type);
        generator_yield(_threads.cur->gen);
    }
}

extern void
channel_write(channel *ch, void *data)
{
    _threads.cur->step++;
    if (!(_threads.blocked))
    {
        while (ch->count >= ch->size)
            generator_yield(_threads.cur->gen);

        mem_copy(&(ch->buffer[ch->count++]), data, ch->type);
        while (ch->count >= ch->size)
            thread_yield();
    }
}
