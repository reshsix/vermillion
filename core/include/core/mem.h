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

#ifndef CORE_MEM_H
#define CORE_MEM_H

#include <core/types.h>

void *mem_new(size_t size);
void *mem_renew(void *mem, size_t size);
void *mem_del(void *mem);

int mem_comp(const void *mem, const void *mem2, size_t length);
void *mem_find(const void *mem, u8 c, size_t length);
void mem_init(void *mem, u8 c, size_t length);
void mem_copy(void *dest, const void *src, size_t length);

void _mem_init(void);
void _mem_clean(void);

#endif
