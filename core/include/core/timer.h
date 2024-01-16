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

#ifndef CORE_TIMER_H
#define CORE_TIMER_H

#include <core/drv.h>
#include <core/dev.h>

drv_typedef (block, timer);
dev_typedef (timer);

struct [[gnu::packed]] timer_cb
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

#endif
