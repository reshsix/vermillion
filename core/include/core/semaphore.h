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

#ifndef CORE_SEMAPHORE_H
#define CORE_SEMAPHORE_H

#include <core/types.h>
#include <core/macros.h>

#define SEMAPHORE(count) \
    static int UNIQUE(_semaphore_s) = count; \
    bool UNIQUE(_semaphore) = true; \
    for (semaphore_wait(&UNIQUE(_semaphore_s)); UNIQUE(_semaphore); \
         semaphore_signal(&UNIQUE(_semaphore_s)), \
                          UNIQUE(_semaphore) = false)
void semaphore_wait(int *s);
void semaphore_signal(int *s);

#endif
