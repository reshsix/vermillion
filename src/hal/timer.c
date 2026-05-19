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

#include <general/mem.h>
#include <general/types.h>

#define VERMILLION_INTERNALS
#include <vermillion/hal/timer.h>

static dev_timer *dev_l = NULL;
static u8 dev_c = 0;

/* Devtree setup */

extern void
timer_setup(dev_timer *list, u8 count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

#define TIMER_CALL(f, ...) \
((id < dev_c) ? dev_l[id].driver->f(dev_l[id].context, ##__VA_ARGS__) : false)

static bool flag = false;
static void
sleep(void *arg)
{
    (void)arg;
    flag = true;
}

extern bool
timer_alarm(u8 id, u32 us, bool repeat, void (*handler)(void *), void *arg)
{
    bool ret = TIMER_CALL(alarm, us, repeat, (handler) ? handler : sleep, arg);
    if (!handler)
    {
        do
        {
            while (!flag)
                TIMER_CALL(wait);
            flag = false;
        } while (repeat);
    }

    return ret;
}

/* ABI definitions */

static struct vrm_timer_v1 v1 =
{
    .alarm = timer_alarm
};

extern void *
timer_driver(u8 version)
{
    void *ret = NULL;

    switch (version)
    {
        case VRM_TIMER_V1:
            ret = &v1;
            break;
    }

    return ret;
}
