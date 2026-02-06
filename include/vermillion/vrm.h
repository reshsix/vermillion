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

#pragma once

#include <stdint.h>

#ifdef VERMILLION_INTERNALS
typedef struct fs_file vrm_disk_f;
#else
typedef struct vrm_disk_f vrm_disk_f;
#endif

struct vrm
{
    struct
    {
        char (*read0)(void);
        char (*read1)(void);
        void (*write0)(char c);
        void (*write1)(char c);
    } comm;

    struct
    {
        vrm_disk_f * (*open)(const char *path);
        vrm_disk_f * (*close)(vrm_disk_f *f);

        bool (*stat)(vrm_disk_f *f,
                     bool *dir, char **name, uint32_t *size);
        bool (*walk)(vrm_disk_f *f, uint32_t index,
                     bool *dir, char **name, uint32_t *size);

        bool (*seek)(vrm_disk_f *f, uint32_t  pos);
        bool (*tell)(vrm_disk_f *f, uint32_t *pos);

        uint32_t (*read)(vrm_disk_f *f, void *buffer, uint32_t bytes);
        uint32_t (*write)(vrm_disk_f *f, void *buffer, uint32_t bytes);
        bool (*flush)(vrm_disk_f *f);

        bool (*rename)(vrm_disk_f *f, const char *name);
        bool (*resize)(vrm_disk_f *f, uint32_t size);

        bool (*mkfile)(const char *path);
        bool (*mkdir)(const char *path);
        bool (*remove)(const char *path);
    } disk;

    struct
    {
        u8 * (*prog)(const char *path, uint32_t *entry);
    } load;

    struct
    {
        bool (*event0)(void (*handler)(void *), void *arg, u8 jiffies);
        bool (*event1)(void (*handler)(void *), void *arg, u8 jiffies);
        void (*sleep0)(u8 jiffies);
        void (*sleep1)(u8 jiffies);
        u64  (*clock0)(void);
        u64  (*clock1)(void);
    } time;
};
