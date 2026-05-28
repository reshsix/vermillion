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

#define VRM_PLATFORM_SUNXI_H3 0

#define VRM_BOARD_ORANGEPI_ONE 0
#define VRM_BOARD_NANOPI_NEO   1

bool vrm_devtree_init(uint8_t platform, uint8_t board, uint32_t flags);
void vrm_devtree_clean(void);
