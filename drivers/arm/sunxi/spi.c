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

#include <general/types.h>
#include <general/mem.h>

#include <hal/spi.h>

#define SPI_GCR(x) *(volatile u32*)(x + 0x04)
#define SPI_TCR(x) *(volatile u32*)(x + 0x08)
#define SPI_ICR(x) *(volatile u32*)(x + 0x10)
#define SPI_ISR(x) *(volatile u32*)(x + 0x14)
#define SPI_FCR(x) *(volatile u32*)(x + 0x18)
#define SPI_CLK(x) *(volatile u32*)(x + 0x24)
#define SPI_MBC(x) *(volatile u32*)(x + 0x30)
#define SPI_MTC(x) *(volatile u32*)(x + 0x34)
#define SPI_BCC(x) *(volatile u32*)(x + 0x38)
#define SPI_TXD(x) *(volatile u8*)(x + 0x200)
#define SPI_RXD(x) *(volatile u8*)(x + 0x300)

/* Driver definition */

struct spi
{
    u32 addr;

    u32 freq;
    u8 mode;
    bool lsb;

    u8 *data;
    size_t count;
    bool busy;
};

static struct spi spis[2] = {0};

static bool
info(void *ctx, u32 *freq, u8 *mode, bool *lsb)
{
    bool ret = false;

    struct spi *spi = ctx;
    if (spi)
    {
        *freq = spi->freq;
        *mode = spi->mode;
        *lsb  = spi->lsb;
        ret = true;
    }

    return ret;
}

static bool
config(void *ctx, u32 freq, u8 mode, bool lsb)
{
    bool ret = (freq <= 24000000 && freq >= 367 && mode < 4);

    if (ret)
    {
        struct spi *spi = ctx;

        u32 div = 24000000 / freq;
        if (div < 2)
            div = 0;
        else if (div > 512)
        {
            u8 log2 = 0;
            while (div >>= 1)
                log2++;

            div = log2 << 8;
        }
        else
            div = (1 << 12) | ((div / 2) - 1);

        SPI_CLK(spi->addr) = div;
        SPI_TCR(spi->addr) = (mode & 0x3) | (lsb << 12) | (1 << 6) | (1 << 13);

        spi->freq = 24000000 / div;
        spi->mode = mode;
        spi->lsb  = lsb;
    }

    return ret;
}

static bool
begin(void *ctx)
{
    struct spi *spi = ctx;
    SPI_TCR(spi->addr) &= ~(1 << 7);
    return true;
}

static bool
end(void *ctx)
{
    struct spi *spi = ctx;
    SPI_TCR(spi->addr) |= (1 << 7);
    return true;
}

static bool
limit(void *ctx, size_t *count)
{
    *count = 64;
    return true;
}

static bool
transfer(void *ctx, u8 *data, size_t count)
{
    bool ret = (count <= 64);

    /* Check if XCH = 0 */
    struct spi *spi = ctx;
    if (ret)
        ret = !(SPI_TCR(spi->addr) & (1 << 31));

    if (ret)
    {
        spi->data  = data;
        spi->count = count;

        /* Set all counters */
        SPI_MBC(spi->addr) = count;
        SPI_MTC(spi->addr) = count;
        SPI_BCC(spi->addr) = count;

        /* Write bytes to TXFIFO */
        for (size_t i = 0; i < count; i++)
            SPI_TXD(spi->addr) = spi->data[i];

        /* Start transfer */
        SPI_TCR(spi->addr) |= (1 << 31);
    }

    return ret;
}

static bool
poll(void *ctx)
{
    bool ret = true;

    /* Check if XCH = 0 */
    struct spi *spi = ctx;
    if (ret)
        ret = !(SPI_TCR(spi->addr) & (1 << 31));

    if (ret)
    {
        /* Read bytes from RXFIFO */
        for (size_t i = 0; i < spi->count; i++)
            spi->data[i] = SPI_RXD(spi->addr);
    }

    return ret;
}

static const drv_spi sunxi_spi =
{
    .info = info, .config = config,
    .begin = begin, .end = end,
    .limit = limit, .transfer = transfer, .poll = poll
};

/* Device creation */

extern dev_spi
sunxi_spi_init(u8 id)
{
    struct spi *ret = NULL;

    if (id < 2)
    {
        ret = &(spis[id]);
        switch (id)
        {
            case 0:
                ret->addr = 0x01c68000;
                break;
            case 1:
                ret->addr = 0x01c69000;
                break;
        }

        SPI_GCR(ret->addr) |= 1 << 31;
        while (SPI_GCR(ret->addr) & 1 << 31);

        /* Enable, Master mode */
        SPI_GCR(ret->addr) |= 1 << 0;
        SPI_GCR(ret->addr) |= 1 << 1;

        /* Mode 0, MSB first, Software CS, No delay */
        SPI_TCR(ret->addr) = (1 << 6) | (1 << 13);
    }

    return (dev_spi){.driver = &sunxi_spi, .context = ret};
}

extern void
sunxi_spi_clean(dev_spi *s)
{
    if (s)
    {
        struct spi *spi = s->context;
        SPI_GCR(spi->addr) = 0x0;
    }
}
