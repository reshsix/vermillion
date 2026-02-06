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

#include <general/types.h>

#include <hal/classes/fs.h>

#include <system/disk.h>

static dev_fs *disk = NULL;

/* For devtree usage */

extern void
disk_config(dev_fs *f)
{
    disk = f;
}

/* For external usage */

extern disk_f *
disk_open(const char *path)
{
    return fs_open(disk, path);
}

extern disk_f *
disk_close(disk_f *f)
{
    return fs_close(f);
}

extern bool
disk_stat(disk_f *f, bool *dir, char **name, u32 *size)
{
    bool ret = false;

    enum fs_type type2 = FS_REGULAR;
    ret = fs_stat(f, &type2, name, size);
    if (ret && dir)
        *dir = (type2 == FS_DIRECTORY);

    return ret;
}

extern bool
disk_walk(disk_f *f, u32 index, bool *dir, char **name, u32 *size)
{
    bool ret = false;

    enum fs_type type2 = FS_REGULAR;
    ret = fs_walk(f, index, &type2, name, size);
    if (ret && dir)
        *dir = (type2 == FS_DIRECTORY);

    return ret;
}

extern bool
disk_seek(disk_f *f, u32 pos)
{
    return fs_seek(f, FS_START, pos);
}

extern bool
disk_tell(disk_f *f, u32 *pos)
{
    return fs_tell(f, pos);
}

extern u32
disk_read(disk_f *f, void *buffer, u32 bytes)
{
    return fs_read(f, buffer, bytes);
}

extern u32
disk_write(disk_f *f, void *buffer, u32 bytes)
{
    return fs_write(f, buffer, bytes);
}

extern bool
disk_flush(disk_f *f)
{
    return fs_flush(f);
}

extern bool
disk_rename(disk_f *f, const char *name)
{
    return fs_rename(f, name);
}

extern bool
disk_resize(disk_f *f, u32 size)
{
    return fs_resize(f, size);
}

extern bool
disk_mkfile(const char *path)
{
    return fs_mkfile(disk, path);
}

extern bool
disk_mkdir(const char *path)
{
    return fs_mkdir(disk, path);
}

extern bool
disk_remove(const char *path)
{
    return fs_remove(disk, path);
}
