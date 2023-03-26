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

#include <errno.h>
#include <stdio.h>
#include <types.h>
#include <stdlib.h>
#include <string.h>

#include <interface/storage.h>

typedef struct _FILE
{
    size_t position;
    int error;
    bool eof;

    struct file *file;
    u8 cache[0x200];
    u32 cachedsect;
    bool cached;

} FILE;

/* File access */

extern int
fclose(FILE *f)
{
    int ret = -1;

    if (f)
    {
        storage_close(f->file);
        free(f);
        ret = 0;
    }

    return ret;
}

extern FILE *
fopen(const char *path, const char *mode)
{
    FILE *ret = NULL;

    if (strcmp(mode, "r") && strcmp(mode, "rb"))
        errno = EROFS;
    else
        ret = calloc(1, sizeof(FILE));

    if (ret)
    {
        ret->file = storage_open((char*)path);
        if (!(ret->file))
        {
            errno = ENOENT;
            fclose(ret);
            ret = NULL;
        }
    }
    else
        errno = ENOMEM;

    return ret;
}

/* Direct input/output */

extern size_t
fread(void *buffer, size_t size, size_t count, FILE *f)
{
    size_t fsize = 0;
    size_t bytes = 0;

    if (f)
    {
        bytes = size * count;
        storage_info(f->file, &fsize, NULL);
        if ((f->position + bytes) >= fsize)
        {
            bytes = fsize - f->position - 1;
            if (bytes == 0)
                f->eof = true;
        }
    }

    while (bytes != 0)
    {
        u32 sector = f->position / 0x200;
        if (!(f->cached) || sector != f->cachedsect)
        {
            if (storage_read(f->file, sector, f->cache))
            {
                f->cachedsect = sector;
                f->cached = true;
            }
            else
                break;
        }

        u16 partial = (bytes > 0x200) ? 0x200 - (f->position % 0x200) : bytes;

        memcpy(buffer, &(f->cache[f->position % 0x200]), partial);
        f->position += partial;

        buffer = &(((u8*)buffer)[partial]);
        bytes -= partial;
    }

    return (f) ? (size * count) - bytes : 0;
}

/* Unformatted input/output */

extern int
fgetc(FILE *f)
{
    int ret = EOF;

    if (!(f->eof))
    {
        char c = 0;
        fread(&c, 1, 1, f);
        ret = c;
    }

    return ret;
}

extern int
getc(FILE *f)
{
    return fgetc(f);
}

/* File positioning */

extern long
ftell(FILE *f)
{
    long ret = -1;

    if (f)
        ret = f->position;
    else
        errno = EINVAL;

    return ret;
}

extern int
fgetpos(FILE *f, fpos_t *pos)
{
    int ret = 0;

    long cur = ftell(f);
    if (cur >= 0)
        *pos = cur;
    else
        ret = 1;

    return ret;
}

extern int
fseek(FILE *f, long offset, int origin)
{
    int ret = 0;

    size_t size = 0;
    storage_info(f->file, &size, NULL);

    switch (origin)
    {
        case SEEK_SET:
            f->position = offset;
            break;

        case SEEK_CUR:
            f->position += offset;
            break;

        case SEEK_END:;
            f->position = offset + size - 1;
            break;

        default:
            ret = 1;
            errno = EINVAL;
            break;
    }

    if (!ret && f->position >= size)
    {
        f->position = size - 1;
        f->eof = true;
    }
    else
        f->eof = false;

    return ret;
}

extern int
fsetpos(FILE *f, const fpos_t *pos)
{
    return fseek(f, *pos, SEEK_SET);
}

extern void
rewind(FILE *f)
{
    f->error = 0;
    fseek(f, 0, SEEK_SET);
}

/* Error Handling */

extern void
clearerr(FILE *f)
{
    f->error = 0;
    f->eof = false;
}

extern int
feof(FILE *f)
{
    return f->eof;
}

extern int
ferror(FILE *f)
{
    return f->error;
}
