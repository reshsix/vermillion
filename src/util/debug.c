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

#include <stdarg.h>

#define VERMILLION_INTERNALS
#include <vermillion/hal/uart.h>
#include <vermillion/util/types.h>

#include <vermillion/util/debug.h>

static void
debug_str(void (*debug_chr)(char), const char *s)
{
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            debug_chr(s[0]);
    }
}

static void
debug_hex(void (*debug_chr)(char), uint64_t n)
{
    bool started = false;

    debug_str(debug_chr, "0x");
    for (uint8_t i = 0; i < 16; i++)
    {
        uint8_t x = (n >> ((15 - i) * 4)) & 0xF;
        if (x != 0)
            started = true;

        if (started)
        {
            if (x < 10)
                debug_chr(x + '0');
            else
                debug_chr(x - 10 + 'A');
        }
    }

    if (!started)
        debug_chr('0');
}

extern void
debug_dec(void (*debug_chr)(char), int64_t n)
{
    bool started = false;

    if (n < 0)
    {
        debug_chr('-');
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
            debug_chr(d + '0');
            a -= i * d;
        }

        if (i == 1)
            break;
    }

    if (!started)
        debug_chr('0');
}

static void
debug(void (*debug_chr)(char), const char *fmt, va_list args)
{
    for (size_t i = 0; fmt[i]; i++)
    {
        if (fmt[i] == '~')
        {
            i++;
            switch (fmt[i])
            {
                case 'd':
                    debug_dec(debug_chr, va_arg(args, int));
                    break;
                case 'D':
                    debug_dec(debug_chr, va_arg(args, long));
                    break;
                case 'x':
                    debug_hex(debug_chr, va_arg(args, int));
                    break;
                case 'X':
                    debug_hex(debug_chr, va_arg(args, long));
                    break;
                case 'c':
                    debug_chr(va_arg(args, int));
                    break;
                case 's':
                    debug_str(debug_chr, va_arg(args, char *));
                    break;
                case 'p':
                    debug_hex(debug_chr, (uintptr_t)va_arg(args, void *));
                    break;
                default:
                    debug_chr('~');
                    debug_chr(fmt[i]);
                    break;
            }
        }
        else
            debug_chr(fmt[i]);
    }

    debug_str(debug_chr, "\r\n");
}

static void
debug_uart0(char c)
{
    vrm_uart_write(0, c, 0);
}

extern void
vrm_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    debug(debug_uart0, fmt, args);
    va_end(args);
}

extern void
vrm_debug_custom(void (*chr)(char), const char *fmt, va_list args)
{
    debug(chr, fmt, args);
}
