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

#include <debug/assert.h>

static bool flag0 = false, flag1 = false;
static bool flag2 = false, flag3 = false, flag4 = false;
static bool loop = false, finish = false;
thread_decl (static, test_thread)
{
    if (!finish)
    {
        flag0 = true;
        thread_yield();

        thread_list.blocked = true;
        thread_yield();
        thread_yield();
        thread_yield();
        thread_yield();
        thread_yield();
        flag1 = true;
        thread_list.blocked = false;

        flag2 = true;
        thread_yield();

        thread_yield();
        thread_yield();
        thread_yield();
        thread_yield();
        thread_yield();
        thread_yield();
    }
    else
        flag4 = true;

    if (!loop)
    {
        flag3 = true;
        thread_finish();
    }
    else
    {
        loop = false;
        finish = true;
        thread_loop();
    }
}

extern void
test_thread_thread(void)
{
    thread *t = thread_new(test_thread, NULL, false, 98);
    assert (t == (thread *)0x1);

    t = thread_del(t);
    assert (t == NULL);

    t = thread_new(test_thread, NULL, true, 76);
    thread *pt = t;
    assert (t != NULL);
    assert (t != (thread *)0x1);

    t = thread_del(t);
    assert (t == NULL);

    t = thread_new(test_thread, NULL, true, 54);
    assert (t == pt);

    thread_yield();
    assert (flag0);

    thread_yield();
    assert (flag1);

    assert (thread_sync(t, 3) == 7);
    assert (flag2);

    assert (thread_wait(t));
    assert (flag3);

    assert (thread_rewind(t));

    loop = true;
    assert (thread_wait(t));
    assert (flag4);
}