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

#ifndef CORE_WHEEL_H
#define CORE_WHEEL_H

#include <core/types.h>
#include <core/timer.h>

#define WHEEL_OUTER_US 10000
#define WHEEL_INNER_US 10

enum wheel_depth
{
    WHEEL_INNER, WHEEL_OUTER
};

struct wheel_slot
{
    void (*handler)(void *), *arg;
    struct wheel_slot *next;
};

dev_timer *wheel_timer(dev_timer *timer);
struct wheel_slot *wheel_events(enum wheel_depth d, u8 jiffies);
bool wheel_schedule(enum wheel_depth d, void (*handler)(void *),
                    void *arg, u8 jiffies);
void wheel_sleep(enum wheel_depth d, u8 jiffies);
u64 wheel_clock(enum wheel_depth d);

#endif
