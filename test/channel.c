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

#include <core/thread.h>
#include <core/channel.h>

#include <debug/exit.h>
#include <debug/assert.h>

static thread_task (test_channel)
{
    channel *ch = thread_arg();

    u32 test = 0;
    channel_read(ch, &test);
    if (test == 0xAB)
    {
        test = 0xBC;
        channel_write(ch, &test);
        test = 0xCD;
        channel_write(ch, &test);
    }

    channel_read(ch, &test);
    if (test == 0xDE)
    {
        test = 0xFA;
        channel_write(ch, &test);
    }

    thread_finish();
}

extern void
main(void)
{
    channel *ch = channel_new(sizeof(u32), 0);
    channel *pch = ch;
    assert (ch != NULL);

    ch = channel_del(ch);
    assert (ch == NULL);

    ch = channel_new(sizeof(u32), 0);
    assert (ch != NULL);
    assert (ch == pch);

    thread_new(test_channel, ch, false, 123);

    u32 test = 0xAB;
    channel_write(ch, &test);
    channel_read(ch, &test);
    assert (test == 0xBC);

    channel_read(ch, &test);
    assert (test == 0xCD);

    test = 0xDE;
    channel_write(ch, &test);
    channel_read(ch, &test);
    assert (test == 0xFA);

    channel_del(ch);
    ch = channel_new(sizeof(u32), 5);
    assert (ch != NULL);

    assert (channel_empty(ch));
    channel_write(ch, &test);
    assert (channel_stat(ch) == 4);
    channel_write(ch, &test);
    assert (channel_stat(ch) == 3);
    channel_write(ch, &test);
    assert (channel_stat(ch) == 2);
    channel_write(ch, &test);
    assert (channel_stat(ch) == 1);
    channel_write(ch, &test);
    assert (channel_stat(ch) == 0);
    assert (channel_full(ch));
    channel_del(ch);

    exit_qemu(assert_failed);
}
