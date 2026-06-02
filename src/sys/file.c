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

#define VERMILLION_INTERNALS
#include <vermillion/sys/file.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/str.h>
#include <vermillion/util/path.h>
#include <vermillion/util/types.h>

/* Devtree setup */

static dev_fs *dev_l = NULL;
static uint8_t dev_c = 0;

extern void
file_setup(dev_fs *list, uint8_t count)
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
     bool *dir, char *name, uint32_t *size)
{
    return df->driver->walk(df->context, parent, entry, dir, name, size);
}

static bool
read(dev_fs *df, void *entry, void *buf, uint32_t fs)
{
    bool ret = (df != NULL);

    if (ret)
        ret = df->driver->read(df->context, entry, buf, fs);

    return ret;
}

static bool
write(dev_fs *df, void *entry, void *buf, uint32_t fs)
{
    bool ret = (df != NULL);

    if (ret)
        ret = df->driver->write(df->context, entry, buf, fs);

    return ret;
}

static bool
resize(dev_fs *df, void *entry, uint32_t size)
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

static char vrm_file_buffer[VRM_PATH_MAX] = {0};

static void *
file_dev(uint8_t id)
{
    return (id < dev_c) ? &(dev_l[id]) : NULL;
}

extern struct vrm_file *
vrm_file_open(uint8_t id, const char *path)
{
    struct vrm_file *ret = vrm_mem_new(sizeof(struct vrm_file));

    dev_fs *df = file_dev(id);

    bool success = false;
    if (ret && vrm_path_validate(path))
    {
        vrm_mem_fill(ret, 0, sizeof(struct vrm_file));
        success = false;

        vrm_str_copy(vrm_file_buffer, path, 0);
        vrm_path_cleanup(vrm_file_buffer);

        char *state = NULL;

        void *cur = root(df);
        if (cur)
        {
            char name[255] = {0};
            success = true;

            ret->dir = true;
            for (char *token = vrm_str_token(vrm_file_buffer, "/", &state);
                       token;
                       token = vrm_str_token(NULL,        "/", &state))
            {
                success = false;
                while (true)
                {
                    ret->cache = walk(df, cur, ret->cache,
                                      &(ret->dir), name, &(ret->size));
                    if (ret->cache)
                    {
                        if (vrm_str_comp(name, token, 0) == 0)
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
            ret->buffer = vrm_mem_new(ret->width);
            if (ret->buffer)
            {
                if (!read(df, ret->cache, ret->buffer, 0))
                    ret->buffer = vrm_mem_del(ret->buffer);
            }

            if (!(ret->buffer))
                ret = vrm_mem_del(ret);
        }
    }
    else
        ret = vrm_mem_del(ret);

    return ret;
}

extern struct vrm_file *
vrm_file_close(struct vrm_file *f)
{
    if (f)
    {
        vrm_file_flush(f);
        vrm_mem_del(f->buffer);
    }

    return vrm_mem_del(f);
}

extern bool
vrm_file_stat(struct vrm_file *f, bool *dir, uint32_t *size)
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
vrm_file_walk(struct vrm_file *f, void *state,
          bool *dir, char *name, uint32_t *size)
{
    return walk(f->df, f->cache, state, dir, name, size);
}

extern bool
vrm_file_seek(struct vrm_file *f, uint32_t pos)
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
vrm_file_tell(struct vrm_file *f, uint32_t *pos)
{
    bool ret = (f && !(f->dir));

    if (ret)
        *pos = f->pos;

    return ret;
}

static uint32_t
vrm_file_rw(struct vrm_file *f, void *buffer, uint32_t bytes, bool w)
{
    uint32_t ret = 0;

    if (f && !(f->dir))
    {
        if (w)
        {
            uint32_t needed = f->pos + bytes;
            if (needed > f->size)
                vrm_file_resize(f, needed);
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

            uint32_t rel = f->pos % f->width;
            uint32_t slice = (bytes < (f->width - rel)) ? bytes :
                                                          f->width - rel;
            if (f->pos + slice >= f->size)
                slice = f->size - f->pos;

            if (w)
            {
                uint8_t *source = buffer;
                vrm_mem_copy(&(f->buffer[rel]), &(source[ret]), slice);
                f->flush = true;
            }
            else
            {
                uint8_t *dest = buffer;
                vrm_mem_copy(&(dest[ret]), &(f->buffer[rel]), slice);
            }

            ret += slice;
            bytes -= slice;
            f->pos += slice;
        }
    }

    return ret;
}

extern uint32_t
vrm_file_read(struct vrm_file *f, void *buffer, uint32_t bytes)
{
    return vrm_file_rw(f, buffer, bytes, false);
}

extern uint32_t
vrm_file_write(struct vrm_file *f, const void *buffer, uint32_t bytes)
{
    return vrm_file_rw(f, (void *)buffer, bytes, true);
}

extern bool
vrm_file_flush(struct vrm_file *f)
{
    if (f && f->flush)
        f->flush = !(write(f->df, f->cache, f->buffer, f->block));

    return (f && !(f->flush));
}

extern bool
vrm_file_resize(struct vrm_file *f, uint32_t size)
{
    bool ret = (f && !(f->dir));

    if (ret)
        ret = resize(f->df, f->cache, size);
    if (ret)
        f->size = size;

    return ret;
}

extern bool
vrm_file_create(uint8_t id, const char *path, bool dir)
{
    bool ret = vrm_path_validate(path);

    dev_fs *df = file_dev(id);
    if (ret)
    {
        struct vrm_file *check = vrm_file_open(id, path);

        if (!check)
        {
            vrm_str_copy(vrm_file_buffer, path, 0);
            vrm_path_dirname(vrm_file_buffer);

            struct vrm_file *f = vrm_file_open(id, vrm_file_buffer);

            if (f && f->dir)
            {
                vrm_str_copy(vrm_file_buffer, path, 0);
                vrm_path_filename(vrm_file_buffer);
                ret = create(df, f->cache, vrm_file_buffer, dir);
            }
            else
                ret = false;

            vrm_file_close(f);
        }
        else
            ret = false;

        vrm_file_close(check);
    }

    return ret;
}

extern bool
vrm_file_remove(uint8_t id, const char *path)
{
    bool ret = false;

    dev_fs *df = file_dev(id);

    struct vrm_file *f = vrm_file_open(id, path);
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
        vrm_mem_del(f->buffer);
        vrm_mem_del(f);
    }
    else
        vrm_file_close(f);

    return ret;
}
