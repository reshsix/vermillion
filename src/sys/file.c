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

#include <general/types.h>
#include <general/mem.h>
#include <general/str.h>
#include <general/path.h>

#define VERMILLION_INTERNALS
#include <vermillion/sys/file.h>

/* Devtree setup */

static dev_fs *dev_l = NULL;
static u8 dev_c = 0;

extern void
file_setup(dev_fs *list, u8 count)
{
    dev_l = list;
    dev_c = count;
}

/* Driver calls */

static void *
root(dev_fs *df)
{
    return df->driver->root(df->context);
}

static void *
walk(dev_fs *df, void *parent, void *entry,
     bool *dir, char *name, u32 *size)
{
    return df->driver->walk(df->context, parent, entry, dir, name, size);
}

static bool
read(dev_fs *df, void *entry, void *buf, u32 fs)
{
    bool ret = (df != NULL);

    if (ret)
        ret = df->driver->read(df->context, entry, buf, fs);

    return ret;
}

static bool
write(dev_fs *df, void *entry, void *buf, u32 fs)
{
    bool ret = (df != NULL);

    if (ret)
        ret = df->driver->write(df->context, entry, buf, fs);

    return ret;
}

static bool
resize(dev_fs *df, void *entry, u32 size)
{
    bool ret = (df != NULL);

    if (ret)
        ret = df->driver->resize(df->context, entry, size);

    return ret;
}

static bool
create(dev_fs *df, void *parent, const char *name, bool dir)
{
    bool ret = (df != NULL);

    if (ret)
        ret = df->driver->create(df->context, parent, name, dir);

    return ret;
}

static bool
remove(dev_fs *df, void *entry)
{
    bool ret = (df != NULL);

    if (ret)
        ret = df->driver->remove(df->context, entry);

    return ret;
}

static char file_buffer[PATH_MAX] = {0};

static void *
file_dev(u8 id)
{
    return (id < dev_c) ? &(dev_l[id]) : NULL;
}

extern struct vrm_file *
file_open(u8 id, const char *path)
{
    struct vrm_file *ret = mem_new(sizeof(struct vrm_file));

    dev_fs *df = file_dev(id);

    bool success = false;
    if (ret && path_validate(path))
    {
        success = false;

        str_copy(file_buffer, path, 0);
        path_cleanup(file_buffer);

        char *state = NULL;

        void *cur = root(df);
        if (cur)
        {
            char name[255] = {0};

            success = true;
            for (char *token = str_token(file_buffer, "/", &state); token;
                       token = str_token(NULL,      "/", &state))
            {
                success = false;
                while (true)
                {
                    ret->cache = walk(df, cur, ret->cache,
                                      &(ret->dir), name, &(ret->size));
                    if (ret->cache)
                    {
                        if (str_comp(name, token, 0) == 0)
                        {
                            success = true;
                            cur = ret->cache;
                            ret->cache = NULL;
                            break;
                        }
                    }
                    else
                        break;
                }
            }

            if (success)
                ret->cache = cur;
        }
    }

    if (success)
    {
        ret->df = df;

        if (!(ret->dir))
        {
            ret->width = 0x200;
            ret->buffer = mem_new(ret->width);
            if (ret->buffer)
                if (!read(df, ret->cache, ret->buffer, 0))
                    ret->buffer = mem_del(ret->buffer);

            if (!(ret->buffer))
                ret = mem_del(ret);
        }
    }
    else
        ret = mem_del(ret);

    return ret;
}

extern struct vrm_file *
file_close(struct vrm_file *f)
{
    if (f)
    {
        file_flush(f);
        mem_del(f->buffer);
    }

    return mem_del(f);
}

extern bool
file_stat(struct vrm_file *f, bool *dir, u32 *size)
{
    bool ret = (f != NULL);

    if (ret)
    {
        if (dir)
            *dir = f->dir;
        if (size)
            *size = f->size;
    }

    return ret;
}

extern void *
file_walk(struct vrm_file *f, void *state, bool *dir, char *name, u32 *size)
{
    return walk(f->df, f->cache, state, dir, name, size);
}

