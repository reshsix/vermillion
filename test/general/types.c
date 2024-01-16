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

#include <debug/exit.h>
#include <debug/assert.h>

extern void
main(void)
{
    assert ((bool)true  == 1);
    assert ((bool)false == 0);

    assert (sizeof(s8)  == 1);
    assert (sizeof(s16) == 2);
    assert (sizeof(s32) == 4);
    assert (sizeof(s64) == 8);

    assert ((s8)-1  < 0);
    assert ((s16)-1 < 0);
    assert ((s32)-1 < 0);
    assert ((s64)-1 < 0);

    assert (sizeof(u8) == 1);
    assert (sizeof(u16) == 2);
    assert (sizeof(u32) == 4);
    assert (sizeof(u64) == 8);

    assert ((u8)-1  > 0);
    assert ((u16)-1 > 0);
    assert ((u32)-1 > 0);
    assert ((u64)-1 > 0);

    exit_qemu(assert_failed);
}
