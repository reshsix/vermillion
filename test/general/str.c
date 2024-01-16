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
#include <general/mem.h>
#include <general/str.h>

#include <debug/exit.h>
#include <debug/assert.h>

extern void
main(void)
{
    u32 ret = 0;

    char str[] = "test1234";
    char str2[] = "test5678";

    assert (str_length(str) == sizeof(str) - 1);
    assert (str_comp(str, str2, 0) != 0);
    if (!(ret & 0x2) && str_comp(str, str2, 4) != 0)
        ret |= 0x2;

    assert (str_span(str, "tes", false) == 4);
    assert (str_span(str, "234", true) == 5);
    assert (str_find_l(str, 'e') == &(str[1]));
    assert (str_find_r(str, 't') == &(str[3]));
    assert (str_find_m(str, "abs12") == &(str[2]));
    assert (str_find_s(str, "34") == &(str[6]));

    char *tok = NULL;
    assert (str_token(str, "15", &tok) == str);
    assert (str[4] == '\0');
    assert (tok == &(str[5]));
    assert (str_token(NULL, "15", &tok) == &(str[5]));
    assert (tok == &(str[8]));

    char buf[32] = {0}, zeros[32] = {0};
    str_copy(buf, str2, 0);
    assert (mem_comp(buf, str2, sizeof(str2)) == 0);

    char buf2[32] = {0};
    str_copy(buf2, str2, 4);
    assert (mem_comp(buf2, str2, 4) == 0);
    assert (mem_comp(&(buf2[4]), zeros, sizeof(buf) - 4) == 0);

    char buf3[32] = {[0 ... 31] = 0xFF};
    str_copy(buf3, str2, sizeof(buf3));
    assert (mem_comp(buf3, str2, sizeof(str2)) == 0);
    assert (mem_comp(&(buf3[sizeof(str2)]), zeros,
                     sizeof(buf3) - sizeof(str2)) == 0);

    char buf4[32] = "testing";
    str_concat(buf4, str2, 0);
    assert (mem_comp(buf4, "testing", 7) == 0);
    assert (mem_comp(&(buf4[7]), str2, 15 - 7) == 0);
    assert (mem_comp(&(buf4[15]), zeros, sizeof(buf4) - 15) == 0);

    char buf5[32] = "testing";
    buf5[15] = 0xFF;
    str_concat(buf5, str2, 32);
    assert (mem_comp(buf5, "testing", 7) == 0);
    assert (mem_comp(&(buf5[7]), str2, 15 - 7) == 0);
    assert (mem_comp(&(buf5[15]), zeros, sizeof(buf5) - 15) == 0);

    char buf6[32] = "testing";
    buf6[11] = 0xFF;
    str_concat(buf6, str2, 4);
    assert (mem_comp(buf6, "testing", 7) == 0);
    assert (mem_comp(&(buf6[7]), str2, 11 - 7) == 0);
    assert (mem_comp(&(buf6[11]), zeros, sizeof(buf6) - 15) == 0);

    char *dup = str_dupl(str2, 0);
    assert (dup != str2);
    assert (mem_comp(dup, str2, sizeof(str2)) == 0);
    mem_del(dup);

    dup = str_dupl(str2, 4);
    assert (dup != str2);
    assert (mem_comp(dup, str2, 4) == 0);
    assert (dup[5] == '\0');
    mem_del(dup);

    exit_qemu(assert_failed);
}
