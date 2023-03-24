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
#include <stdlib.h>

#include <h3/ports.h>

#include <interface/timer.h>

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
        timer_usleep(delay);
        pin_write(bz->pin, false);
        timer_usleep(delay);
    }
}

static void
buzzer_sample(struct buzzer *bz, u16 freq, u8 *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        pin_write(bz->pin, data[i] >= UINT8_MAX / 2);
        timer_usleep(1000000 / freq);
    }
}

#endif

#ifdef CONFIG_AUDIO_BUZZER

struct audio
{
    struct buzzer *bz;
};

struct audio audio;

extern void
_audio_clean(void)
{
    buzzer_del(audio.bz);
}

extern bool
_audio_init(void)
{
    bool ret = false;

    audio.bz = buzzer_new(CONFIG_BUZZER_PIN);
    if (audio.bz)
    {
        for (int i = 0; i < 4; i++)
        {
            buzzer_note(audio.bz, 329, 100);
            timer_msleep(100);
        }
        buzzer_note(audio.bz, 523, 500);
        timer_msleep(100);

        ret = true;
    }
    else
        _audio_clean();

    return ret;
}

extern void
audio_note(u16 freq, u16 duration)
{
    buzzer_note(audio.bz, freq, duration);
}

extern void
audio_sample(u16 freq, u8 *data, size_t size)
{
    buzzer_sample(audio.bz, freq, data, size);
}

#endif
