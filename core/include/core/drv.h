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

#ifndef CORE_DRV_H
#define CORE_DRV_H

#include <core/types.h>

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
        u32  (*open) (void *ctx, char *path);
        u32  (*close)(void *ctx, u32 idx);
        void (*info) (void *ctx, u32 idx, size_t *size, s32 *files);
        u32  (*index)(void *ctx, u32 idx, u32 sub);
    } fs;
};

#define DRIVER_SPI_MAX 0

/* Driver structures */

#define _drv_typedef_block(x) \
    typedef struct __attribute__((packed)) \
    { \
        void *init, (*clean)(void *); \
        struct __attribute__((packed)) \
        { \
            bool (*get)(void *ctx, union config *cfg); \
            bool (*set)(void *ctx, union config *cfg); \
        } config; \
        bool (*stat) (void *ctx, u32 idx, u32 *width, u32 *length); \
        bool (*read) (void *ctx, u32 idx, void *buffer, u32 block); \
        bool (*write)(void *ctx, u32 idx, void *buffer, u32 block); \
    } drv_##x; \

#define _drv_typedef_stream(x) \
    typedef struct __attribute__((packed)) \
    { \
        void *init, (*clean)(void *); \
        struct __attribute__((packed)) \
        { \
            bool (*get)(void *ctx, union config *cfg); \
            bool (*set)(void *ctx, union config *cfg); \
        } config; \
        bool (*stat) (void *ctx, u32 idx, u32 *width); \
        bool (*read)  (void *ctx, u32 idx, void *data); \
        bool (*write) (void *ctx, u32 idx, void *data); \
    } drv_##x; \

#define drv_typedef(archetype, x) _drv_typedef_##archetype(x)

#define drv(x) _driver_##x
#define drv_decl(type, x) const drv_##type drv(x) =
#define drv_incl(type, x) extern const drv_##type drv(x);

#endif
