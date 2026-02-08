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

#include <system/comm.h>

#include <syslog.h>

extern void
syslog_char(const char c)
{
    comm_write0(c);
}

extern void
syslog_string(const char *s)
{
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            syslog_char(s[0]);
    }
}

extern void
syslog_bool(const bool n)
{
    if (n)
        syslog_string("true");
    else
        syslog_string("false");
}

extern void
syslog_unsigned(const u64 n)
{
    bool started = false;

    syslog_string("0x");
    for (u8 i = 0; i < 16; i++)
    {
        u8 x = (n >> ((15 - i) * 4)) & 0xF;
        if (x != 0)
            started = true;

        if (started)
        {
            if (x < 10)
                syslog_char(x + '0');
            else
                syslog_char(x - 10 + 'A');
        }
    }

    if (!started)
        syslog_char('0');
}

extern void
syslog_signed(s64 n)
{
    bool started = false;

    if (n < 0)
    {
        syslog_char('-');
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
            syslog_char(d + '0');
            a -= i * d;
        }

        if (i == 1)
            break;
    }

    if (!started)
        syslog_char('0');
}
