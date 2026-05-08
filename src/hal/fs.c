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

#include <hal/fs.h>

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

static char fs_buffer[PATH_MAX] = {0};

extern struct fs_file *
fs_open(dev_fs *df, const char *path)
{
    struct fs_file *ret = mem_new(sizeof(struct fs_file));

    bool success = false;
    if (ret && path_validate(path))
    {
        success = false;

        str_copy(fs_buffer, path, 0);
        path_cleanup(fs_buffer);

        char *state = NULL;

        void *cur = root(df);
        if (cur)
        {
            char name[255] = {0};

            success = true;
            for (char *token = str_token(fs_buffer, "/", &state); token;
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
fs_stat(struct fs_file *f, bool *dir, u32 *size)
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
fs_walk(struct fs_file *f, void *state, bool *dir, char *name, u32 *size)
{
    return walk(f->df, f->cache, state, dir, name, size);
}

extern bool
fs_seek(struct fs_file *f, enum fs_seek seek, s32 pos)
{
    bool ret = (f && !(f->dir));

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
    bool ret = (f && !(f->dir));

    if (ret)
        *pos = f->pos;

    return ret;
}

static u32
fs_rw(struct fs_file *f, void *buffer, u32 bytes, bool w)
{
    u32 ret = 0;

    if (f && !(f->dir))
    {
        if (w)
        {
            u32 needed = f->pos + bytes;
            if (needed > f->size)
                fs_resize(f, needed);
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
        f->flush = !(write(f->df, f->cache, f->buffer, f->block));

    return (f && !(f->flush));
}

extern bool
fs_resize(struct fs_file *f, u32 size)
{
    bool ret = (f && !(f->dir));

    if (ret)
        ret = resize(f->df, f->cache, size);
    if (ret)
        f->size = size;

    return ret;
}

extern bool
fs_create(dev_fs *df, const char *path, bool dir)
{
    bool ret = path_validate(path);

    if (ret)
    {
        struct fs_file *check = fs_open(df, path);

        if (!check)
        {
            str_copy(fs_buffer, path, 0);
            path_dirname(fs_buffer);

            struct fs_file *f = fs_open(df, fs_buffer);

            if (f && f->dir)
            {
                str_copy(fs_buffer, path, 0);
                path_filename(fs_buffer);
                ret = create(df, f->cache, &(fs_buffer), dir);
            }
            else
                ret = false;

            fs_close(f);
        }
        else
            ret = false;

        fs_close(check);
    }

    return ret;
}

extern bool
fs_remove(dev_fs *df, const char *path)
{
    bool ret = false;

    struct fs_file *f = fs_open(df, path);

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
        fs_close(f);

    return ret;
}
