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

#ifdef CONFIG_VIDEO_DUMMY

#include <types.h>

struct video
{
    u8 dummy;
};

extern struct video *
video_new(void)
{
    return NULL;
}

extern struct video *
video_del(struct video *v)
{
    (void)v;
    return NULL;
}

extern void
video_update(struct video *v, u8* buffer, u16 x, u16 y, u16 w, u16 h)
{
    (void)v, (void)buffer, (void)x, (void)y, (void)w, (void)h;
}

extern void
video_clear(struct video *v)
{
    (void)v;
}

#endif

#ifdef CONFIG_AUDIO_DUMMY

#include <types.h>

struct audio
{
    u8 dummy;
};

extern struct audio *
audio_new(void)
{
    return NULL;
}

extern struct audio *
audio_del(struct audio *a)
{
    (void)a;
    return NULL;
}

extern void
audio_note(struct audio *a, u16 freq, u16 duration)
{
    (void)a, (void)freq, (void)duration;
}

extern void
audio_sample(struct audio *a, u16 freq, u8 *data, size_t size)
{
    (void)a, (void)freq, (void)data, (void)size;
}

#endif
