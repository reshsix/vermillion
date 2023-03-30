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

#include <_types.h>
#include <stdlib.h>

#include <h3/ports.h>

#include <vermillion/drivers.h>

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
    const struct driver *t0 = driver_find(DRIVER_TYPE_TIMER, 0);
    const u32 delay = 1000000 / freq / 2;
    for (u32 i = 0; i < (duration * 1000) / (delay * 2); i++)
    {
        pin_write(bz->pin, true);
        t0->routines.timer.usleep(delay);
        pin_write(bz->pin, false);
        t0->routines.timer.usleep(delay);
    }
}

static void
buzzer_sample(struct buzzer *bz, u16 freq, u8 *data, size_t size)
{
    const struct driver *t0 = driver_find(DRIVER_TYPE_TIMER, 0);
    for (size_t i = 0; i < size; i++)
    {
        pin_write(bz->pin, data[i] >= UINT8_MAX / 2);
        t0->routines.timer.usleep(1000000 / freq);
    }
}

struct audio
{
    struct buzzer *bz;
};

static struct audio audio;

static void
clean(void)
{
    buzzer_del(audio.bz);
}

static bool
init(void)
{
    bool ret = false;

    audio.bz = buzzer_new(CONFIG_BUZZER_PIN);
    if (audio.bz)
        ret = true;
    else
        clean();

    return ret;
}

static void
audio_note(u16 freq, u16 duration)
{
    buzzer_note(audio.bz, freq, duration);
}

static void
audio_sample(u16 freq, u8 *data, size_t size)
{
    buzzer_sample(audio.bz, freq, data, size);
}

static const struct driver buzzer =
{
    .name = "Buzzer",
    .init = init, .clean = clean,
    .type = DRIVER_TYPE_AUDIO,
    .routines.audio.note   = audio_note,
    .routines.audio.sample = audio_sample
};
driver_register(buzzer);
