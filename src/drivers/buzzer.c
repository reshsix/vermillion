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

#ifdef CONFIG_AUDIO_BUZZER

#include <types.h>
#include <utils.h>
#include <stdlib.h>

#include <h3/ports.h>

struct buzzer
{
    enum pin pin;
};

static struct buzzer *
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

static struct buzzer *
buzzer_del(struct buzzer *bz)
{
    if (bz)
    {
        pin_config(bz->pin, PIN_CFG_OFF);
        free(bz);
    }

    return NULL;
}

static void
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

static void
buzzer_sample(struct buzzer *bz, u16 freq, u8 *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        pin_write(bz->pin, data[i] >= UINT8_MAX / 2);
        usleep(1000000 / freq);
    }
}

#endif

#ifdef CONFIG_AUDIO_BUZZER

struct audio
{
    struct buzzer *bz;
};

extern struct audio *
audio_del(struct audio *a)
{
    if (a)
    {
        buzzer_del(a->bz);
        free(a);
    }

    return NULL;
}

extern struct audio *
audio_new(void)
{
    struct audio *ret = malloc(sizeof(struct audio));

    if (ret)
    {
        ret->bz = buzzer_new(CONFIG_BUZZER_PIN);
        if (!(ret->bz))
            ret = audio_del(ret);
    }

    if (ret)
    {
        for (int i = 0; i < 4; i++)
        {
            buzzer_note(ret->bz, 329, 100);
            msleep(100);
        }
        buzzer_note(ret->bz, 523, 500);
        msleep(100);
    }

    return ret;
}

extern void
audio_note(struct audio *a, u16 freq, u16 duration)
{
    buzzer_note(a->bz, freq, duration);
}

extern void
audio_sample(struct audio *a, u16 freq, u8 *data, size_t size)
{
    buzzer_sample(a->bz, freq, data, size);
}

#endif
