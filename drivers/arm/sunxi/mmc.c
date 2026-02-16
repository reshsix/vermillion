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

#include <general/types.h>
#include <general/mem.h>

#include <hal/block.h>

#define SD_CFG(x)   *(volatile u32*)(x + 0x00)
#define SD_BLK(x)   *(volatile u32*)(x + 0x10)
#define SD_CNT(x)   *(volatile u32*)(x + 0x14)
#define SD_CMD(x)   *(volatile u32*)(x + 0x18)
#define SD_ARG(x)   *(volatile u32*)(x + 0x1C)
#define SD_RES0(x)  *(volatile u32*)(x + 0x20)
#define SD_RES1(x)  *(volatile u32*)(x + 0x24)
#define SD_RES2(x)  *(volatile u32*)(x + 0x28)
#define SD_RES3(x)  *(volatile u32*)(x + 0x2C)
#define SD_RAW(x)   *(volatile u32*)(x + 0x38)
#define SD_STA(x)   *(volatile u32*)(x + 0x3C)
#define SD_FIFO(x)  *(volatile u32*)(x + 0x200)

/* Driver definition */

struct card
{
    u32 base;
    bool mmc;
};

struct card cards[3] = {0};

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *depth)
{
    bool ret = true;

    (void)ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            *width = 0x200;
            *depth = 0x800000;
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

static bool
mmc_rw(void *ctx, u32 idx, void *buffer, u32 block, bool write)
{
    bool ret = false;

    struct card *card = ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            ret = true;

            SD_BLK(card->base) = 0x200;
            SD_CFG(card->base) = 1 << 31;

            while (SD_CMD(card->base) >> 31);

            SD_CNT(card->base) = 0x200;
            if (!(card->mmc))
                SD_ARG(card->base) = block;
            else
                SD_ARG(card->base) = block * 0x200;

            if (write)
                SD_CMD(card->base) = 0x80002658;
            else
                SD_CMD(card->base) = 0x80002251;

            while (SD_CMD(card->base) >> 31);

            for (u32 i = 0; i < (0x200 / sizeof(u32)); i++)
            {
                if (write)
                {
                    while (SD_STA(card->base) & (1 << 3));
                    u8 *source = buffer;
                    u32 x = 0;
                    mem_copy(&x, &(source[(i * sizeof(u32))]), sizeof(u32));
                    SD_FIFO(card->base) = x;
                }
                else
                {
                    while (SD_STA(card->base) & (1 << 2));
                    u8 *dest = buffer;
                    u32 x = SD_FIFO(card->base);
                    mem_copy(&(dest[(i * sizeof(u32))]), &x, sizeof(u32));
                }
            }
            break;
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    return mmc_rw(ctx, idx, buffer, block, false);
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    return mmc_rw(ctx, idx, buffer, block, true);
}

static const drv_block sunxi_mmc =
{
    .stat = stat, .read = read, .write = write
};

/* Device creation */

extern dev_block
sunxi_mmc_init(u8 id)
{
    struct card *ret = NULL;

    if (id < 3)
    {
        ret = &(cards[id]);

        switch (id)
        {
            case 0:
                ret->base = 0x01c0f000;
                break;
            case 1:
                ret->base = 0x01c10000;
                break;
            case 2:
                ret->base = 0x01c11000;
                break;
        }

        while (SD_CMD(ret->base) & (1 << 31));
        SD_ARG(ret->base) = 0x0;
        SD_CMD(ret->base) = 1 << 31 | 1 << 6 | 58;
        while (SD_CMD(ret->base) & (1 << 31));
        ret->mmc = (SD_RAW(ret->base)  & (1 << 1) ||
                    SD_RES1(ret->base) & (1 << 2));
    }

    return (dev_block){.driver = &sunxi_mmc, .context = ret};
}

extern void
sunxi_mmc_clean(dev_block *b)
{
    if (b)
        b->context = NULL;
}
