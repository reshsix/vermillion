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
#include <vermillion/hal/timer.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

static dev_timer *dev_l = NULL;
static uint8_t dev_c = 0;

/* Devtree setup */

extern void
timer_setup(dev_timer *list, uint8_t count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define TIMER_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

static void
sleep(void *arg)
{
    bool *flag = arg;
    *flag = true;
}

extern bool
vrm_timer_alarm(uint8_t id, uint32_t us,
                bool repeat, void (*handler)(void *), void *arg)
{
    return TIMER_CALL(alarm, us, repeat, handler, arg);
}

extern bool
vrm_timer_sleep(uint8_t id, uint32_t us)
{
    volatile bool ret = false;

    if (vrm_timer_alarm(id, us, false, sleep, (bool *)&ret))
    {
        while (!ret)
            TIMER_CALL(wait);
    }

    return ret;
}
