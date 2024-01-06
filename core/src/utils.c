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
#include <core/utils.h>

#include <core/block.h>
#include <core/stream.h>

#include <core/timer.h>
#include <core/serial.h>

extern u32
clock(dev_timer *tmr)
{
    union config cfg = {0};
    tmr->driver->config.get(tmr->context, &cfg);
    return cfg.timer.clock;
}

extern void
csleep(dev_timer *tmr, const u32 n)
{
    block_write((dev_block *)tmr, 0, (u8*)&n, 0);
}

static void
csleep2(dev_timer *tmr, const u32 n, u32 div)
{
    s64 clk = clock(tmr) / div;
    for (s64 a = clk * (s64)n; a > 0; a -= UINT32_MAX)
        csleep(tmr, (a < UINT32_MAX) ? a : UINT32_MAX);
}

extern void
usleep(dev_timer *tmr, const u32 n)
{
    csleep2(tmr, n, 1000000);
}

extern void
msleep(dev_timer *tmr, const u32 n)
{
    csleep2(tmr, n, 1000);
}

extern void
sleep(dev_timer *tmr, const u32 n)
{
    csleep2(tmr, n, 1);
}
