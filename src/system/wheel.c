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

#include <system/wheel.h>

static dev_timer *tmrdev = NULL;

static struct wheel_slot *outer[256] = {0};
static struct wheel_slot *inner[256] = {0};
static u64 current = 0;

static struct wheel_slot **
wheel_slots(enum wheel_depth d)
{
    struct wheel_slot **ret = NULL;

    switch (d)
    {
        case WHEEL_INNER:
            ret = inner;
            break;

        case WHEEL_OUTER:
            ret = outer;
            break;
    }

    return ret;
}

static u8
wheel_index(enum wheel_depth d, u8 jiffies)
{
    u8 ret = 0;

    switch (d)
    {
        case WHEEL_INNER:
            ret = current + jiffies;
            break;

        case WHEEL_OUTER:
            ret = current / (WHEEL_OUTER_US / WHEEL_INNER_US) + jiffies;
            break;
    }

    return ret;
}


static void
handler(void *arg)
{
    (void)arg;

    u8 depth = 1 + ((current % (WHEEL_OUTER_US / WHEEL_INNER_US)) == 0);
    for (u8 i = 0; i < depth; i++)
    {
        struct wheel_slot **slots = wheel_slots(i);
        u8 index = wheel_index(i, 0);

        struct wheel_slot *last = slots[index];
        while (last != NULL)
        {
            last->handler(last->arg);

            struct wheel_slot *next = last->next;
            mem_del(last);
            last = next;
        }
        slots[index] = NULL;
    }

    current++;
}

extern dev_timer *
wheel_timer(dev_timer *timer)
{
    dev_timer *ret = tmrdev;

    if (ret)
        timer_setup(ret, false, NULL, NULL, 0);

    tmrdev = timer;
    if (tmrdev)
        timer_setup(tmrdev, true, handler, NULL, WHEEL_INNER_US);

    return ret;
}

extern struct wheel_slot *
wheel_events(enum wheel_depth d, u8 jiffies)
{
    struct wheel_slot *ret = NULL;

    struct wheel_slot **slots = wheel_slots(d);
    if (slots)
        ret = slots[wheel_index(d, jiffies)];

    return ret;
}

extern bool
wheel_schedule(enum wheel_depth d, void (*handler)(void *),
               void *arg, u8 jiffies)
{
    bool ret = false;

    struct wheel_slot **slots = wheel_slots(d);
    if (slots)
    {
        u8 index = wheel_index(d, jiffies);

        struct wheel_slot *ev = mem_new(sizeof(struct wheel_slot));
        if (ev)
        {
            ret = true;

            ev->handler = handler;
            ev->arg = arg;

            if (slots[index])
            {
                struct wheel_slot *last = slots[index];
                while (last->next != NULL)
                    last = last->next;

                last->next = ev;
            }
            else
                slots[index] = ev;
        }
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
wheel_sleep(enum wheel_depth d, u8 jiffies)
{
    bool flag = false;
    wheel_schedule(d, sleeper, &flag, jiffies);

    while (!flag)
        timer_wait(tmrdev);
}

extern u64
wheel_clock(enum wheel_depth d)
{
    u64 ret = 0;

    switch (d)
    {
        case WHEEL_INNER:
            ret = current;
            break;
        case WHEEL_OUTER:
            ret = current / (WHEEL_OUTER_US / WHEEL_INNER_US);
            break;
    }

    return ret;
}
