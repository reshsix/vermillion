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

#define VRM_FLAG_UART0  (1ULL << 0)
#define VRM_FLAG_UART1  (1ULL << 1)
#define VRM_FLAG_UART2  (1ULL << 2)
#define VRM_FLAG_GPIO0  (1ULL << 3)
#define VRM_FLAG_GPIO1  (1ULL << 4)
#define VRM_FLAG_SPI0   (1ULL << 5)
#define VRM_FLAG_LEDS   (1ULL << 6)
#define VRM_FLAG_TIMER0 (1ULL << 7)
#define VRM_FLAG_TIMER1 (1ULL << 8)
#define VRM_FLAG_SDCARD (1ULL << 9)
#define VRM_FLAG_ALL    (-1LL)

uint64_t vrm_devtree_init(uint8_t platform, uint8_t board, uint64_t flags);
void vrm_devtree_clean(void);

#define VRM_DEVTREE(platform, board, flags) \
    for (uint64_t vrm_flags = vrm_devtree_init(VRM_PLATFORM_##platform, \
                                               VRM_BOARD_##board, (flags)); \
         vrm_flags; (vrm_flags = 0, vrm_devtree_clean())) \
        if (vrm_flags == (flags))
