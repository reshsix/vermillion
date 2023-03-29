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

#include <_utils.h>
#include <vermillion/drivers.h>

/* Dummy driver */

static void
video_info(u16 *width, u16 *height)
{
    if (width)
        *width = 0;
    if (height)
        *height = 0;
}

static void
video_update(u8* buffer, u16 x, u16 y, u16 w, u16 h)
{
    (void)buffer, (void)x, (void)y, (void)w, (void)h;
}

static void
video_clear(void)
{
    return;
}

static void
audio_note(u16 freq, u16 duration)
{
    (void)freq, (void)duration;
}

static void
audio_sample(u16 freq, u8 *data, size_t size)
{
    (void)freq, (void)data, (void)size;
}

static bool
storage_read(u8 *buffer, u32 block, u32 count)
{
    (void)buffer, (void)block, (void)count;
    return true;
}

static struct file *
fs_open(char *path)
{
    (void)path;
    return NULL;
}

static struct file *
fs_close(struct file *f)
{
    (void)f;
    return NULL;
}

static void
fs_info(struct file *f, size_t *size, s32 *files)
{
    (void)f, (void)size, (void)files;
}

static struct file *
fs_index(struct file *f, u32 index)
{
    (void)f, (void)index;
    return NULL;
}

static bool
fs_read(struct file *f, u32 sector, u8 *buffer)
{
    (void)f, (void)sector, (void)buffer;
    return false;
}

static u8
serial_ports(void)
{
    return 0;
}

static bool
serial_config(u8 port, u32 baud, u8 ch, u8 parity, u8 stop)
{
    (void)port, (void)baud, (void)ch, (void)parity, (void)stop;
    return true;
}

static u8
serial_read(u8 port)
{
    (void)port;
    return 0;
}

static void
serial_write(u8 port, u16 data)
{
    (void)port, (void)data;
}

static u8 *
loader_prog(const char *path, u32 *entry)
{
    (void)path, (void)entry;
    return NULL;
}

static void
gic_config(u16 n, void (*f)(void), bool enable, u8 priority)
{
    (void)n, (void)f, (void)enable, (void)priority;
}

static void
gic_wait(void)
{
    return;
}

static u32
timer_clock(void)
{
    return 0;
}

static void
timer_csleep(const u32 n)
{
    (void)n;
}

static void
timer_usleep(const u32 n)
{
    (void)n;
}

static void
timer_msleep(const u32 n)
{
    (void)n;
}

static void
timer_sleep(const u32 n)
{
    (void)n;
}

static bool
spi_config(u32 freq, u8 mode, bool lsb)
{
    (void)freq, (void)mode, (void)lsb;
    return true;
}

static u8
spi_transfer(u8 x)
{
    (void)x;
    return 0;
}

static const struct driver dummy[] =
{
    [DRIVER_TYPE_VIDEO] =
    {
        .name = "Dummy Video",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_VIDEO,
        .routines.video.info    = video_info,
        .routines.video.update  = video_update,
        .routines.video.clear   = video_clear
    },
    [DRIVER_TYPE_AUDIO] =
    {
        .name = "Dummy Audio",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_AUDIO,
        .routines.audio.note    = audio_note,
        .routines.audio.sample  = audio_sample
    },
    [DRIVER_TYPE_STORAGE] =
    {
        .name = "Dummy Storage",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_STORAGE,
        .routines.storage.read  = storage_read
    },
    [DRIVER_TYPE_FS] =
    {
        .name = "Dummy Filesystem",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_FS,
        .routines.fs.open       = fs_open,
        .routines.fs.close      = fs_close,
        .routines.fs.info       = fs_info,
        .routines.fs.index      = fs_index,
        .routines.fs.read       = fs_read
    },
    [DRIVER_TYPE_LOADER] =
    {
        .name = "Dummy Loader",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_LOADER,
        .routines.loader.prog   = loader_prog
    },
    [DRIVER_TYPE_GIC] =
    {
        .name = "Dummy Interrupts",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_GIC,
        .routines.gic.config    = gic_config,
        .routines.gic.wait      = gic_wait
    },
    [DRIVER_TYPE_TIMER] =
    {
        .name = "Dummy Timer",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_TIMER,
        .routines.timer.clock   = timer_clock,
        .routines.timer.csleep  = timer_csleep,
        .routines.timer.usleep  = timer_usleep,
        .routines.timer.msleep  = timer_msleep,
        .routines.timer.sleep   = timer_sleep
    },
    [DRIVER_TYPE_SERIAL] =
    {
        .name = "Dummy UART Controller",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_SERIAL,
        .routines.serial.ports  = serial_ports,
        .routines.serial.config = serial_config,
        .routines.serial.read   = serial_read,
        .routines.serial.write  = serial_write,
    },
    [DRIVER_TYPE_SPI] =
    {
        .name = "Dummy SPI Controller",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_SPI,
        .routines.spi.config    = spi_config,
        .routines.spi.transfer  = spi_transfer,
    },
    [DRIVER_TYPE_DUMMY] =
    {
        .name = "Dummy-type Dummy",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_DUMMY
    }
};

/* Extern functions */

extern void
_drivers_init(u8 type)
{
    if (type == DRIVER_TYPE_SERIAL)
    {
        for (u32 i = 0; i < (u32)&__drivers_c; i++)
        {
            if (__drivers[i].type == type && __drivers[i].init)
                __drivers[i].init();
        }
    }
    else
    {
        for (u32 i = 0; i < (u32)&__drivers_c; i++)
        {
            if (__drivers[i].type == type)
            {
                print("Initializing ");
                print(__drivers[i].name);
                print(": ");
                if (!(__drivers[i].init) || __drivers[i].init())
                    print("Success");
                else
                    print("Failure");
                print("\r\n");
            }
        }
    }
}

extern void
_drivers_clean(u8 type)
{
    for (u32 i = 0; i < (u32)&__drivers_c; i++)
    {
        if (__drivers[i].type == type && __drivers[i].clean)
        {
            print("Cleaning ");
            print(__drivers[i].name);
            __drivers[i].clean();
            print("\r\n");
        }
    }
}

extern u32
driver_count(u8 type)
{
    u32 ret = 0;

    for (u32 i = 0; i < (u32)&__drivers_c; i++)
    {
        if (__drivers[i].type == type)
            ret++;
    }

    return ret;
}

extern struct driver *
driver_find(u8 type, u32 index)
{
    struct driver *ret = NULL;

    if (type >= DRIVER_TYPE_DUMMY)
        type = DRIVER_TYPE_DUMMY;
    ret = (struct driver *)&(dummy[type]);

    for (u32 i = 0; i < (u32)&__drivers_c; i++)
    {
        if (__drivers[i].type == type)
        {
            if (index == 0)
            {
                ret = &(__drivers[i]);
                break;
            }
            index--;
        }
    }

    return ret;
}
