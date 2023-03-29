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

#include <_types.h>

#include <h3/ports.h>

#include <vermillion/drivers.h>

extern void
print(const char *s)
{
    const struct driver *serial = driver_find(DRIVER_TYPE_SERIAL, 0);
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            serial->routines.serial.write(0, s[0]);
    }
}

static void
print_h8(const u8 n)
{
    const struct driver *serial = driver_find(DRIVER_TYPE_SERIAL, 0);
    for (u8 i = 1; i <= 1; i--)
    {
        u8 x = (n >> (i * 4)) & 0xF;
        if (x < 10)
            serial->routines.serial.write(0, x + '0');
        else
            serial->routines.serial.write(0, x - 10 + 'A');
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
    const struct driver *serial = driver_find(DRIVER_TYPE_SERIAL, 0);

    bool start = false;

    u32 a = n;
    for (int i = 1000000000;; i /= 10)
    {
        u8 d = a / i;
        if (d != 0)
            start = true;

        if (start)
        {
            serial->routines.serial.write(0, d + '0');
            a -= i * d;
        }

        if (i == 1)
            break;
    }

    if (!start)
        serial->routines.serial.write(0, '0');
}
