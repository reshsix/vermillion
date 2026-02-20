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

#include <vermillion/lib.h>

static int
test(struct vrm *v)
{
    const char msg[] = "vrm_lib message\r\n";
    for (int i = 0; i < sizeof(msg) - 1; i++)
        v->comm.write0(msg[i]);

    return 0;
}

struct lib
{
    int (*test)(struct vrm *v);
};

extern void *
vrm_lib(void)
{
    static struct lib l = {0};
    l.test = test;
    return &l;
}
