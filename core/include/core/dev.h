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

#define dev_typedef(x) \
    typedef struct [[gnu::packed]] \
    { \
        const drv_##x *driver; \
        void *context; \
    } dev_##x; \

#define dev(x) _device_##x
#define dev_decl(type, drv_, x) dev_##type dev(x) = {.driver = &drv(drv_)}
#define dev_incl(type, x) extern dev_##type dev(x)

#define dev_init(x, ...) \
{ \
    void *_ctx = NULL; \
    if (dev(x).driver->init) \
        ((void (*)(void **, ...))dev(x).driver->init)(&_ctx,##__VA_ARGS__); \
    dev(x).context = _ctx; \
} (void)0x0
#define dev_config(x, ...) \
{ \
    union config _cfg = {__VA_ARGS__}; \
    dev(x).driver->config.set(dev(x).context, &_cfg); \
} (void)0x0
#define dev_clean(x) \
{ \
    if (dev(x).driver->clean) \
        dev(x).driver->clean(dev(x).context); \
} (void)0x0

#endif
