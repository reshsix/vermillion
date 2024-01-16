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

#ifndef CORE_IMPLICIT_H
#define CORE_IMPLICIT_H

#include <core/types.h>
#include <core/macros.h>

#include <core/thread.h>

#define implicit \
    bool UNIQUE(_implicit) = true; \
    bool UNIQUE(_implicit_prev) = thread_list.current->stepping; \
    for ((thread_list.current->stepping = false); UNIQUE(_implicit); \
         (thread_list.current->stepping = UNIQUE(_implicit_prev)), \
         UNIQUE(_implicit) = false)

#endif
