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

typedef struct
{
    void *init, (*clean)(void *);
    bool (*info)(void *ctx, u32 *freq, u8 *mode, bool *lsb);
    bool (*config)(void *ctx, u32 freq, u8 mode, bool lsb);
    bool (*begin)(void *ctx);
    bool (*end)(void *ctx);
    bool (*limit)(void *ctx, size_t *count);
    bool (*transfer)(void *ctx, u8 *data, size_t count);
    bool (*poll)(void *ctx);
} drv_spi;

typedef struct
{
    const drv_spi *driver;
    void *context;
} dev_spi;

/* For devtree usage */

void spi_setup(dev_spi *list, u8 count);

/* For external usage */

bool spi_info(u8 id, u32 *freq, u8 *mode, bool *lsb);
bool spi_config(u8 id, u32 freq, u8 mode, bool lsb);
bool spi_begin(u8 id);
bool spi_end(u8 id);
bool spi_limit(u8 id, size_t *count);
bool spi_transfer(u8 id, u8 *data, size_t count);
bool spi_poll(u8 id);
