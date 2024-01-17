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

#include <hal/generic/stream.h>

#include <system/log.h>

static dev_stream *logdev = NULL;

extern dev_stream *
log_output(dev_stream *logger)
{
    dev_stream *ret = logdev;
    logdev = logger;
    return ret;
}

extern void
log_char(const char c)
{
    if (logdev != NULL)
        stream_write((dev_stream *)logdev, STREAM_COMMON, (char *)&c);
}

extern void
log_string(const char *s)
{
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            log_char(s[0]);
    }
}

extern void
log_bool(const bool n)
{
    if (n)
        log_string("true");
    else
        log_string("false");
}

extern void
log_unsigned(const u64 n)
{
    bool started = false;

    log_string("0x");
    for (u8 i = 0; i < 16; i++)
    {
        u8 x = (n >> ((15 - i) * 4)) & 0xF;
        if (x != 0)
            started = true;

        if (started)
        {
            if (x < 10)
                log_char(x + '0');
            else
                log_char(x - 10 + 'A');
        }
    }

    if (!started)
        log_char('0');
}

extern void
log_signed(s64 n)
{
    bool started = false;

    if (n < 0)
    {
        log_char('-');
        n = -n;
    }

    u64 a = n;
    for (u64 i = 10000000000000000000ULL;; i /= 10)
    {
        u8 d = a / i;
        if (d != 0)
            started = true;

        if (started)
        {
            log_char(d + '0');
            a -= i * d;
        }

        if (i == 1)
            break;
    }

    if (!started)
        log_char('0');
}
