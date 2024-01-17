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

#include <general/types.h>

#include <thread/mutex.h>
#include <thread/thread.h>

extern void
mutex_lock(void **m, void *param)
{
    while (*m != NULL)
        thread_yield();

    *m = ((param) ? param : thread_list.current);
}

extern void
mutex_unlock(void **m, void *param)
{
    if (*m == ((param) ? param : thread_list.current))
        *m = NULL;
}