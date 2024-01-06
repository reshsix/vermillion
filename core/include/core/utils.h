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

#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <core/types.h>

#include <core/dev.h>

#include <core/timer.h>

u32 clock(dev_timer *tmr);
void csleep(dev_timer *tmr, const u32 n);
void usleep(dev_timer *tmr, const u32 n);
void msleep(dev_timer *tmr, const u32 n);
void sleep(dev_timer *tmr, const u32 n);

#endif
