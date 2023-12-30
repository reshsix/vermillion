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

#ifndef CORE_DEV_H
#define CORE_DEV_H

#include <core/types.h>

#include <core/drv.h>

/* Device structures */

#define _DEVICE_TYPE(name) \
    typedef struct __attribute__((packed)) \
    { \
        const drv_##name *driver; \
        void *context; \
    } dev_##name; \

_DEVICE_TYPE(generic)
_DEVICE_TYPE(block)
_DEVICE_TYPE(stream)

_DEVICE_TYPE(video)
_DEVICE_TYPE(audio)
_DEVICE_TYPE(storage)
_DEVICE_TYPE(fs)
_DEVICE_TYPE(timer)
_DEVICE_TYPE(serial)
_DEVICE_TYPE(spi)
_DEVICE_TYPE(gpio)
_DEVICE_TYPE(pic)

/* Device function parameters */

union __attribute__((transparent_union)) dev_generic_ptr
{
    dev_generic *generic;
    dev_pic *pic;
};

union __attribute__((transparent_union)) dev_block_ptr
{
    dev_block *block;
    dev_video *video;
    dev_storage *storage;
    dev_fs *fs;
    dev_timer *timer;
    dev_gpio *gpio;
};

union __attribute__((transparent_union)) dev_stream_ptr
{
    dev_stream *stream;
    dev_audio *video;
    dev_serial *storage;
    dev_spi *timer;
};

/* Device macros */

#define dev(x) _device_##x
#define dev_decl(type, drv_, x) dev_##type dev(x) = {.driver = &drv(drv_)};
#define dev_incl(type, x) extern dev_##type dev(x);

#define dev_init(x, ...) \
{ \
    void *_ctx = NULL; \
    if (dev(x).driver->init) \
        ((void (*)(void **, ...))dev(x).driver->init)(&_ctx,##__VA_ARGS__); \
    dev(x).context = _ctx; \
}
#define dev_config(x, ...) \
{ \
    union config _cfg = {__VA_ARGS__}; \
    dev(x).driver->config.set(dev(x).context, &_cfg); \
}
#define dev_clean(x) \
{ \
    if (dev(x).driver->clean) \
        dev(x).driver->clean(dev(x).context); \
}

#endif
