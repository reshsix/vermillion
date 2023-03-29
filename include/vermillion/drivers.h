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

extern struct driver *__drivers;
extern void *__drivers_c;

struct file;

enum
{
    DRIVER_SERIAL_CHAR_5B,
    DRIVER_SERIAL_CHAR_6B,
    DRIVER_SERIAL_CHAR_7B,
    DRIVER_SERIAL_CHAR_8B,
    DRIVER_SERIAL_CHAR_9B,
};

enum
{
    DRIVER_SERIAL_PARITY_NONE,
    DRIVER_SERIAL_PARITY_ODD,
    DRIVER_SERIAL_PARITY_EVEN
};

enum
{
    DRIVER_SERIAL_STOP_1B,
    DRIVER_SERIAL_STOP_1HB,
    DRIVER_SERIAL_STOP_2B
};

#define DRIVER_SPI_MAX 0

struct driver
{
    char *name;

    bool (*init)(void);
    void (*clean)(void);

    enum
    {
        DRIVER_TYPE_VIDEO, DRIVER_TYPE_AUDIO,
        DRIVER_TYPE_STORAGE, DRIVER_TYPE_FS, DRIVER_TYPE_LOADER,
        DRIVER_TYPE_GIC, DRIVER_TYPE_TIMER,
        DRIVER_TYPE_SERIAL, DRIVER_TYPE_SPI,
        DRIVER_TYPE_DUMMY
    } type;
    union
    {
        struct
        {
            void (*info)  (u16 *width, u16 *height);
            void (*update)(u8 *buffer, u16 x, u16 y, u16 w, u16 h);
            void (*clear) (void);
        } video;
        struct
        {
            void (*note)  (u16 freq, u16 duration);
            void (*sample)(u16 freq, u8 *data, size_t size);
        } audio;
        struct
        {
            bool (*read)(u8 *buffer, u32 block, u32 count);
        } storage;
        struct
        {
            struct file * (*open) (char *path);
            struct file * (*close)(struct file *f);
            void   (*info) (struct file *f, size_t *size, s32 *files);
            struct file * (*index)(struct file *f, u32 index);
            bool   (*read) (struct file *f, u32 sector, u8 *buffer);
        } fs;
        struct
        {
            u8 * (*prog)(const char *path, u32 *entry);
        } loader;
        struct
        {
            void (*config)(u16 n, void (*f)(void), bool enable, u8 priority);
            void (*wait)(void);
        } gic;
        struct
        {
            u32  (*clock) (void);
            void (*csleep)(const u32 n);
            void (*usleep)(const u32 n);
            void (*msleep)(const u32 n);
            void (*sleep) (const u32 n);
        } timer;
        struct
        {
            u8   (*ports) (void);
            bool (*config)(u8 port, u32 baud, u8 bits, u8 parity, u8 stop);
            u8   (*read)  (u8 port);
            void (*write) (u8 port, u16 data);
        } serial;
        struct
        {
            bool (*config)(u32 freq, u8 mode, bool lsb);
            u8   (*transfer)(u8 x);
        } spi;
    } routines;
};

#define driver_register(x) \
static const volatile struct driver *_driver_##x \
__attribute__((used, section(".data.drivers"))) = &(x);

void _drivers_init(u8 type);
void _drivers_clean(u8 type);

u32 driver_count(u8 type);
struct driver *driver_find(u8 type, u32 index);

#endif
