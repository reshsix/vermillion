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
    return false;
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

static bool
serial_config(u32 baud, u8 ch, u8 parity, u8 stop)
{
    (void)baud, (void)ch, (void)parity, (void)stop;
    return false;
}

static u8
serial_read(void)
{
    return 0;
}

static void
serial_write(u16 data)
{
    (void)data;
}

static void
gic_config(u16 n, void (*f)(void), bool enable, u8 priority)
{
    (void)n, (void)f, (void)enable, (void)priority;
}

static void
gic_wait(void)
{
    #if defined(CONFIG_ARCH_ARM)
    asm volatile ("wfi");
    #elif defined(CONFIG_ARCH_I686)
    asm volatile ("hlt");
    #else
    static volatile u8 a = 0;
    while (a == 0);
    #endif
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
    return false;
}

static u8
spi_transfer(u8 x)
{
    (void)x;
    return 0;
}

static void
gpio_count(u8 *ports, u16 *pins, u16 *intrs)
{
    if (ports)
        *ports = 0;
    if (pins)
        *pins = 0;
    if (intrs)
        *intrs = 0;
}

static void
gpio_write(u8 port, u32 data)
{
    (void)port, (void)data;
}

static u32
gpio_read(u8 port)
{
    (void)port;
    return 0;
}

static bool
gpio_cfgpin(u16 pin, u8 role, u8 pull)
{
    (void)pin, (void)role, (void)pull;
    return false;
}

static void
gpio_set(u16 pin, bool value)
{
    (void)pin, (void)value;
}

static bool
gpio_get(u16 pin)
{
    (void)pin;
    return false;
}

static bool
gpio_cfgint(u16 intr, bool enable, u8 level)
{
    (void)intr, (void)enable, (void)level;
    return false;
}

static bool
gpio_ack(u16 intr)
{
    (void)intr;
    return false;
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
    [DRIVER_TYPE_GPIO] =
    {
        .name = "Dummy GPIO Controller",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_GPIO,
        .routines.gpio.count  = gpio_count,
        .routines.gpio.write  = gpio_write,
        .routines.gpio.read   = gpio_read,
        .routines.gpio.cfgpin = gpio_cfgpin,
        .routines.gpio.set    = gpio_set,
        .routines.gpio.get    = gpio_get,
        .routines.gpio.cfgint = gpio_cfgint,
        .routines.gpio.ack    = gpio_ack
    },
    [DRIVER_TYPE_DUMMY] =
    {
        .name = "Dummy-type Dummy",
        .init = NULL, .clean = NULL,
        .type = DRIVER_TYPE_DUMMY
    }
};

/* Extern functions */

static void
drivers_init_type(u8 type)
{
    if (type == DRIVER_TYPE_SERIAL)
    {
        bool serial0 = false;
        for (u32 i = 0; i < _drivers_c; i++)
        {
            if (_drivers[i]->type != type)
                continue;

            bool status = (_drivers[i]->init) ?
                           _drivers[i]->init() : true;
            if (status && !serial0)
            {
                _drivers[i]->routines.serial.config(115200,
                                                     DRIVER_SERIAL_CHAR_8B,
                                                     DRIVER_SERIAL_PARITY_NONE,
                                                     DRIVER_SERIAL_STOP_1B);
                print("\r\nVermillion ");
                print(__VERMILLION__);
                print(" (");
                print(__COMPILATION__);
                print(")\r\n\r\n");

                serial0 = true;
            }

            if (serial0)
            {
                print("Initializing ");
                print(_drivers[i]->name);
                print(": ");
                if (status)
                    print("Success");
                else
                    print("Failure");
                print("\r\n");
            }
        }
    }
    else
    {
        for (u32 i = 0; i < _drivers_c; i++)
        {
            if (_drivers[i]->type == type)
            {
                print("Initializing ");
                print(_drivers[i]->name);
                print(": ");
                if (!(_drivers[i]->init) || _drivers[i]->init())
                    print("Success");
                else
                    print("Failure");
                print("\r\n");
            }
        }
    }
}

extern void
_drivers_init(void)
{
    drivers_init_type(DRIVER_TYPE_SERIAL);

    drivers_init_type(DRIVER_TYPE_GPIO);

    #ifdef CONFIG_LED_SUCCESS
    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.cfgpin(CONFIG_LED_SUCCESS_PIN,
                               DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.set(CONFIG_LED_SUCCESS_PIN, true);
    #endif

    drivers_init_type(DRIVER_TYPE_GIC);
    drivers_init_type(DRIVER_TYPE_SPI);
    drivers_init_type(DRIVER_TYPE_TIMER);
    drivers_init_type(DRIVER_TYPE_VIDEO);
    drivers_init_type(DRIVER_TYPE_AUDIO);
    drivers_init_type(DRIVER_TYPE_STORAGE);
    drivers_init_type(DRIVER_TYPE_FS);
}

static void
drivers_clean_type(u8 type)
{
    const struct driver *serial0 = NULL;

    if (type == DRIVER_TYPE_SERIAL)
        serial0 = driver_find(DRIVER_TYPE_SERIAL, 0);

    for (u32 i = 0; i < _drivers_c; i++)
    {
        if (_drivers[i]->type == type)
        {
            print("Cleaning ");
            print(_drivers[i]->name);
            if (_drivers[i]->clean && _drivers[i] != serial0)
                _drivers[i]->clean();
            print("\r\n");
        }
    }

    if (type == DRIVER_TYPE_SERIAL && serial0->clean)
        serial0->clean();
}

extern void
_drivers_clean(void)
{
    drivers_clean_type(DRIVER_TYPE_FS);
    drivers_clean_type(DRIVER_TYPE_STORAGE);
    drivers_clean_type(DRIVER_TYPE_AUDIO);
    drivers_clean_type(DRIVER_TYPE_VIDEO);
    drivers_clean_type(DRIVER_TYPE_TIMER);
    drivers_clean_type(DRIVER_TYPE_SPI);
    drivers_clean_type(DRIVER_TYPE_GIC);
    drivers_clean_type(DRIVER_TYPE_GPIO);
    drivers_clean_type(DRIVER_TYPE_SERIAL);
}

extern u32
driver_count(u8 type)
{
    u32 ret = 0;

    for (u32 i = 0; i < _drivers_c; i++)
    {
        if (_drivers[i]->type == type)
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

    for (u32 i = 0; i < _drivers_c; i++)
    {
        if (_drivers[i]->type == type)
        {
            if (index == 0)
            {
                ret = _drivers[i];
                break;
            }
            index--;
        }
    }

    return ret;
}
