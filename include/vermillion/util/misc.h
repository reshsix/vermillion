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

uint8_t vrm_misc_getc (uint8_t id);
void    vrm_misc_putc (uint8_t id, uint8_t  data);
void    vrm_misc_spi  (uint8_t id, uint8_t *data, size_t count, uint32_t flags);
void    vrm_misc_sleep(uint8_t id, uint32_t us);
