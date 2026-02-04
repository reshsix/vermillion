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
#include <general/mem.h>
#include <general/str.h>
#include <general/path.h>

#include <hal/classes/fs.h>

extern struct fs_file *
fs_open(dev_fs *df, const char *path)
{
    struct fs_file *ret = mem_new(sizeof(struct fs_file));

    char *cpath = NULL;
    if (ret)
    {
        if (path)
            cpath = path_cleanup(path);
        if (!cpath)
            ret = mem_del(ret);
    }

    if (ret)
    {
        bool success = false;
        success = block_write((dev_block *)df, FS_FIND,  &cpath,        0) &&
                  block_read ((dev_block *)df, FS_CACHE, &(ret->cache), 0) &&
                  block_read ((dev_block *)df, FS_TYPE,  &(ret->type),  0) &&
                  block_read ((dev_block *)df, FS_NAME,  &(ret->name),  0) &&
                  block_read ((dev_block *)df, FS_SIZE,  &(ret->size),  0) &&
                  block_stat ((dev_block *)df, BLOCK_COMMON,
                              &(ret->width), NULL);
        if (!success)
            ret = mem_del(ret);
    }

    if (ret)
    {
        ret->df = df;
        if (ret->type == FS_REGULAR)
        {
            ret->buffer = mem_new(ret->width);
            if (ret->buffer)
                if (!block_read((dev_block *)df, BLOCK_COMMON, ret->buffer, 0))
                    ret->buffer = mem_del(ret->buffer);

            if (!(ret->buffer))
                ret = mem_del(ret);
        }
    }

    return ret;
}

extern struct fs_file *
fs_close(struct fs_file *f)
{
    if (f)
    {
        fs_flush(f);
        mem_del(f->buffer);
    }

    return mem_del(f);
}

extern bool
fs_stat(struct fs_file *f, enum fs_type *type, char **name, u32 *size)
{
    bool ret = (f != NULL);

    if (ret)
    {
        if (type)
            *type = f->type;
        if (name)
            *name = f->name;
        if (size)
            *size = f->size;
    }

    return ret;
}

extern bool
fs_walk(struct fs_file *f, u32 index,
        enum fs_type *type, char **name, u32 *size)
{
    bool ret = (f && f->type == FS_DIRECTORY) &&
               block_write((dev_block *)f->df, FS_SWITCH, &(f->cache), 0) &&
               block_write((dev_block *)f->df, FS_WALK, &index, 0);

    enum fs_type t = FS_REGULAR;
    char *n = NULL;
    u32 s = 0;

    if (ret)
        ret = block_read((dev_block *)f->df, FS_TYPE, &t, 0) &&
              block_read((dev_block *)f->df, FS_NAME, &n, 0) &&
              block_read((dev_block *)f->df, FS_SIZE, &s, 0);

    if (ret)
    {
        if (type)
            *type = t;
        if (name)
            *name = n;
        if (size)
            *size = s;
    }

    return ret;
}

extern bool
fs_seek(struct fs_file *f, enum fs_seek seek, s32 pos)
{
    bool ret = (f && f->type == FS_REGULAR);

    if (ret)
    {
        switch (seek)
        {
            case FS_START:
                ret = (pos >= 0 && (u32)pos <= f->size);
                if (ret)
                    f->pos = pos;
                break;
            case FS_CURRENT:
                ret = (f->pos + pos <= f->size && f->pos + pos > 0);
                if (ret)
                    f->pos += pos;
                break;
            case FS_END:
                ret = (f->size + pos <= f->size && f->size + pos > 0);
                if (ret)
                    f->pos = f->size + pos;
                break;
        }
    }

    return ret;
}

extern bool
fs_tell(struct fs_file *f, u32 *pos)
{
    bool ret = (f && f->type == FS_REGULAR);

    if (ret)
        *pos = f->pos;

    return ret;
}

