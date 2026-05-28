/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#define VERMILLION_INTERNALS
#include <vermillion/hal/uart.h>
#include <vermillion/util/types.h>

#include <vermillion/util/debug.h>

extern void
vrm_debug_char(char c)
{
    vrm_uart_write(0, c, 0);
}

extern void
vrm_debug_string(const char *s)
{
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            vrm_debug_char(s[0]);
    }
}

extern void
vrm_debug_unsigned(uint64_t n)
{
    bool started = false;

    vrm_debug_string("0x");
    for (uint8_t i = 0; i < 16; i++)
    {
        uint8_t x = (n >> ((15 - i) * 4)) & 0xF;
        if (x != 0)
            started = true;

        if (started)
        {
            if (x < 10)
                vrm_debug_char(x + '0');
            else
                vrm_debug_char(x - 10 + 'A');
        }
    }

    if (!started)
        vrm_debug_char('0');
}

extern void
vrm_debug_signed(int64_t n)
{
    bool started = false;

    if (n < 0)
    {
        vrm_debug_char('-');
        n = -n;
    }

    uint64_t a = n;
    for (uint64_t i = 10000000000000000000ULL;; i /= 10)
    {
        uint8_t d = a / i;
        if (d != 0)
            started = true;

        if (started)
        {
            vrm_debug_char(d + '0');
            a -= i * d;
        }

        if (i == 1)
            break;
    }

    if (!started)
        vrm_debug_char('0');
}
