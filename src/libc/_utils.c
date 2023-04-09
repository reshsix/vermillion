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

#include <stdio.h>
#include <_types.h>

extern void
print(const char *s)
{
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            fputc(s[0], stdout);
    }
}

static void
print_h8(const u8 n)
{
    for (u8 i = 1; i <= 1; i--)
    {
        u8 x = (n >> (i * 4)) & 0xF;
        if (x < 10)
            fputc(x + '0', stdout);
        else
            fputc(x - 10 + 'A', stdout);
    }
}

extern void
print_hex(const u32 n)
{
    print("0x");
    if (n >= (1 << 24))
        print_h8(n >> 24);
    if (n >= (1 << 16))
        print_h8(n >> 16);
    if (n >= (1 << 8))
        print_h8(n >> 8);
    print_h8(n);
}

extern void
print_uint(const u32 n)
{
    bool start = false;

    u32 a = n;
    for (int i = 1000000000;; i /= 10)
    {
        u8 d = a / i;
        if (d != 0)
            start = true;

        if (start)
        {
            fputc(d + '0', stdout);
            a -= i * d;
        }

        if (i == 1)
            break;
    }

    if (!start)
        fputc('0', stdout);
}
