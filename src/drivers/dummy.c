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

#ifndef CONFIG_VIDEO

#include <_types.h>

struct video
{
    u8 dummy;
};

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

#ifndef CONFIG_AUDIO

#include <_types.h>

struct audio
{
    u8 dummy;
};

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

#ifndef CONFIG_FS

#include <_types.h>

struct fs
{
    u8 dummy;
};

extern bool
_fs_init(void)
{
    return true;
}

extern void
_fs_clean(void)
{
    return;
}

extern struct file *
fs_open(char *path)
{
    (void)path;
    return NULL;
}

extern struct file *
fs_close(struct file *f)
{
    (void)f;
    return NULL;
}

extern void
fs_info(struct file *f, size_t *size, s32 *files)
{
    (void)f, (void)size, (void)files;
}

extern struct file *
fs_index(struct file *f, u32 index)
{
    (void)f, (void)index;
    return NULL;
}

extern bool
fs_read(struct file *f, u32 sector, u8 *buffer)
{
    (void)f, (void)sector, (void)buffer;
    return false;
}

#endif

#ifndef CONFIG_SERIAL

#include <_types.h>
#include <interface/serial.h>

extern bool
_serial_init(void)
{
    return true;
}

extern void
_serial_clean(void)
{
    return;
}

extern bool
serial_config(u8 port, u32 baud, enum serial_char c,
              enum serial_parity p, enum serial_stop s)
{
    (void)port, (void)baud, (void)c, (void)p, (void)s;
    return true;
}

extern u8
serial_read(u8 port)
{
    (void)port;
    return 0;
}

extern void
serial_write(u8 port, u16 data)
{
    (void)port, (void)data;
    return;
}

#endif

#ifndef CONFIG_TIMER

#include <_types.h>

extern bool
_timer_init(void)
{
    return true;
}

extern void
_timer_clean(void)
{
    return;
}

extern void
timer_csleep(const u32 n)
{
    (void)n;
}

extern void
timer_usleep(const u32 n)
{
    (void)n;
}

extern void
timer_msleep(const u32 n)
{
    (void)n;
}

extern void
timer_sleep(const u32 n)
{
    (void)n;
}

#endif

#ifndef CONFIG_SPI

#include <_types.h>

extern bool
_spi_init(void)
{
    return true;
}

extern void
_spi_clean(void)
{
    return;
}

extern bool
spi_config(u32 freq, u8 mode, bool lsb)
{
    (void)freq, (void)mode, (void)lsb;
    return true;
}

extern u8
spi_transfer(u8 x)
{
    (void)x;
    return 0;
}

#endif
