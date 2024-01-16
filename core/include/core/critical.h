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

#ifndef CORE_CRITICAL_H
#define CORE_CRITICAL_H

#include <core/types.h>
#include <core/macros.h>

#include <core/thread.h>

#define critical \
    bool UNIQUE(_critical) = true; \
    bool UNIQUE(_critical_prev) = thread_list.blocked; \
    for ((thread_list.blocked = true); UNIQUE(_critical); \
         (thread_list.blocked = UNIQUE(_critical_prev)), \
         UNIQUE(_critical) = false)

#endif
