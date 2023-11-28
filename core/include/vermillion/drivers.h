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

#ifndef VERMILLION_DRIVERS_H
#define VERMILLION_DRIVERS_H

#include <vermillion/types.h>

/* Definitions for specific driver types */

union __attribute__((packed)) config
{
    struct __attribute__((packed))
    {
        u16 width;
        u16 height;
        enum
        {
            DRIVER_VIDEO_FORMAT_GRAY,
            DRIVER_VIDEO_FORMAT_RGBX32, DRIVER_VIDEO_FORMAT_BGRX32,
            DRIVER_VIDEO_FORMAT_RGBA32, DRIVER_VIDEO_FORMAT_BGRA32,
            DRIVER_VIDEO_FORMAT_XRGB32, DRIVER_VIDEO_FORMAT_XBGR32,
            DRIVER_VIDEO_FORMAT_ARGB32, DRIVER_VIDEO_FORMAT_ABGR32,
        } format;
    } video;

    struct __attribute__((packed))
    {
        u32 freq;
        enum
        {
            DRIVER_AUDIO_FORMAT_PCM8,
            DRIVER_AUDIO_FORMAT_PCM16LE, DRIVER_AUDIO_FORMAT_PCM16BE,
            DRIVER_AUDIO_FORMAT_PCM24LE, DRIVER_AUDIO_FORMAT_PCM24BE,
            DRIVER_AUDIO_FORMAT_PCM32LE, DRIVER_AUDIO_FORMAT_PCM32BE,
            DRIVER_AUDIO_FORMAT_PCM32FP, DRIVER_AUDIO_FORMAT_PCM64FP
        } format;
    } audio;

    struct __attribute__((packed))
    {
        u32 baud;
        enum
        {
            DRIVER_SERIAL_CHAR_5B, DRIVER_SERIAL_CHAR_6B,
            DRIVER_SERIAL_CHAR_7B, DRIVER_SERIAL_CHAR_8B,
        } bits;
        enum
        {
            DRIVER_SERIAL_PARITY_NONE,
            DRIVER_SERIAL_PARITY_ODD,  DRIVER_SERIAL_PARITY_EVEN,
            DRIVER_SERIAL_PARITY_MARK, DRIVER_SERIAL_PARITY_SPACE
        } parity;
        enum
        {
            DRIVER_SERIAL_STOP_1B,
            DRIVER_SERIAL_STOP_1HB, DRIVER_SERIAL_STOP_2B
        } stop;
    } serial;

    struct __attribute__((packed))
    {
        u32 freq;
        enum
        {
            DRIVER_SPI_MODE_0, DRIVER_SPI_MODE_1,
            DRIVER_SPI_MODE_2, DRIVER_SPI_MODE_3
        } mode;
        bool lsb;
    } spi;

    struct __attribute__((packed))
    {
        u32 clock;
    } timer;

    struct __attribute__((packed))
    {
        bool (*pin)  (void *ctx, u16 pin, u8 role, u8 pull);
        bool (*intr) (void *ctx, u16 intr, bool enable, u8 level);
        bool (*ack)  (void *ctx, u16 intr);
    } gpio;

    struct __attribute__((packed))
    {
        bool (*config)(void *ctx, u16 n, void (*f)(void),
                                  bool enable, u8 priority,
                                  bool edge, bool high);
        void (*wait)  (void *ctx);
    } pic;
};

struct file;

enum
{
    DRIVER_GPIO_OFF, DRIVER_GPIO_IN, DRIVER_GPIO_OUT, DRIVER_GPIO_EXTRA
};

enum
{
    DRIVER_GPIO_PULLOFF, DRIVER_GPIO_PULLUP, DRIVER_GPIO_PULLDOWN
};

enum
{
    DRIVER_GPIO_EDGE_H,  DRIVER_GPIO_EDGE_L,
    DRIVER_GPIO_LEVEL_H, DRIVER_GPIO_LEVEL_L, DRIVER_GPIO_DOUBLE
};

#define DRIVER_SPI_MAX 0

/* Driver and device structures */

struct __attribute__((packed)) driver
{
    void *init, (*clean)(void *);
    enum
    {
        DRIVER_API_GENERIC,
        DRIVER_API_BLOCK, DRIVER_API_STREAM,
        DRIVER_API_FS
    } api;
    enum
    {
        DRIVER_TYPE_GENERIC,
        DRIVER_TYPE_VIDEO, DRIVER_TYPE_AUDIO,
        DRIVER_TYPE_STORAGE, DRIVER_TYPE_FS,
        DRIVER_TYPE_TIMER, DRIVER_TYPE_SERIAL,
        DRIVER_TYPE_SPI, DRIVER_TYPE_GPIO,
        DRIVER_TYPE_PIC
    } type;
    struct __attribute__((packed))
    {
        bool (*get)(void *ctx, union config *cfg);
        bool (*set)(void *ctx, union config *cfg);
    } config;
    union __attribute__((packed))
    {
        struct __attribute__((packed))
        {
            bool (*read) (void *ctx, u8 *buffer, u32 block);
            bool (*write)(void *ctx, u8 *buffer, u32 block);
        } block;
        struct __attribute__((packed))
        {
            bool (*read)  (void *ctx, u8 *data);
            bool (*write) (void *ctx, u8 data);
        } stream;
        struct __attribute__((packed))
        {
            struct file * (*open) (void *ctx, char *path);
            struct file * (*close)(struct file *f);
            void          (*info) (struct file *f, size_t *size, s32 *files);
            struct file * (*index)(struct file *f, u32 index);
            bool          (*read) (struct file *f, u32 sector, u8 *buffer);
        } fs;
    } interface;
};

struct device
{
    struct driver *driver;
    void *context;
};

/* Devtree interface */

void _devtree_init(void);
void _devtree_clean(void);

#define DRIVER(x) _driver_##x
#define DECLARE_DRIVER(x) const struct driver DRIVER(x) =
#define INCLUDE_DRIVER(x) extern const struct driver DRIVER(x);

#define DEVICE(x) _device_##x
#define DECLARE_DEVICE(x) struct device DEVICE(x) = {0};
#define INCLUDE_DEVICE(x) extern struct device DEVICE(x);
#define INIT_DEVICE(x, drv, ...) \
{ \
    DEVICE(x).driver = (struct driver *)&DRIVER(drv); \
    if (DEVICE(x).driver->init) \
        ((void (*)(void **, ...))DEVICE(x).driver->init)( \
                                     &(DEVICE(x).context),##__VA_ARGS__); \
}
#define CONFIG_DEVICE(x, ...) \
{ \
    union config _cfg = {__VA_ARGS__}; \
    DEVICE(x).driver->config.set(DEVICE(x).context, &_cfg); \
}
#define CLEAN_DEVICE(x) \
{ \
    if (DEVICE(x).driver->clean) \
        DEVICE(x).driver->clean(DEVICE(x).context); \
}

#endif
