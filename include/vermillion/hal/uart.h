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

#ifdef VERMILLION_INTERNALS
enum uart_index
{
    UART_BAUD_GET, UART_BAUD_SET
};

typedef struct
{
    void *init, (*clean)(void *);
    bool (*info)  (void *ctx, uint32_t *baud);
    bool (*config)(void *ctx, uint32_t baud);
    bool (*read)  (void *ctx, uint8_t *data);
    bool (*write) (void *ctx, uint8_t data);
} drv_uart;

typedef struct
{
    const drv_uart *driver;
    void *context;
} dev_uart;

void uart_setup(dev_uart *list, uint8_t count);

bool uart_read(uint8_t id, uint8_t *data);
bool uart_write(uint8_t id, uint8_t data);
bool uart_info(uint8_t id, uint32_t *baud);
bool uart_config(uint8_t id, uint32_t baud);
#endif

struct vrm_uart_v1
{
    bool (*read)(uint8_t id, uint8_t *data);
    bool (*write)(uint8_t id, uint8_t data);
    bool (*info)(uint8_t id, uint32_t *baud);
    bool (*config)(uint8_t id, uint32_t baud);
};

enum
{
    VRM_UART_V1 = 0
};

void *uart_driver(uint8_t version);
