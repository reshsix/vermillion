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
#include <thread/implicit.h>

#include <debug/assert.h>

bool flag = false;
thread_decl (static, test_implicit)
{
    u32 arg = (u32)thread_arg();

    implicit for (u32 i = 0; i < arg; i++)
        thread_yield();

    flag = true;
    thread_yield();

    thread_finish();
}

extern void
test_thread_implicit(void)
{
    thread *th = thread_new(test_implicit, (void *)10, true, 89);
    assert (th != NULL);

    for (int i = 0; i < 5; i++)
        thread_yield();
    assert (!flag);

    assert (thread_sync(th, 1) == 1);
    assert (flag);

    th = thread_del(th);
    assert (th == NULL);
}
