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

#define TIME_OUTER_US 10000
#define TIME_INNER_US 10

enum time_depth
{
    TIME_INNER, TIME_OUTER
};

struct time_slot
{
    void (*handler)(void *), *arg;
    struct time_slot *next;
};

static dev_timer *tmrdev = NULL;

static struct time_slot *outer[256] = {0};
static struct time_slot *inner[256] = {0};
static u64 current = 0;

static struct time_slot **
time_slots(enum time_depth d)
{
    struct time_slot **ret = NULL;

    switch (d)
    {
        case TIME_INNER:
            ret = inner;
            break;

        case TIME_OUTER:
            ret = outer;
            break;
    }

    return ret;
}

static u8
time_index(enum time_depth d, u8 jiffies)
{
    u8 ret = 0;

    switch (d)
    {
        case TIME_INNER:
            ret = current + jiffies;
            break;

        case TIME_OUTER:
            ret = current / (TIME_OUTER_US / TIME_INNER_US) + jiffies;
            break;
    }

    return ret;
}


static void
handler(void *arg)
{
    (void)arg;

    u8 depth = 1 + ((current % (TIME_OUTER_US / TIME_INNER_US)) == 0);
    for (u8 i = 0; i < depth; i++)
    {
        struct time_slot **slots = time_slots(i);
        u8 index = time_index(i, 0);

        struct time_slot *last = slots[index];
        while (last != NULL)
        {
            last->handler(last->arg);

            struct time_slot *next = last->next;
            mem_del(last);
            last = next;
        }
        slots[index] = NULL;
    }

    current++;
}

/* For devtree usage */

extern void
time_config(dev_timer *timer)
{
    if (tmrdev)
        timer_setup(tmrdev, false, NULL, NULL, 0);

    tmrdev = timer;
    if (tmrdev)
        timer_setup(tmrdev, true, handler, NULL, TIME_INNER_US);
}

/* For external usage */

static bool
time_event(enum time_depth d, void (*handler)(void *),
           void *arg, u8 jiffies)
{
    bool ret = false;

    struct time_slot **slots = time_slots(d);
    if (slots)
    {
        u8 index = time_index(d, jiffies);

        struct time_slot *ev = mem_new(sizeof(struct time_slot));
        if (ev)
        {
            ret = true;

            ev->handler = handler;
            ev->arg = arg;

            if (slots[index])
            {
                struct time_slot *last = slots[index];
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

static void
time_sleep(enum time_depth d, u8 jiffies)
{
    bool flag = false;
    time_event(d, sleeper, &flag, jiffies);

    while (!flag)
        timer_wait(tmrdev);
}

static u64
time_clock(enum time_depth d)
{
    u64 ret = 0;

    switch (d)
    {
        case TIME_INNER:
            ret = current;
            break;
        case TIME_OUTER:
            ret = current / (TIME_OUTER_US / TIME_INNER_US);
            break;
    }

    return ret;
}

extern bool
time_event0(void (*handler)(void *), void *arg, u8 jiffies)
{
    return time_event(TIME_INNER, handler, arg, jiffies);
}

extern bool
time_event1(void (*handler)(void *), void *arg, u8 jiffies)
{
    return time_event(TIME_OUTER, handler, arg, jiffies);
}

extern void
time_sleep0(u8 jiffies)
{
    time_sleep(TIME_INNER, jiffies);
}

extern void
time_sleep1(u8 jiffies)
{
    time_sleep(TIME_OUTER, jiffies);
}

extern u64
time_clock0(void)
{
    return time_clock(TIME_INNER);
}

extern u64
time_clock1(void)
{
    return time_clock(TIME_OUTER);
}
