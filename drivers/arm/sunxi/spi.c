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

#define VERMILLION_INTERNALS
#include <vermillion/hal/spi.h>

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
    u32 fields;
    bool csh;

    u8 *data;
    size_t count;
    bool partial;

    bool busy;
};

static struct spi spis[2] = {0};

static bool
info(void *ctx, u32 *freq, u32 *fields)
{
    bool ret = false;

    struct spi *spi = ctx;
    if (spi)
    {
        *freq   = spi->freq;
        *fields = spi->fields;
        ret = true;
    }

    return ret;
}

static bool
config(void *ctx, u32 freq, u32 fields)
{
    bool ret = (freq <= 24000000 && freq >= 367);

    if (ret)
    {
        struct spi *spi = ctx;
        while (SPI_TCR(spi->addr) & (1 << 31));

        u32 divider = 24000000 / freq;
        u8 mode  = (fields >> 0) & 0x3;
        bool lsb = (fields >> 2) & 0x1;
        bool csh = (fields >> 3) & 0x1;

        if (divider > 512)
        {
            if ((divider & (divider - 1)) == 0)
            {
                u8 log2 = 0;
                while (divider >>= 1)
                    log2++;

                divider = log2 << 8;
            }
            else
                ret = false;
        }
        else if (divider == 1)
            divider = 0;
        else if (divider != 0)
            divider = (1 << 12) | ((divider / 2) - 1);
        else
            ret = false;

        if (mode >= 4)
            ret = false;

        if (ret)
        {
            SPI_CLK(spi->addr) = divider;
            SPI_TCR(spi->addr) = (mode & 0x3) | (lsb << 12) |
                                 (1   <<   6) | (1   << 13) ;
            spi->freq   = freq;
            spi->fields = fields;
            spi->csh    = csh;

            /* Chip Select inactive */
            if (spi->csh)
                SPI_TCR(spi->addr) &= ~(1 << 7);
            else
                SPI_TCR(spi->addr) |=  (1 << 7);
        }
    }

    return ret;
}

static bool
limit(void *ctx, size_t *count)
{
    (void)ctx;
    *count = 64;
    return true;
}

static bool
transfer(void *ctx, u8 *data, size_t count, u32 flags)
{
    bool ret = (count <= 64);

    /* Check if XCH = 0 */
    struct spi *spi = ctx;
    if (ret)
        ret = !(SPI_TCR(spi->addr) & (1 << 31));

    if (ret)
    {
        spi->data    = (flags & VRM_SPI_NO_RX) ? NULL : data;
        spi->count   = count;
        spi->partial = flags & VRM_SPI_PARTIAL;

        /* Set all counters */
        SPI_MBC(spi->addr) = count;
        SPI_MTC(spi->addr) = count;
        SPI_BCC(spi->addr) = count;

        /* Write bytes to TXFIFO */
        for (size_t i = 0; i < count; i++)
            SPI_TXD(spi->addr) = (flags & VRM_SPI_NO_TX) ? 0 : data[i];

        /* Chip Select active */
        if (spi->csh)
            SPI_TCR(spi->addr) |=  (1 << 7);
        else
            SPI_TCR(spi->addr) &= ~(1 << 7);

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
        if (spi->data)
        {
            for (size_t i = 0; i < spi->count; i++)
                spi->data[i] = SPI_RXD(spi->addr);
        }
        else
        {
            volatile u8 a = 0;
            for (size_t i = 0; i < spi->count; i++)
                a = SPI_RXD(spi->addr);
            (void)a;
        }

        /* Chip Select inactive */
        if (!(spi->partial))
        {
            if (spi->csh)
                SPI_TCR(spi->addr) &= ~(1 << 7);
            else
                SPI_TCR(spi->addr) |=  (1 << 7);
        }
    }

    return ret;
}

static const drv_spi sunxi_spi =
{
    .info = info,   .config = config,
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
