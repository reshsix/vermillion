/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <general/types.h>

typedef struct
{
    void *init, (*clean)(void *);
    void *  (*root)(void *ctx);
    void *  (*walk)(void *ctx, void *parent, void *entry,
                    bool *dir, char *name, u32 *size);
    bool    (*read)(void *ctx, void *entry, u8 *data, u32 block);
    bool   (*write)(void *ctx, void *entry, u8 *data, u32 block);
    bool  (*resize)(void *ctx, void *entry, u32 size);
    bool  (*create)(void *ctx, void *parent, const char *name, bool dir);
    bool  (*remove)(void *ctx, void *entry);
} drv_fs;

typedef struct
{
    const drv_fs *driver;
    void *context;
} dev_fs;

struct fs_file
{
    dev_fs *df;
    void *cache;

    u8 *buffer;
    u32 width, block;
    bool flush;

    bool dir;
    u32 size;

    u32 pos;
};

/* For internal usage */

void fs_setup(dev_fs *list, u8 count);

/* For external usage */

struct fs_file *fs_open(u8 id, const char *path);
struct fs_file *fs_close(struct fs_file *f);

bool fs_stat(struct fs_file *f, bool *dir, u32 *size);
void *fs_walk(struct fs_file *f, void *state, bool *dir, char *name, u32 *size);

bool fs_seek(struct fs_file *f, u32 pos);
bool fs_tell(struct fs_file *f, u32 *pos);

u32 fs_read(struct fs_file *f, void *buffer, u32 bytes);
u32 fs_write(struct fs_file *f, void *buffer, u32 bytes);
bool fs_flush(struct fs_file *f);

bool fs_resize(struct fs_file *f, u32 size);

bool fs_create(u8 id, const char *path, bool dir);
bool fs_remove(u8 id, const char *path);
