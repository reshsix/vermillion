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

struct video video;

extern bool
_video_init(void)
{
    return true;
}

extern void
_video_clean(void)
{
    return;
}

extern void
video_update(u8* buffer, u16 x, u16 y, u16 w, u16 h)
{
    (void)buffer, (void)x, (void)y, (void)w, (void)h;
}

extern void
video_clear(void)
{
    return;
}

#endif

#ifdef CONFIG_AUDIO_DUMMY

#include <types.h>

struct audio
{
    u8 dummy;
};

struct audio audio;

extern bool
_audio_init(void)
{
    return true;
}

extern void
_audio_clean(void)
{
    return;
}

extern void
audio_note(u16 freq, u16 duration)
{
    (void)freq, (void)duration;
}

extern void
audio_sample(u16 freq, u8 *data, size_t size)
{
    (void)freq, (void)data, (void)size;
}

#endif

#ifdef CONFIG_STORAGE_DUMMY

#include <types.h>

struct storage
{
    u8 dummy;
};

struct storage storage;

extern bool
_storage_init(void)
{
    return true;
}

extern void
_storage_clean(void)
{
    return;
}

extern struct file *
storage_open(char *path)
{
    (void)path;
    return NULL;
}

extern struct file *
storage_close(struct file *f)
{
    (void)f;
    return NULL;
}

extern void
storage_info(struct file *f, size_t *size, s32 *files)
{
    (void)f, (void)size, (void)files;
}

extern struct file *
storage_index(struct file *f, u32 index)
{
    (void)f, (void)index;
    return NULL;
}

extern bool
storage_read(struct file *f, u32 sector, u8 *buffer)
{
    (void)f, (void)sector, (void)buffer;
    return false;
}

#endif
