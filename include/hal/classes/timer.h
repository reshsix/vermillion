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

#pragma once

#include <hal/block.h>

enum timer_index
{
    TIMER_CONFIG = BLOCK_COMMON + 1,
    TIMER_WAIT
};

typedef drv_block drv_timer;
typedef dev_block dev_timer;

struct timer_cb
{
    bool enabled;
    void (*handler)(void *), *arg;
    u32 delay;
};

bool timer_check(dev_timer *dg, bool *enabled,
                 void (**handler)(void *), void **arg, u32 *delay);
bool timer_setup(dev_timer *dg, bool enabled,
                 void (*handler)(void *), void *arg, u32 delay);
bool timer_wait(dev_timer *dg);
