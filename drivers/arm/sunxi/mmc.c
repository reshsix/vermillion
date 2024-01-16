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

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/generic/block.h>

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

struct card
{
    u32 base;
    bool mmc;
};

static void
init(void **ctx, u32 base)
{
    struct card *ret = mem_new(sizeof(struct card));

    if (ret)
    {
        ret->base = base;
        while (SD_CMD(ret->base) & (1 << 31));
        SD_ARG(ret->base) = 0x0;
        SD_CMD(ret->base) = 1 << 31 | 1 << 6 | 58;
        while (SD_CMD(ret->base) & (1 << 31));
        ret->mmc = (SD_RAW(ret->base)  & (1 << 1) ||
                    SD_RES1(ret->base) & (1 << 2));
        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    mem_del(ctx);
}

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *depth)
{
    bool ret = true;

    (void)ctx;
    switch (idx)
    {
        case 0:
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
read(void *ctx, u32 idx, void *buffer, u32 block)
{
    bool ret = false;

    struct card *card = ctx;
    switch (idx)
    {
        case 0:
            ret = true;

            SD_BLK(card->base) = 0x200;
            SD_CFG(card->base) = 1 << 31;

            while (SD_CMD(card->base) >> 31);

            SD_CNT(card->base) = 0x200;
            if (!(card->mmc))
                SD_ARG(card->base) = block;
            else
                SD_ARG(card->base) = block * 0x200;

            SD_CMD(card->base) = 0x80002251;
            while (SD_CMD(card->base) >> 31);
            while (SD_STA(card->base) & (1 << 10));

            for (u32 i = 0; i < (0x200 / 4); i++)
            {
                while (SD_STA(card->base) & (1 << 2));

                u32 x = SD_FIFO(card->base);
                for (u8 j = 0; j < 4; j++)
                    ((u8*)buffer)[(i * 4) + j] = ((u8*)&x)[j];
            }
            break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *buffer, u32 block)
{
    (void)(ctx), (void)(idx), (void)(buffer), (void)(block);
    return false;
}

drv_decl (block, sunxi_mmc)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
