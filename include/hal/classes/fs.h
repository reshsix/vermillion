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

#include <general/types.h>

#include <hal/block.h>

enum fs_type
{
    FS_REGULAR,
    FS_DIRECTORY
};

enum fs_index
{
    FS_FIND = BLOCK_COMMON + 1,
    FS_CACHE,
    FS_SWITCH,
    FS_WALK,

    FS_TYPE, FS_NAME, FS_SIZE,

    FS_MKFILE, FS_MKDIR, FS_REMOVE,
};

typedef drv_block drv_fs;
typedef dev_block dev_fs;

struct fs_file
{
    dev_fs *df;
    void *cache;

    u8 *buffer;
    u32 width, block;
    bool flush;

    enum fs_type type;
    char *name;
    u32 size;

    u32 pos;
};

enum fs_seek
{
    FS_START,
    FS_CURRENT,
    FS_END
};

struct fs_file *fs_open(dev_fs *df, const char *path);
struct fs_file *fs_close(struct fs_file *f);

bool fs_stat(struct fs_file *f, enum fs_type *type, char **name, u32 *size);
bool fs_walk(struct fs_file *f, u32 index,
             enum fs_type *type, char **name, u32 *size);

bool fs_seek(struct fs_file *f, enum fs_seek seek, s32 pos);
bool fs_tell(struct fs_file *f, u32 *pos);

u32 fs_read(struct fs_file *f, void *buffer, u32 bytes);
u32 fs_write(struct fs_file *f, void *buffer, u32 bytes);
bool fs_flush(struct fs_file *f);

bool fs_rename(struct fs_file *f, const char *name);
bool fs_resize(struct fs_file *f, u32 size);

bool fs_mkfile(dev_fs *df, const char *path);
bool fs_mkdir(dev_fs *df, const char *path);
bool fs_remove(dev_fs *df, const char *path);
