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

#include <debug/exit.h>
#include <debug/assert.h>

extern void
main(void)
{
    u8 *a = mem_new(10);
    u8 *b = mem_new(20);
    u8 *c = mem_new(30);
    u8 *d = mem_new(40);
    u8 *e = mem_new(50);

    assert (a && b && c && d && e);
    assert (a + 10 <= b);
    assert (b + 20 <= c);
    assert (c + 30 <= d);
    assert (d + 40 <= e);

    for (u8 i = 0; i < 10; i++)  assert (a[i] == 0);
    for (u8 i = 10; i < 20; i++) assert (b[i] == 0);
    for (u8 i = 20; i < 30; i++) assert (c[i] == 0);
    for (u8 i = 30; i < 40; i++) assert (d[i] == 0);
    for (u8 i = 40; i < 50; i++) assert (e[i] == 0);

    u8 *f = mem_renew(a, 100);
    assert (f);
    assert (b < f && b < (f - 100));
    for (u8 i = 0; i < 100; i++)
        assert (f[i] == 0);

    mem_del(b);
    mem_del(c);
    mem_del(d);
    mem_del(e);
    mem_del(f);
    b = mem_new(10);
    assert (b == a);

    static u8 g[128] = {0};
    static u8 h[128] = {0};

    assert (mem_comp(g, h, 128) == 0);

    g[127] = 1;
    assert (mem_comp(g, h, 128) != 0);
    assert (mem_find(g, 1, 128) == &(g[127]));

    mem_init(g, 2, 128);
    for (u8 i = 0; i < 128; i++)
        assert (g[i] == 2);

    mem_copy(h, g, 128);
    for (u8 i = 0; i < 128; i++)
        assert (h[i] == g[i]);

    g[0] = 1;
    g[1] = 2;
    g[2] = 3;

    mem_copy(&(g[1]), g, 127);
    assert (g[1] = 1);
    assert (g[2] = 2);
    assert (g[3] = 3);

    exit_qemu(assert_failed);
}
