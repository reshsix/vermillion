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

#include <general/types.h>

#include <hal/classes/timer.h>

/* For devtree usage */

void time_config(dev_timer *timer);

/* For external usage */

bool time_event0(void (*handler)(void *), void *arg, u8 jiffies);
bool time_event1(void (*handler)(void *), void *arg, u8 jiffies);
void time_sleep0(u8 jiffies);
void time_sleep1(u8 jiffies);
u64 time_clock0(void);
u64 time_clock1(void);
