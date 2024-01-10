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

#ifndef DEBUG_PROFILE_H
#define DEBUG_PROFILE_H

#include <core/types.h>
#include <core/macros.h>

#include <core/wheel.h>

#define profile \
    bool UNIQUE(_profile) = true; \
    for (u64 _start = wheel_clock(WHEEL_INNER); UNIQUE(_profile); \
         profile_result(__FILE__, __LINE__, __func__, \
                        _start, wheel_clock(WHEEL_INNER)), \
         UNIQUE(_profile) = false)
void profile_result(const char *file, int line, const char *func,
                    u64 start, u64 end);

#endif
