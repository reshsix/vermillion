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

#include <_types.h>

extern void *__drivers;
extern void *__drivers_c;

static __attribute__((used)) struct driver **_drivers =
    (struct driver **)&__drivers;
static __attribute__((used)) u32 _drivers_c =
    (u32)&__drivers_c;

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

/* Driver structure */

struct __attribute__((packed)) driver
{
    char *name;
    bool status;

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
        DRIVER_TYPE_SPI, DRIVER_TYPE_GPIO
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
    char *name;
    struct driver *driver;
    void *context;
};

struct devnode
{
    struct device cur;
    struct devnode *prev;
    struct devnode *next;
};

/* Driver interface */

#define driver_register(x) \
_Pragma("GCC push_options"); \
_Pragma("GCC optimize \"-fno-toplevel-reorder\""); \
static const volatile struct driver *_driver_##x \
__attribute__((used, section(".data.drivers"))) = &(x); \
_Pragma("GCC pop_options");

/* Devtree interface */

#include <stdlib.h>
#include <string.h>

void _devices_init(void);
void _devices_clean(void);

struct driver *driver_find(const char *name);
struct device *device_find(const char *name);

extern struct devnode _devtree_head;
extern struct devnode *_devtree_tail;

extern struct device *_devtree_logger;
void _devtree_log(const char *s);

#define DRIVER(x) driver_find(x)
#define DEVICE(x) device_find(x)

#define DEVICE_NEW(_name, _driver, ...) \
{ \
    _devtree_log("Creating device: "); \
    _devtree_log(_name); \
    _devtree_log(": "); \
    _devtree_log(_driver); \
    _devtree_log("("); \
    _devtree_log(#__VA_ARGS__); \
    _devtree_log("): "); \
\
    _devtree_tail->cur.driver = DRIVER(_driver); \
    if (_devtree_tail->cur.driver) \
    { \
        _devtree_tail->cur.name = _name; \
        if (_devtree_tail->cur.driver->init) \
        { \
            ((void (*)(void **, ...))_devtree_tail->cur.driver->init)( \
                &(_devtree_tail->cur.context),##__VA_ARGS__); \
            if (!(_devtree_tail->cur.context)) \
                _devtree_log("Failure"); \
        } \
\
        if (!(_devtree_tail->cur.driver->init) || _devtree_tail->cur.context) \
        { \
            _devtree_tail->next = calloc(1, sizeof(struct devnode)); \
            if (_devtree_tail->next) \
            { \
                _devtree_tail->next->prev = _devtree_tail; \
                _devtree_tail = _devtree_tail->next; \
                _devtree_log("Success"); \
            } \
            else \
                _devtree_log("Out of memory"); \
        } \
    } \
    else \
        _devtree_log("Driver not found"); \
\
    _devtree_log("\r\n"); \
}

#define DEVICE_CONFIG(name, ...) \
{ \
    union config _cfg = { __VA_ARGS__ }; \
    struct device *_dev = DEVICE(name); \
    if (_dev && _dev->driver && _dev->driver->config.set) \
        _dev->driver->config.set(_dev->context, &_cfg); \
}

#define DEVICE_LOGGER(name) \
{ \
    struct device *_dev = DEVICE(name); \
    if (_dev && _dev->driver && \
        _dev->driver->api == DRIVER_API_STREAM && \
        _dev->driver->interface.stream.write) \
    { \
        _devtree_logger = _dev; \
\
        _devtree_log("\r\nVermillion "); \
        _devtree_log(__VERMILLION__); \
        _devtree_log(" ("); \
        _devtree_log(__COMPILATION__); \
        _devtree_log(") on " name "\r\n\r\n"); \
    } \
}

#define DEVTREE_CLEANUP() \
{ \
    struct devnode *_cur = _devtree_tail; \
    while (_cur != NULL) \
    { \
        if (_cur->cur.driver->clean && _cur->cur.context) \
            _cur->cur.driver->clean(_cur->cur.context); \
\
        struct devnode *_prev = _cur->prev; \
        if (_prev) \
            free(_cur); \
        else \
            memset(_cur, 0, sizeof(struct devnode)); \
        _cur = _prev; \
    } \
    _devtree_tail = &_devtree_head; \
}

#endif
