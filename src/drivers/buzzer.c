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

#include <drivers/buzzer.h>

extern struct buzzer *
buzzer_new(void (*write)(bool))
{
    struct buzzer *ret = malloc(sizeof(struct buzzer));

    if (ret)
        ret->write = write;

    return ret;
}

extern struct buzzer *
buzzer_del(struct buzzer *bz)
{
    free(bz);
    return bz;
}

extern void
buzzer_note(struct buzzer *bz, u16 freq, u16 duration)
{
    const u32 delay = 1000000 / freq / 2;
    for (u32 i = 0; i < (duration * 1000) / (delay * 2); i++)
    {
        bz->write(true);
        usleep(delay);
        bz->write(false);
        usleep(delay);
    }
}

extern void
buzzer_sample(struct buzzer *bz, u16 freq, u8 *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        bz->write(data[i] >= UINT8_MAX / 2);
        usleep(1000000 / freq);
    }
}

#endif
