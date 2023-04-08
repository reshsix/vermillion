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

/* Considers card to have already been initialized by u-boot */

#include <_types.h>
#include <vermillion/drivers.h>

#define SD 0x01C0F000
#define SD_CFG   *(volatile u32*)(SD + 0x00)
#define SD_BLK   *(volatile u32*)(SD + 0x10)
#define SD_CNT   *(volatile u32*)(SD + 0x14)
#define SD_CMD   *(volatile u32*)(SD + 0x18)
#define SD_ARG   *(volatile u32*)(SD + 0x1C)
#define SD_RES0  *(volatile u32*)(SD + 0x20)
#define SD_RES1  *(volatile u32*)(SD + 0x24)
#define SD_RES2  *(volatile u32*)(SD + 0x28)
#define SD_RES3  *(volatile u32*)(SD + 0x2C)
#define SD_RAW   *(volatile u32*)(SD + 0x38)
#define SD_STA   *(volatile u32*)(SD + 0x3C)
#define SD_FIFO  *(volatile u32*)(SD + 0x200)

struct card
{
    bool mmc;
};

struct card card;

static bool
storage_init(void)
{
    while (SD_CMD & (1 << 31));
    SD_ARG = 0x0;
    SD_CMD = 1 << 31 | 1 << 6 | 58;
    while (SD_CMD & (1 << 31));
    card.mmc = (SD_RAW & (1 << 1) || SD_RES1 & (1 << 2));

    return true;
}

static bool
storage_read(u8 *buffer, u32 block, u32 count)
{
    if (count != 0)
    {
        SD_BLK = 0x200;
        SD_CFG = 1 << 31;

        while (SD_CMD >> 31);

        SD_CNT = count * 0x200;
        if (!(card.mmc))
            SD_ARG = block;
        else
            SD_ARG = block * 0x200;

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

static const struct driver sunxi_mmc =
{
    .name = "Sunxi SD/MMC Controller",
    .init = storage_init, .clean = NULL,
    .type = DRIVER_TYPE_STORAGE,
    .routines.storage.read = storage_read
};
driver_register(sunxi_mmc);