extern bool
file_seek(struct vrm_file *f, u32 pos)
{
    bool ret = (f && !(f->dir));

    if (ret)
    {
        ret = (pos <= f->size);
        if (ret)
            f->pos = pos;
    }

    return ret;
}

extern bool
file_tell(struct vrm_file *f, u32 *pos)
{
    bool ret = (f && !(f->dir));

    if (ret)
        *pos = f->pos;

    return ret;
}

static u32
file_rw(struct vrm_file *f, void *buffer, u32 bytes, bool w)
{
    u32 ret = 0;

    if (f && !(f->dir))
    {
        if (w)
        {
            u32 needed = f->pos + bytes;
            if (needed > f->size)
                file_resize(f, needed);
        }

        while (bytes > 0 && f->pos < f->size)
        {
            if (f->pos / f->width != f->block)
            {
                if (f->flush)
                {
                    if (write(f->df, f->cache, f->buffer, f->block))
                        f->flush = false;
                    else
                        break;
                }

                f->block = f->pos / f->width;
                if (!read(f->df, f->cache, f->buffer, f->block))
                    break;
            }

            u32 rel = f->pos % f->width;
            u32 slice = (bytes < (f->width - rel)) ? bytes : f->width - rel;
            if (f->pos + slice >= f->size)
                slice = f->size - f->pos;

            if (w)
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
file_read(struct vrm_file *f, void *buffer, u32 bytes)
{
    return file_rw(f, buffer, bytes, false);
}

extern u32
file_write(struct vrm_file *f, void *buffer, u32 bytes)
{
    return file_rw(f, buffer, bytes, true);
}

extern bool
file_flush(struct vrm_file *f)
{
    if (f && f->flush)
        f->flush = !(write(f->df, f->cache, f->buffer, f->block));

    return (f && !(f->flush));
}

extern bool
file_resize(struct vrm_file *f, u32 size)
{
    bool ret = (f && !(f->dir));

    if (ret)
        ret = resize(f->df, f->cache, size);
    if (ret)
        f->size = size;

    return ret;
}

extern bool
file_create(u8 id, const char *path, bool dir)
{
    bool ret = path_validate(path);

    dev_fs *df = file_dev(id);
    if (ret)
    {
        struct vrm_file *check = file_open(id, path);

        if (!check)
        {
            str_copy(file_buffer, path, 0);
            path_dirname(file_buffer);

            struct vrm_file *f = file_open(id, file_buffer);

            if (f && f->dir)
            {
                str_copy(file_buffer, path, 0);
                path_filename(file_buffer);
                ret = create(df, f->cache, file_buffer, dir);
            }
            else
                ret = false;

            file_close(f);
        }
        else
            ret = false;

        file_close(check);
    }

    return ret;
}

extern bool
file_remove(u8 id, const char *path)
{
    bool ret = false;

    dev_fs *df = file_dev(id);

    struct vrm_file *f = file_open(id, path);
    if (f)
    {
        if (f->dir)
        {
            ret = true;

            void *cur = NULL;
            for (size_t i = 0; i < 3; i++)
            {
                cur = walk(df, f, cur, NULL, NULL, NULL);
                if (i >= 2 && cur)
                    ret = false;
            }

            if (ret)
                ret = remove(df, f);
        }
        else
            ret = remove(df, f->cache);
    }

    if (ret)
    {
        mem_del(f->buffer);
        mem_del(f);
    }
    else
        file_close(f);

    return ret;
}

/* ABI definitions */

static struct vrm_file_v1 v1 =
{
    .open   = file_open,   .close  = file_close,
    .stat   = file_stat,   .walk   = file_walk,
    .seek   = file_seek,   .tell   = file_tell,
    .read   = file_read,   .write  = file_write,
    .flush  = file_flush,
    .resize = file_resize,
    .create = file_create, .remove = file_remove
};

extern void *
file_system(u8 version)
{
    void *ret = NULL;

    switch (version)
    {
        case VRM_FILE_V1:
            ret = &v1;
            break;
    }

    return ret;
}
