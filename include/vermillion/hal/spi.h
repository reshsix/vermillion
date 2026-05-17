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

#define VRM_SPI_MODE0 (0 << 0)
#define VRM_SPI_MODE1 (1 << 0)
#define VRM_SPI_MODE2 (2 << 0)
#define VRM_SPI_MODE3 (3 << 0)
#define VRM_SPI_MSB   (0 << 2)
#define VRM_SPI_LSB   (1 << 2)
#define VRM_SPI_CSL   (0 << 3)
#define VRM_SPI_CSH   (1 << 3)

#define VRM_SPI_WAIT     (0 << 0)
#define VRM_SPI_NOWAIT   (1 << 0)
#define VRM_SPI_COMPLETE (0 << 1)
#define VRM_SPI_PARTIAL  (1 << 1)

#ifdef VERMILLION_INTERNALS
typedef struct
{
    void *init, (*clean)(void *);
    bool (*info)    (void *ctx, uint32_t *freq, uint32_t *fields);
    bool (*config)  (void *ctx, uint32_t  freq, uint32_t  fields);
    bool (*limit)   (void *ctx, size_t *count);
    bool (*transfer)(void *ctx, uint8_t *data, size_t count, bool partial);
    bool (*poll)    (void *ctx);
} drv_spi;

typedef struct
{
    const drv_spi *driver;
    void *context;
} dev_spi;

void spi_setup(dev_spi *list, uint8_t count);

bool spi_info    (uint8_t id, uint32_t *freq, uint32_t *fields);
bool spi_config  (uint8_t id, uint32_t  freq, uint32_t  fields);
bool spi_limit   (uint8_t id, size_t *count);
bool spi_transfer(uint8_t id, uint8_t *data, size_t count, uint32_t flags);
bool spi_poll    (uint8_t id);
#endif

struct vrm_spi_v1
{
    bool (*info)    (uint8_t id, uint32_t *freq, uint32_t *fields);
    bool (*config)  (uint8_t id, uint32_t  freq, uint32_t  fields);
    bool (*limit)   (uint8_t id, size_t *count);
    bool (*transfer)(uint8_t id, uint8_t *data, size_t count, uint32_t flags);
    bool (*poll)    (uint8_t id);
};

enum
{
    VRM_SPI_V1 = 0
};

void *spi_driver(uint8_t version);