static u32
fs_rw(struct fs_file *f, void *buffer, u32 bytes, bool write)
{
    u32 ret = 0;

    if (f && f->type == FS_REGULAR)
    {
        if (write)
        {
            u32 needed = f->pos + bytes;
            if (needed > f->size)
            {
                (void)
                (block_write((dev_block *)f->df, FS_SWITCH, &(f->cache), 0) &&
                 block_write((dev_block *)f->df, FS_SIZE, &needed, 0));
            }
        }

        while (bytes > 0 && f->pos < f->size)
        {
            if (f->pos / f->width != f->block)
            {
                if (!(block_write((dev_block *)f->df, FS_SWITCH,
                                  &(f->cache), 0)))
                    break;

                if (f->flush)
                {
                    if (block_write((dev_block *)f->df, BLOCK_COMMON,
                                    f->buffer, f->block))
                        f->flush = false;
                    else
                        break;
                }

                f->block = f->pos / f->width;
                if (!block_read((dev_block *)f->df, BLOCK_COMMON,
                                f->buffer, f->block))
                    break;
            }

            u32 rel = f->pos % f->width;
            u32 slice = (bytes < (f->width - rel)) ? bytes : f->width - rel;
            if (f->pos + slice >= f->size)
                slice = f->size - f->pos;

            if (write)
            {
                u8 *source = buffer;
                mem_copy(&(f->buffer[rel]), &(source[ret]), slice);
                f->flush = true;
            }
            else
            {
                u8 *dest = buffer;
                mem_copy(&(dest[ret]), &(f->buffer[rel]), slice);
            }

            ret += slice;
            bytes -= slice;
            f->pos += slice;
        }
    }

    return ret;
}

extern u32
fs_read(struct fs_file *f, void *buffer, u32 bytes)
{
    return fs_rw(f, buffer, bytes, false);
}

extern u32
fs_write(struct fs_file *f, void *buffer, u32 bytes)
{
    return fs_rw(f, buffer, bytes, true);
}

extern bool
fs_flush(struct fs_file *f)
{
    if (f && f->flush)
        f->flush = !(block_write((dev_block *)f->df, FS_SWITCH,
                                  &(f->cache), 0) &&
                      block_write((dev_block *)f->df, BLOCK_COMMON,
                                  f->buffer, f->block));

    return (f && !(f->flush));
}

extern bool
fs_rename(struct fs_file *f, const char *name)
{
    bool ret = (f && block_write((dev_block *)f->df, FS_SWITCH,
                                 &(f->cache), 0));

    if (ret)
        ret = block_write((dev_block *)f->df, FS_NAME, &name, 0) &&
              block_read((dev_block *)f->df, FS_NAME, &(f->name), 0) &&
              block_read((dev_block *)f->df, FS_CACHE, &(f->cache), 0) &&
              block_read((dev_block *)f->df, FS_NAME,  &(f->name),  0);

    return ret;
}

extern bool
fs_resize(struct fs_file *f, u32 size)
{
    bool ret = (f && f->type == FS_REGULAR) &&
               block_write((dev_block *)f->df, FS_SWITCH, &(f->cache), 0);

    if (ret)
        ret = block_write((dev_block *)f->df, FS_SIZE, &size, 0) &&
              block_read((dev_block *)f->df, FS_SIZE, &(f->size), 0);

    return ret;
}

static bool
fs_pathop(dev_fs *df, enum fs_index op, const char *path)
{
    bool ret = (df && path);

    char *cpath = NULL;
    if (ret)
        cpath = path_cleanup(path);

    if (ret && cpath)
        ret = block_write((dev_block *)df, op, &cpath, 0);
    mem_del(cpath);

    return ret;
}

extern bool
fs_mkfile(dev_fs *df, const char *path)
{
    return fs_pathop(df, FS_MKFILE, path);
}

extern bool
fs_mkdir(dev_fs *df, const char *path)
{
    return fs_pathop(df, FS_MKDIR, path);
}

extern bool
fs_remove(dev_fs *df, const char *path)
{
    return fs_pathop(df, FS_REMOVE, path);
}
