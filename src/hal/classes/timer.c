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

#include <hal/generic/block.h>
#include <hal/classes/timer.h>

extern bool
timer_check(dev_timer *dt, bool *enabled,
            void (**handler)(void *), void **arg, u32 *delay)
{
    struct timer_cb cb = {0};

    bool ret = block_read((dev_block *)dt, TIMER_CONFIG, &cb, 0);

    if (ret)
    {
        if (enabled)
            *enabled = cb.enabled;
        if (handler)
            *handler = cb.handler;
        if (arg)
            *arg = cb.arg;
        if (delay)
            *delay = cb.delay;
    }

    return ret;
}

extern bool
timer_setup(dev_timer *dt, bool enabled,
            void (*handler)(void *), void *arg, u32 delay)
{
    struct timer_cb cb = {.enabled = enabled, .handler = handler,
                          .arg = arg, .delay = delay};
    return block_write((dev_block *)dt, TIMER_CONFIG, &cb, 0);
}

extern bool
timer_wait(dev_timer *dt)
{
    return block_read((dev_block *)dt, TIMER_WAIT, NULL, 0);
}
