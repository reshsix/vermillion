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

#ifndef INTERFACE_TIMER_H
#define INTERFACE_TIMER_H

#include <_types.h>

bool _timer_init(void);
void _timer_clean(void);

u32 timer_clock(void);
void timer_csleep(const u32 n);
void timer_usleep(const u32 n);
void timer_msleep(const u32 n);
void timer_sleep(const u32 n);

#endif
