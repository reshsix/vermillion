/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <vermillion/util/types.h>

#ifdef VERMILLION_INTERNALS
void mem_init(void);
void mem_clean(void);
#endif

void *vrm_mem_new(size_t size);
void *vrm_mem_del(void *mem);
void  vrm_mem_use(size_t *free, size_t *total);

int   vrm_mem_comp(const void *mem, const void *mem2, size_t length);
void *vrm_mem_find(const void *mem, uint8_t c,        size_t length);
void  vrm_mem_fill(void *mem,       uint8_t c,        size_t length);
void  vrm_mem_copy(void *dest,      const void *src,  size_t length);
