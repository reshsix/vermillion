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

#include <vermillion/util/types.h>

#define VRM_EINT_EDGE_H  (0 << 0)
#define VRM_EINT_EDGE_L  (1 << 0)
#define VRM_EINT_LEVEL_H (2 << 0)
#define VRM_EINT_LEVEL_L (3 << 0)
#define VRM_EINT_DOUBLE  (4 << 0)

#ifdef VERMILLION_INTERNALS
typedef struct
{
    void *init, (*clean)(void *);
    bool (*trigger)(void *ctx, uint8_t port, uint8_t slot, uint32_t fields,
                    void (*handler)(void *), void *arg);
    bool (*count)(void *ctx, uint8_t *ports, uint8_t *slots);
} drv_eint;

typedef struct
{
    const drv_eint *driver;
    void *context;
} dev_eint;

#include <vermillion/task.h>

void eint_setup(dev_eint *list, struct vrm_task_mut *muts, uint8_t count);
#endif

bool vrm_eint_trigger(uint8_t id, uint8_t port, uint8_t slot, uint32_t fields,
                      void (*handler)(void *), void *arg);
bool vrm_eint_count(uint8_t id, uint8_t *ports, uint8_t *slots);
