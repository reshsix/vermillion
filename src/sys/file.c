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
#include <vermillion/util/types.h>

/* Path utils */

static char vrm_path_buffer[VRM_FILE_PATH_S] = {0};

extern bool
vrm_file_validate(const char *path)
{
    return (vrm_str_length(path) < VRM_FILE_PATH_S);
}

extern void
vrm_file_sanitize(char *path)
{
    vrm_path_buffer[0] = '/';
    if (path[0] == '/')
        vrm_str_copy(&(vrm_path_buffer[1]), &(path[1]), 0);
    else
        vrm_str_copy(&(vrm_path_buffer[1]), path, 0);

    char *next = vrm_path_buffer;
    do
    {
        uint32_t move = 0;
        next = vrm_str_find_l(next, '/');
        if (next)
        {
            while (next[1 + move] == '/')
                move++;

            if (move)
                vrm_str_copy(next, &(next[move]), 0);

            if (next[1] == '\0')
                next = NULL;
            else
                next = &(next[1]);
        }
    }
    while (next);

    uint32_t length = vrm_str_length(vrm_path_buffer);
    if (vrm_path_buffer[length - 1] == '/')
        vrm_path_buffer[length - 1] = '\0';

    if (vrm_path_buffer[0] == '\0')
        vrm_str_copy(vrm_path_buffer, "/", 0);

    vrm_str_copy(path, vrm_path_buffer, 0);
}

extern void
vrm_file_dirname(char *path)
{
    char *last = vrm_str_find_r(path, '/');
    if (last)
        last[0] = '\0';
    else
        vrm_str_copy(path, "/", 0);

    if (path[0] == '\0')
        vrm_str_copy(path, "/", 0);
}

extern void
vrm_file_basename(char *path)
{
    char *last = vrm_str_find_r(path, '/');
    if (last)
        vrm_str_copy(path, &(last[1]), 0);
}

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

#define FS_ROOT() \
((f->dev < dev_c) ? dev_l[f->dev].driver->root(dev_l[f->dev].context) : 0)
#define FS_CALL(name, ...) \
((f->dev < dev_c) ? dev_l[f->dev].driver->name(dev_l[f->dev].context, \
                                               ##__VA_ARGS__) : false)

static char vrm_file_buffer[VRM_FILE_PATH_S] = {0};

extern struct vrm_file *
vrm_file_open(uint8_t id, const char *path)
{
    struct vrm_file *f = vrm_mem_new(sizeof(struct vrm_file));

    bool success = false;
    if (f && vrm_file_validate(path))
    {
        vrm_mem_fill(f, 0, sizeof(struct vrm_file));
        success = true;

        vrm_str_copy(vrm_file_buffer, path, 0);
        vrm_file_sanitize(vrm_file_buffer);

        f->location = FS_ROOT();
        f->dir      = true;

        char name[256] = {0};
        char *state    = NULL;
        for (char *token = vrm_str_token(vrm_file_buffer, "/", &state);
                   token;
                   token = vrm_str_token(NULL,            "/", &state))
        {
            success   = false;
            f->parent = f->location;
            f->idx    = 0;

            uint32_t idx = 0;
            while (FS_CALL(walk, f->parent,  &(idx), &(f->dir), name,
                                 &(f->size), &(f->location)))
            {
                if (vrm_str_comp(name, token, 0) == 0)
                {
                    success = true;
                    break;
                }

                f->idx = idx;
            }
        }
    }

    if (success)
    {
        f->dev = id;
        if (!(f->dir) && f->size)
        {
            if (!FS_CALL(read, f->location, f->buffer, 0))
                f = vrm_mem_del(f);
        }
    }
    else
        f = vrm_mem_del(f);

    return f;
}

extern struct vrm_file *
vrm_file_close(struct vrm_file *f)
{
    if (f)
        vrm_file_flush(f);

    return vrm_mem_del(f);
}

extern bool
vrm_file_stat(struct vrm_file *f, bool *dir, uint32_t *size)
{
    bool ret = (f != NULL);

    if (ret)
    {
        if (dir)
            *dir  = f->dir;
        if (size)
            *size = f->size;
    }

    return ret;
}

extern bool
vrm_file_walk(struct vrm_file *f, uint32_t *idx,
              bool *dir, char *name, uint32_t *size)
{
    bool ret = false;

    if (f->dir)
        ret = FS_CALL(walk, f->location, idx, dir, name, size, NULL);

    return ret;
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
                if (!vrm_file_resize(f, needed))
                    bytes = 0;
        }

        while (bytes > 0 && f->pos < f->size)
        {
            if (f->pos / 0x200 != f->block)
            {
                if (f->flush)
                {
                    if (FS_CALL(write, f->location, f->buffer, f->block))
                        f->flush = false;
                    else
                        break;
                }

                f->block = f->pos / 0x200;
                if (!FS_CALL(read, f->location, f->buffer, f->block))
                    break;
            }

            uint32_t rel = f->pos % 0x200;
            uint32_t slice = (bytes < (0x200 - rel)) ? bytes : 0x200 - rel;
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

            ret    += slice;
            bytes  -= slice;
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
        f->flush = !(FS_CALL(write, f->location, f->buffer, f->block));

    return (f && !(f->flush));
}

extern bool
vrm_file_create(uint8_t id, const char *path, bool dir)
{
    bool ret = vrm_file_validate(path);

    if (ret)
    {
        struct vrm_file *check = vrm_file_open(id, path);

        if (!check)
        {
            vrm_str_copy(vrm_file_buffer, path, 0);
            vrm_file_sanitize(vrm_file_buffer);
            vrm_file_dirname(vrm_file_buffer);

            struct vrm_file *f = vrm_file_open(id, vrm_file_buffer);

            if (f && f->dir)
            {
                vrm_str_copy(vrm_file_buffer, path, 0);
                vrm_file_sanitize(vrm_file_buffer);
                vrm_file_basename(vrm_file_buffer);
                ret = FS_CALL(create, f->location, vrm_file_buffer, dir, 0, 0);
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

    struct vrm_file *f = vrm_file_open(id, path);
    if (f)
        ret = FS_CALL(remove, f->parent, f->idx, true);

    if (ret)
        vrm_mem_del(f);
    else
        vrm_file_close(f);

    return ret;
}

extern bool
vrm_file_resize(struct vrm_file *f, uint32_t size)
{
    bool ret = (f && !(f->dir));

    if (ret)
        ret = FS_CALL(resize, f->parent, f->idx, size, &(f->location));
    if (ret)
        f->size = size;

    return ret;
}

extern bool
vrm_file_move(struct vrm_file *f, const char *path)
{
    bool ret = vrm_file_validate(path);

    if (ret)
    {
        struct vrm_file *check = vrm_file_open(f->dev, path);

        if (!check)
        {
            vrm_str_copy(vrm_file_buffer, path, 0);
            vrm_file_sanitize(vrm_file_buffer);
            vrm_file_dirname(vrm_file_buffer);

            struct vrm_file *p = vrm_file_open(f->dev, vrm_file_buffer);

            if (p && p->dir)
            {
                vrm_str_copy(vrm_file_buffer, path, 0);
                vrm_file_sanitize(vrm_file_buffer);
                vrm_file_basename(vrm_file_buffer);
                ret = FS_CALL(remove, f->parent, f->idx, false) &&
                      FS_CALL(create, f->parent,
                              vrm_file_buffer, f->dir, f->location, f->size);
            }
            else
                ret = false;

            vrm_file_close(p);
        }
        else
            ret = false;

        vrm_file_close(check);
    }

    return ret;
}
