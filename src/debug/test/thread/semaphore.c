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

#include <thread/thread.h>
#include <thread/semaphore.h>

#include <debug/assert.h>

static u8 count = 0;

thread_decl (static, test_semaphore)
{
    bool *arg = thread_arg();

    semaphore (5)
    {
        count++;

        for (u8 i = 0; i < 10; i++)
            thread_yield();

        if (count > 5 || !count)
            *arg = false;

        count--;
    }

    thread_finish();
}

extern void
test_thread_semaphore(void)
{
    bool success = true;

    thread *th[16] = {NULL};
    for (u8 i = 0; i < 16; i++)
    {
        th[i] = thread_new(test_semaphore, &success, true, 123);
        assert (th[i]);
    }

    for (u8 i = 0; i < 16; i++)
        thread_wait(th[i]);

    for (u8 i = 0; i < 16; i++)
    {
        th[i] = thread_del(th[i]);
        assert (!th[i]);
    }

    assert (success);
}
