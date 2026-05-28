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

/* Considers card to have been initialized by u-boot */

#define VERMILLION_INTERNALS
#include <vermillion/hal/disk.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

#define SD_CFG(x)   *(volatile uint32_t*)(x + 0x00)
#define SD_BLK(x)   *(volatile uint32_t*)(x + 0x10)
#define SD_CNT(x)   *(volatile uint32_t*)(x + 0x14)
#define SD_CMD(x)   *(volatile uint32_t*)(x + 0x18)
#define SD_ARG(x)   *(volatile uint32_t*)(x + 0x1C)
#define SD_RES0(x)  *(volatile uint32_t*)(x + 0x20)
#define SD_RES1(x)  *(volatile uint32_t*)(x + 0x24)
#define SD_RES2(x)  *(volatile uint32_t*)(x + 0x28)
#define SD_RES3(x)  *(volatile uint32_t*)(x + 0x2C)
#define SD_RAW(x)   *(volatile uint32_t*)(x + 0x38)
#define SD_STA(x)   *(volatile uint32_t*)(x + 0x3C)
#define SD_FIFO(x)  *(volatile uint32_t*)(x + 0x200)

/* Driver definition */

struct card
{
    uint32_t base;
    bool mmc;
};

struct card cards[3] = {0};

static bool
size(void *ctx, uint16_t *sector, uint32_t *count)
{
    (void)ctx;
    *sector = 0x200;
    *count  = 0x800000;
    return true;
}

static bool
mmc_rw(void *ctx, uint8_t *buffer, uint32_t block, bool write)
{
    bool ret = true;

    struct card *card = ctx;
    while (SD_STA(card->base) & (1 << 9) ||
           SD_STA(card->base) & (1 << 10));
    while (SD_CMD(card->base) >> 31);

    SD_BLK(card->base) = 0x200;
    SD_CFG(card->base) = 1 << 31;

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

    for (uint32_t i = 0; i < (0x200 / sizeof(uint32_t)); i++)
    {
        if (write)
        {
            while (SD_STA(card->base) & (1 << 3));
            uint32_t x = 0;
            vrm_mem_copy(&x,
                         &(buffer[(i * sizeof(uint32_t))]), sizeof(uint32_t));
            SD_FIFO(card->base) = x;
        }
        else
        {
            while (SD_STA(card->base) & (1 << 2));
            uint32_t x = SD_FIFO(card->base);
            vrm_mem_copy(&(buffer[(i * sizeof(uint32_t))]),
                         &x, sizeof(uint32_t));
        }
    }

    while (SD_STA(card->base) & (1 << 9) ||
           SD_STA(card->base) & (1 << 10));

    return ret;
}

static bool
read(void *ctx, uint8_t *buffer, uint32_t block)
{
    return mmc_rw(ctx, buffer, block, false);
}

static bool
write(void *ctx, uint8_t *buffer, uint32_t block)
{
    return mmc_rw(ctx, buffer, block, true);
}

static const drv_disk sunxi_mmc =
{
    .size = size, .read = read, .write = write
};

/* Device creation */

extern dev_disk
sunxi_mmc_init(uint8_t id)
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

    return (dev_disk){.driver = &sunxi_mmc, .context = ret};
}

extern void
sunxi_mmc_clean(dev_disk *d)
{
    if (d)
        d->context = NULL;
}
