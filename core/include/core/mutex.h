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

#ifndef CORE_MUTEX_H
#define CORE_MUTEX_H

#include <core/types.h>
#include <core/macros.h>

#define MUTEX(...) \
    void *UNIQUE(_mutex_p) = NULL; \
    __VA_OPT__(UNIQUE(_mutex_p) = (void *)__VA_ARGS__;) \
    static void * UNIQUE(_mutex_m) = NULL; \
    bool UNIQUE(_mutex) = true; \
    for (mutex_lock(&UNIQUE(_mutex_m), UNIQUE(_mutex_p)); UNIQUE(_mutex); \
         mutex_unlock(&UNIQUE(_mutex_m), UNIQUE(_mutex_p)), \
         UNIQUE(_mutex) = false)
void mutex_lock(void **m, void *param);
void mutex_unlock(void **m, void *param);

#endif
