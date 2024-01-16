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

#include <environ/state.h>

#include <debug/exit.h>
#include <debug/assert.h>

extern void
main(void)
{
    state st = {};

    bool flag0 = false, flag1 = false;
    volatile void *test = (void*)987654321;
    register volatile void *test2 = (void*)0xAABBCC;

    void *x = state_save(&st);
    if (x == NULL)
    {
        flag0 = true;
        state_load(&st, (void*)123456789);
    }
    else if (x == (void*)123456789)
        flag1 = true;

    assert (flag0);
    assert (flag1);

    assert (test == (void*)987654321);
    assert (test2 == (void*)0xAABBCC);

    exit_qemu(assert_failed);
}
