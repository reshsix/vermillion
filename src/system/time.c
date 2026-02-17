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

#include <hal/classes/timer.h>

#include <system/time.h>

/* Basic definitions */

struct time_slot
{
    void (*handler)(void *), *arg;
};
static struct time_slot wheel[256] = {0};
static u64 current = 0;

static void
time_handler(void *arg)
{
    (void)arg;
    u8 index = current;

    if (wheel[index].handler)
        wheel[index].handler(wheel[index].arg);

    wheel[index].handler = NULL;
    wheel[index].arg     = NULL;

    current++;
}

/* For devtree usage */

static dev_timer *tmrdev = NULL;

extern void
time_config(dev_timer *timer)
{
    if (tmrdev)
        timer_setup(tmrdev, false, NULL, NULL, 0);

    tmrdev = timer;
    if (tmrdev)
        timer_setup(tmrdev, true, time_handler, NULL, 10000);
}

/* For external usage */

extern bool
time_event(void (*handler)(void *), void *arg, u8 cs)
{
    bool ret = false;

    u8 index = current + cs;
    if (!(wheel[index].handler))
    {
        wheel[index].handler = handler;
        wheel[index].arg     = arg;
        ret = true;
    }

    return ret;
}

static void
sleeper(void *arg)
{
    bool *flag = arg;
    *flag = true;
}

extern void
time_sleep(u8 cs)
{
    bool flag = false;
    time_event(sleeper, &flag, cs);

    while (!flag)
        timer_wait(tmrdev);
}

extern u64
time_clock(void)
{
    return current;
}
