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
#include <core/critical.h>

#include <debug/exit.h>
#include <debug/assert.h>

static thread_task (test_critical)
{
    u32 arg = (u32)thread_arg();

    critical for (u32 i = 0; i < arg; i++)
        thread_yield();

    thread_finish();
}

extern void
main(void)
{
    thread *th = thread_new(test_critical, (void *)10, true, 89);
    assert (th != NULL);

    assert (thread_sync(th, 1) == 11);

    th = thread_del(th);
    assert (th == NULL);

    exit_qemu(assert_failed);
}
