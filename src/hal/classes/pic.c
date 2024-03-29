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
#include <hal/classes/pic.h>

extern bool
pic_state(dev_pic *dp, bool enabled)
{
    return block_write((dev_block *)dp, PIC_STATE, &enabled, 0);
}

extern bool
pic_info(dev_pic *dp, u16 n, bool *enabled, void (**handler)(void *),
         void **arg, enum pic_level *level)
{
    struct pic_irq irq = {0};

    bool ret = block_read((dev_block *)dp, PIC_CONFIG_IRQ, &irq, n);

    if (ret)
    {
        if (enabled)
            *enabled = irq.enabled;
        if (handler)
            *handler = irq.handler;
        if (arg)
            *arg = irq.arg;
        if (level)
            *level = irq.level;
    }

    return ret;
}

extern bool
pic_config(dev_pic *dp, u16 n, bool enabled, void (*handler)(void *),
           void *arg, enum pic_level level)
{
    struct pic_irq irq = {.enabled = enabled, .handler = handler,
                          .arg = arg, .level = level};
    return block_write((dev_block *)dp, PIC_CONFIG_IRQ, &irq, n);
}

extern bool
pic_check(dev_pic *dp, u16 n, bool *enabled,
          void (**handler)(void *, void *), void **arg)
{
    struct pic_swi swi = {0};

    bool ret = block_read((dev_block *)dp, PIC_CONFIG_SWI, &swi, n);

    if (ret)
    {
        if (enabled)
            *enabled = swi.enabled;
        if (handler)
            *handler = swi.handler;
        if (arg)
            *arg = swi.arg;
    }

    return ret;
}

extern bool
pic_setup(dev_pic *dp, u16 n, bool enabled,
          void (*handler)(void *, void *), void *arg)
{
    struct pic_swi swi = {.enabled = enabled, .handler = handler, .arg = arg};
    return block_write((dev_block *)dp, PIC_CONFIG_SWI, &swi, n);
}

extern bool
pic_wait(dev_pic *dp)
{
    return block_write((dev_block *)dp, PIC_WAIT, NULL, 0);
}

extern bool
pic_syscall(dev_pic *dp, u8 id, void *data)
{
    return block_write((dev_block *)dp, PIC_SYSCALL, (void *)&data, id);
}
