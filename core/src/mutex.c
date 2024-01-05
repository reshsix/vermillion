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

#include <core/types.h>

#include <core/mutex.h>
#include <core/thread.h>

extern void
mutex_lock(void **m, void *param)
{
    while (*m != NULL)
        thread_yield();

    *m = ((param) ? param : _threads.cur);
}

extern void
mutex_unlock(void **m, void *param)
{
    if (*m == ((param) ? param : _threads.cur))
        *m = NULL;
}
