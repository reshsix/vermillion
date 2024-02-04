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

#include <general/types.h>
#include <general/mem.h>

#include <hal/base/dev.h>
#include <hal/base/drv.h>
#include <hal/generic/stream.h>
#include <hal/classes/spi.h>

#include <system/log.h>

#define SPI_GCR(x) *(volatile u32*)(x + 0x04)
#define SPI_TCR(x) *(volatile u32*)(x + 0x08)
#define SPI_ICR(x) *(volatile u32*)(x + 0x10)
#define SPI_ISR(x) *(volatile u32*)(x + 0x14)
#define SPI_FCR(x) *(volatile u32*)(x + 0x18)
#define SPI_CLK(x) *(volatile u32*)(x + 0x24)
#define SPI_MBC(x) *(volatile u32*)(x + 0x30)
#define SPI_MTC(x) *(volatile u32*)(x + 0x34)
#define SPI_BCC(x) *(volatile u32*)(x + 0x38)
#define SPI_TXD(x) *(volatile u32*)(x + 0x200)
#define SPI_RXD(x) *(volatile u32*)(x + 0x300)

struct spi
{
    u32 addr;
    struct spi_cfg cfg;
    u8 last;
};

static void
init(void **ctx, u32 addr)
{
    struct spi *ret = mem_new(sizeof(struct spi));

    if (ret)
    {
        ret->addr = addr;

        SPI_GCR(ret->addr) |= 1 << 31;
        while (SPI_GCR(ret->addr) & 1 << 31);

        SPI_GCR(ret->addr) |= 1 << 0;
        SPI_GCR(ret->addr) |= 1 << 1;

        *ctx = ret;
    }
}

static void
clean(void *ctx)
{
    struct spi *spi = ctx;
    SPI_GCR(spi->addr) = 0x0;
    mem_del(spi);
}

static bool
stat(void *ctx, u32 idx, u32 *width)
{
    bool ret = true;

    if (ctx)
    {
        switch (idx)
        {
            case STREAM_COMMON:
                *width = sizeof(u8);
                break;
            case SPI_CONFIG:
                *width = sizeof(struct spi_cfg);
                break;
            default:
                ret = false;
                break;
        }
    }

    return ret;
}

static u8
spi_transfer(struct spi *spi, u8 byte)
{
    /* Reset FIFOs */
    SPI_FCR(spi->addr) |= (1 << 31) | (1 << 15);
    while (!(SPI_ISR(spi->addr) & ((1 << 1) | (1 << 5))));
    SPI_ISR(spi->addr) |= 0xFFFFFFFF;

    /* Set all counters */
    SPI_MBC(spi->addr) = 0x1;
    SPI_MTC(spi->addr) = 0x1;
    SPI_BCC(spi->addr) = 0x1;

    /* Add byte to FIFO */
    SPI_TXD(spi->addr) = byte;

    /* Wait for XCH=0 */
    while (SPI_TCR(spi->addr) & (1 << 31));

    /* Set SPI mode */
    SPI_TCR(spi->addr) = (spi->cfg.mode & 0x3) | (spi->cfg.lsb << 12) |
                         (1 << 6) | (1 << 13);

    /* Manually change CS */
    if (spi->cfg.csp) SPI_TCR(spi->addr) |=  (1 << 7);
    else              SPI_TCR(spi->addr) &= ~(1 << 7);

    /* Clean ISR */
    SPI_ISR(spi->addr) |= 0xFFFFFFFF;

    /* Start transfer */
    SPI_TCR(spi->addr) |= (1 << 31);

    /* Wait for completion */
    while (!(SPI_ISR(spi->addr) & (1 << 12)));
    SPI_ISR(spi->addr) |= 0xFFFFFFFF;

    /* Manually change CS */
    if (!(spi->cfg.csp)) SPI_TCR(spi->addr) |=  (1 << 7);
    else                 SPI_TCR(spi->addr) &= ~(1 << 7);

    /* Return result from read pipe */
    return SPI_RXD(spi->addr);
}

static bool
read(void *ctx, u32 idx, void *data)
{
    bool ret = false;

    if (ctx)
    {
        struct spi *spi = ctx;
        switch (idx)
        {
            case STREAM_COMMON:
                ret = true;

                u8 byte = 0;
                if (spi->cfg.duplex)
                    byte = spi->last;
                else
                    byte = spi_transfer(ctx, 0x0);
                mem_copy(data, &byte, sizeof(u8));
                break;

            case SPI_CONFIG:
                ret = true;
                mem_copy(data, &(spi->cfg), sizeof(struct spi_cfg));
                break;
        }
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *data)
{
    bool ret = false;

    if (ctx)
    {
        struct spi *spi = ctx;
        switch (idx)
        {
            case STREAM_COMMON:
                ret = true;

                u8 byte = 0;
                mem_copy(&byte, data, sizeof(u8));
                spi->last = spi_transfer(spi, byte);
                break;

            case SPI_CONFIG:;
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
        }
    }

    return ret;
}

drv_decl (spi, sunxi_spi)
{
    .init = init, .clean = clean,
    .stat = stat, .read = read, .write = write
};
