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

#ifndef H3_SD_H
#define H3_SD_H

#include <_types.h>

#define SD 0x01C0F000
#define SD_CFG   *(volatile u32*)(SD + 0x00)
#define SD_BLK   *(volatile u32*)(SD + 0x10)
#define SD_CNT   *(volatile u32*)(SD + 0x14)
#define SD_CMD   *(volatile u32*)(SD + 0x18)
#define SD_ARG   *(volatile u32*)(SD + 0x1C)
#define SD_STA   *(volatile u32*)(SD + 0x3C)
#define SD_FIFO  *(volatile u32*)(SD + 0x200)

static bool
sd_read(u8 *buffer, u32 block, u32 count)
{
    if (count != 0)
    {
        SD_BLK = 0x200;
        SD_CFG = 1 << 31;

        while (SD_CMD >> 31);

        SD_CNT = count * 0x200;
        SD_ARG = block;

        if (count != 1)
            SD_CMD = 0x80003252;
        else
            SD_CMD = 0x80002251;

        while (SD_CMD >> 31);
        while (SD_STA & (1 << 10));

        for (u32 i = 0; i < (count * 0x200 / 4); i++)
        {
            while (SD_STA & (1 << 2));

            u32 x = SD_FIFO;
            for (u8 j = 0; j < 4; j++)
                buffer[(i * 4) + j] = ((u8*)&x)[j];
        }
    }

    return true;
}

#endif
