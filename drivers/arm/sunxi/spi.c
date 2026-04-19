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

#include <hal/classes/spi.h>

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
    struct spi_cfg cfg;

    u8 *data;
    size_t count;
    bool busy;
};

static struct spi spis[2] = {0};

static bool
ioctl(void *ctx, u8 idx, void *data)
{
    bool ret = false;

    struct spi *spi = ctx;
    switch (idx)
    {
        case SPI_CONFIG_GET:
            ret = true;
            mem_copy(data, &(spi->cfg), sizeof(struct spi_cfg));
            break;
        case SPI_CONFIG_SET:;
            struct spi_cfg cfg = {0};
            mem_copy(&cfg, data, sizeof(struct spi_cfg));
            ret = (cfg.freq <= 24000000 && cfg.freq >= 367 && cfg.mode < 4);
            if (ret)
            {
                u32 div = 24000000 / cfg.freq;

                if (div < 2)
                    SPI_CLK(spi->addr) = 0x0;
                else if (div > 512)
                {
                    u8 log2 = 0;
                    while (div >>= 1)
                        log2++;

                    SPI_CLK(spi->addr) = log2 << 8;
                }
                else
                    SPI_CLK(spi->addr) = (1 << 12) | ((div / 2) - 1);

                mem_copy(&(spi->cfg), &cfg, sizeof(struct spi_cfg));
            }
            break;
        case SPI_STATE_CS:
            ret = true;

            bool high = data;
            SPI_TCR(spi->addr) = (spi->cfg.mode & 0x3) | (spi->cfg.lsb << 12) |
                                 (1 << 6) | (1 << 13);

            if (high) SPI_TCR(spi->addr) |=  (1 << 7);
            else      SPI_TCR(spi->addr) &= ~(1 << 7);
            break;
    }

    return ret;
}

static bool
stat(void *ctx, size_t *width, size_t *length)
{
    *width  = sizeof(u8);
    *length = 64;
    return true;
}

static bool
transfer(void *ctx, void *data, size_t count)
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

        /* Set mode and start transfer */
        SPI_TCR(spi->addr) = (spi->cfg.mode & 0x3) | (spi->cfg.lsb << 12) |
                             (1 << 6) | (1 << 13) | (1 << 31);
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
    .ioctl = ioctl, .stat = stat, .transfer = transfer, .poll = poll
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

        SPI_GCR(ret->addr) |= 1 << 0;
        SPI_GCR(ret->addr) |= 1 << 1;
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
