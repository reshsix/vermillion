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

#define VRM_UART_8B    (0 << 0)
#define VRM_UART_7B    (1 << 0)
#define VRM_UART_6B    (2 << 0)
#define VRM_UART_5B    (3 << 0)
#define VRM_UART_NONE  (0 << 3)
#define VRM_UART_ODD   (1 << 3)
#define VRM_UART_EVEN  (2 << 3)
#define VRM_UART_MARK  (3 << 3)
#define VRM_UART_SPACE (4 << 3)
#define VRM_UART_1S    (0 << 6)
#define VRM_UART_1HS   (1 << 6)
#define VRM_UART_2S    (2 << 6)

#define VRM_UART_WAIT   (0 << 0)
#define VRM_UART_NOWAIT (1 << 0)

#ifdef VERMILLION_INTERNALS
typedef struct
{
    void *init, (*clean)(void *);
    bool (*info)  (void *ctx, uint32_t *baud, uint32_t *fields);
    bool (*config)(void *ctx, uint32_t  baud, uint32_t  fields);
    bool (*read)  (void *ctx, uint8_t  *data);
    bool (*write) (void *ctx, uint8_t   data);
} drv_uart;

typedef struct
{
    const drv_uart *driver;
    void *context;
} dev_uart;

void uart_setup(dev_uart *list, uint8_t count);

bool uart_info  (uint8_t id, uint32_t *baud, uint32_t *fields);
bool uart_config(uint8_t id, uint32_t  baud, uint32_t  fields);
bool uart_read  (uint8_t id, uint8_t  *data, uint32_t   flags);
bool uart_write (uint8_t id, uint8_t   data, uint32_t   flags);
#endif

struct vrm_uart_v1
{
    bool (*info)  (uint8_t id, uint32_t *baud, uint32_t *fields);
    bool (*config)(uint8_t id, uint32_t baud,  uint32_t  fields);
    bool (*read)  (uint8_t id, uint8_t *data,  uint32_t   flags);
    bool (*write) (uint8_t id, uint8_t  data,  uint32_t   flags);
};

enum
{
    VRM_UART_V1 = 0
};

void *uart_driver(uint8_t version);
