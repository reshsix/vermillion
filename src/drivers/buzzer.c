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

#ifdef CONFIG_DRIVERS_BUZZER

#include <types.h>
#include <utils.h>
#include <stdlib.h>

#include <h3/ports.h>

#include <drivers/buzzer.h>

extern struct buzzer *
buzzer_new(enum pin pin)
{
    struct buzzer *ret = malloc(sizeof(struct buzzer));

    if (ret)
    {
        pin_config(pin, PIN_CFG_OUT);
        ret->pin = pin;
    }

    return ret;
}

extern struct buzzer *
buzzer_del(struct buzzer *bz)
{
    if (bz)
    {
        pin_config(bz->pin, PIN_CFG_OFF);
        free(bz);
    }

    return NULL;
}

extern void
buzzer_note(struct buzzer *bz, u16 freq, u16 duration)
{
    const u32 delay = 1000000 / freq / 2;
    for (u32 i = 0; i < (duration * 1000) / (delay * 2); i++)
    {
        pin_write(bz->pin, true);
        usleep(delay);
        pin_write(bz->pin, false);
        usleep(delay);
    }
}

extern void
buzzer_sample(struct buzzer *bz, u16 freq, u8 *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        pin_write(bz->pin, data[i] >= UINT8_MAX / 2);
        usleep(1000000 / freq);
    }
}

#endif
