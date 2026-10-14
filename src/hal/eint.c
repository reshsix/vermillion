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

#define VERMILLION_INTERNALS
#include <vermillion/task.h>
#include <vermillion/hal/eint.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

static dev_eint            *dev_l = NULL;
static struct vrm_task_mut *dev_m = NULL;
static uint8_t              dev_c = 0;

/* Devtree setup */

extern void
eint_setup(dev_eint *list, struct vrm_task_mut *muts, uint8_t count)
{
    dev_l = list;
    dev_m = muts;
    dev_c = count;
}

/* Driver calls */

#define EINT_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

extern bool
vrm_eint_trigger(uint8_t id, uint8_t port, uint8_t slot, uint32_t fields,
                 void (*handler)(void *), void *arg)
{
    bool ret = false;

    VRM_TASK_MUTEX(&(dev_m[id]), 0)
        ret = EINT_CALL(trigger, port, slot, fields, handler, arg);

    return ret;
}

extern bool
vrm_eint_count(uint8_t id, uint8_t *ports, uint8_t *slots)
{
    bool ret = false;

    VRM_TASK_MUTEX(&(dev_m[id]), 0)
        ret = EINT_CALL(count, ports, slots);

    return ret;
}
