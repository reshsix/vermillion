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

#include <core/mutex.h>
#include <core/thread.h>

#include <debug/exit.h>
#include <debug/assert.h>

static bool flag = false, success = true;
static bool resources[3] = {false};

thread_decl (static, test_mutex0)
{
    bool *arg = thread_arg();

    mutex ()
    {
        flag = !flag;
        for (u8 i = 0; i < 10; i++)
            thread_yield();

        if (!flag)
            *arg = false;

        flag = !flag;
    }

    thread_finish();
}

thread_decl (static, test_mutex1)
{
    u32 arg = (u32)thread_arg();

    mutex (arg)
    {
        if (resources[arg])
            success = false;

        resources[arg] = !resources[arg];
        for (u8 i = 0; i < 10; i++)
            thread_yield();

        if (!resources[arg])
            success = false;

        resources[arg] = !resources[arg];
    }

    thread_finish();
}

extern void
main(void)
{
    bool success = true;

    thread *th[16] = {0}, *th2[16] = {0};
    for (u8 i = 0; i < 16; i++)
    {
        th[i] = thread_new(test_mutex0, &success, true, 45);
        assert (th[i]);
        th2[i] = thread_new(test_mutex1, (void *)(i % 3), true, 67);
        assert (th2[i]);
    }

    for (u8 i = 0; i < 16; i++)
    {
        thread_wait(th[i]);
        thread_wait(th2[i]);
    }

    for (u8 i = 0; i < 16; i++)
    {
        th[i] = thread_del(th[i]);
        assert (!th[i]);
        th2[i] = thread_del(th2[i]);
        assert (!th2[i]);
    }
    assert (success);

    exit_qemu(assert_failed);
}
