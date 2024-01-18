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
#include <general/dict.h>

#include <debug/assert.h>

extern void
test_general_dict(void)
{
    dict *d = dict_new(sizeof(u32));
    dict *pd = d;
    assert (d != NULL);

    d = dict_del(d);
    assert (d == NULL);

    d = dict_new(sizeof(u32));
    assert (d != NULL);
    assert (d == pd);

    u32 a = 1234;
    assert (dict_set(d, "test0", &a));
    a = 456;
    assert (dict_set(d, "test1", &a));
    a = 789;
    assert (dict_set(d, "test0", &a));

    assert (dict_get(d, "test0", &a));
    assert (a == 789);
    assert (dict_get(d, "test1", &a));
    assert (a == 456);

    a = 4567;
    assert (dict_set(d, "test0", NULL));
    assert (!dict_get(d, "test0", &a));
    assert (a == 4567);
    assert (dict_get(d, "test1", &a));
    assert (a == 456);

    char test[] = "test2";
    a = 10;
    assert (dict_set(d, test, &a));
    test[4] = '0';
    assert (dict_get(d, "test2", &a));
    assert (a == 10);

    a = 20;
    const char str[] = "abcdefghijklmnopqrstuvwxyz12345678901234567890";
    assert (sizeof(str) > 32);
    for (u8 i = 0; i < sizeof(str); i++)
        assert (dict_set(d, &(str[i]), &a));

    assert (dict_get(d, "test2", &a));
    assert (a == 10);
    assert (dict_get(d, str, &a));
    assert (a == 20);
    assert (dict_get(d, "0", &a));
    assert (a == 20);

    dict_del(d);
}
