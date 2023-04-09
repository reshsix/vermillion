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
#include <_types.h>
#include <stdlib.h>
#include <string.h>

#include <vermillion/drivers.h>

typedef struct _FILE
{
    bool readonly;
    size_t position;
    int error;
    bool eof;

    struct driver *io;

    struct driver *fs;
    struct file *file;

    u8 cache[0x200];
    u32 cachedsect;
    bool cached;

} FILE;

/* Initialization */

FILE *stdin;
FILE *stdout;
FILE *stderr;

#define __STRING(x) #x
#define _STRING(x) __STRING(x)

extern void
_stdio_init(void)
{
    stdin  = fopen(":" _STRING(CONFIG_STDIO_STDIN),  "r");
    stdout = fopen(":" _STRING(CONFIG_STDIO_STDOUT), "w");
    stderr = fopen(":" _STRING(CONFIG_STDIO_STDERR), "w");
}

extern void
_stdio_clean(void)
{
    if (stdin)
        fclose(stdin);
    if (stdout)
        fclose(stdout);
    if (stderr)
        fclose(stdout);
}

/* File access */

static int
fclose_io(FILE *f)
{
    int ret = -1;

    if (f)
    {
        free(f);
        ret = 0;
    }

    return ret;
}

static FILE *
fopen_io(const char *path, const char *mode)
{
    FILE *ret = NULL;

    ret = calloc(1, sizeof(FILE));
    if (ret)
    {
        if (!(strcmp(mode, "r")) || !(strcmp(mode, "rb")))
            ret->readonly = true;

        ret->io = driver_find_name(path);
        if (!(ret->io) ||
            !(ret->io->api == DRIVER_API_BLOCK ||
              ret->io->api == DRIVER_API_STREAM))
        {
            errno = ENOENT;
            fclose_io(ret);
            ret = NULL;
        }
    }
    else
        errno = ENOMEM;

    return ret;
}

static int
fclose_fs(FILE *f)
{
    int ret = -1;

    if (f)
    {
        if (f->file)
            f->fs->routines.fs.close(f->file);
        free(f);
        ret = 0;
    }

    return ret;
}

static FILE *
fopen_fs(const char *path, const char *mode)
{
    FILE *ret = NULL;

    if (strcmp(mode, "r") && strcmp(mode, "rb"))
        errno = EROFS;
    else
        ret = calloc(1, sizeof(FILE));

    if (ret)
    {
        ret->readonly = true;

        ret->fs = driver_find(DRIVER_TYPE_FS, 0);
        if (ret->fs)
            ret->file = ret->fs->routines.fs.open((char*)path);

        if (!(ret->fs && ret->file))
        {
            errno = ENOENT;
            fclose_fs(ret);
            ret = NULL;
        }
    }
    else
        errno = ENOMEM;

    return ret;
}

extern int
fclose(FILE *f)
{
    int ret = -1;

    if (f && f->io)
        ret = fclose_io(f);
    else if (f && f->fs && f->file)
        ret = fclose_fs(f);
    else
        errno = EBADF;

    return ret;
}

extern FILE *
fopen(const char *path, const char *mode)
{
    FILE *ret = NULL;

    if (path)
    {
        if (path[0] == ':' && path[1] != '\0')
            ret = fopen_io(&(path[1]), mode);
        else
            ret = fopen_fs(path, mode);
    }

    return ret;
}

/* Direct input/output */

static size_t
fread_io(void *buffer, size_t size, size_t count, FILE *f)
{
    size_t ret = 0;

    size_t bytes = size * count;
    if (f->io->api == DRIVER_API_BLOCK)
    {
        while (bytes != 0)
        {
            u32 sector = f->position / 0x200;
            if (f->io->interface.block.read(f->cache, sector))
            {
                u16 partial = (bytes > 0x200 - (f->position % 0x200)) ?
                                       0x200 - (f->position % 0x200)  : bytes;

                memcpy(buffer, &(f->cache[f->position % 0x200]), partial);
                f->position += partial;
                buffer = &(((u8*)buffer)[partial]);
                bytes -= partial;
            }
            else
            {
                errno = EIO;
                break;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < bytes; i++)
        {
            u8 c = 0;
            if (f->io->interface.stream.read(&c))
            {
                ((u8*)buffer)[i] = c;
                ret++;
            }
            else
            {
                errno = EIO;
                break;
            }
        }
    }

    return ret;
}

static size_t
fwrite_io(const void *buffer, size_t size, size_t count, FILE *f)
{
    size_t ret = 0;

    size_t bytes = size * count;
    if (f->io->api == DRIVER_API_BLOCK)
    {
        while (bytes != 0)
        {
            bool failure = false;

            u32 sector = f->position / 0x200;
            if (f->io->interface.block.read(f->cache, sector))
            {
                u16 partial = (bytes > 0x200 - (f->position % 0x200)) ?
                                       0x200 - (f->position % 0x200)  : bytes;

                memcpy(&(f->cache[f->position % 0x200]),
                       (void*)buffer, partial);
                f->position += partial;
                buffer = &(((u8*)buffer)[partial]);
                bytes -= partial;

                if (!(f->io->interface.block.write(f->cache, sector)))
                    failure = true;
            }
            else
                failure = true;

            if (failure)
            {
                errno = EIO;
                break;
            }
        }
    }
    else
    {
        for (size_t i = 0; i < bytes; i++)
        {
            if (f->io->interface.stream.write(((u8*)buffer)[i]))
                ret++;
            else
            {
                errno = EIO;
                break;
            }
        }
    }

    return ret;
}

static size_t
fread_fs(void *buffer, size_t size, size_t count, FILE *f)
{
    size_t bytes = size * count;

    size_t fsize = 0;
    f->fs->routines.fs.info(f->file, &fsize, NULL);
    if ((f->position + bytes) >= fsize)
    {
        bytes = fsize - f->position - 1;
        if (bytes == 0)
            f->eof = true;
    }

    while (bytes != 0)
    {
        u32 sector = f->position / 0x200;
        if (!(f->cached) || sector != f->cachedsect)
        {
            if (f->fs->routines.fs.read(f->file, sector, f->cache))
            {
                f->cachedsect = sector;
                f->cached = true;
            }
            else
            {
                errno = EIO;
                break;
            }
        }

        u16 partial = (bytes > 0x200 - (f->position % 0x200)) ?
                               0x200 - (f->position % 0x200)  : bytes;

        memcpy(buffer, &(f->cache[f->position % 0x200]), partial);
        f->position += partial;

        buffer = &(((u8*)buffer)[partial]);
        bytes -= partial;
    }

    return (size * count) - bytes;
}

static size_t
fwrite_fs(const void *buffer, size_t size, size_t count, FILE *f)
{
    (void)buffer, (void)size, (void)count, (void)f;
    errno = EROFS;
    return 0;
}

extern size_t
fread(void *buffer, size_t size, size_t count, FILE *f)
{
    size_t ret = 0;

    if (f && f->io)
        ret = fread_io(buffer, size, count, f);
    else if (f && f->fs && f->file)
        ret = fread_fs(buffer, size, count, f);
    else
        errno = EBADF;

    return ret;
}

extern size_t
fwrite(const void *buffer, size_t size, size_t count, FILE *f)
{
    size_t ret = 0;

    if (f && !(f->readonly))
    {
        if (f->io)
            ret = fwrite_io(buffer, size, count, f);
        else if (f->fs && f->file)
            ret = fwrite_fs(buffer, size, count, f);
    }
    else if (!f)
        errno = EBADF;
    else
        errno = EROFS;

    return ret;
}

/* Unformatted input/output */

extern int
fgetc(FILE *f)
{
    int ret = EOF;

    if (f && !(f->eof))
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

extern int
getchar(void)
{
    return getc(stdin);
}

extern int
fputc(int c, FILE *f)
{
    int ret = EOF;

    if (f && !(f->eof))
    {
        char cc = c;
        fwrite(&cc, 1, 1, f);
        ret = cc;
    }

    return ret;
}

extern int
putc(int c, FILE *f)
{
    return fputc(c, f);
}

extern int
putchar(int c)
{
    return putc(c, stdout);
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

static int
fseek_io(FILE *f, long offset, int origin)
{
    int ret = 0;

    if (f->io->api == DRIVER_API_BLOCK)
    {
        switch (origin)
        {
            case SEEK_SET:
                f->position = offset;
                break;

            case SEEK_CUR:
                f->position += offset;
                break;

            case SEEK_END:;
                f->position = offset + UINT32_MAX - 1;
                break;

            default:
                ret = 1;
                errno = EINVAL;
                break;
        }

        if (!ret && f->position >= UINT32_MAX)
            f->eof = true;
        else
            f->eof = false;
    }
    else
        errno = ESPIPE;

    return ret;
}

static int
fseek_fs(FILE *f, long offset, int origin)
{
    int ret = 0;

    size_t size = 0;
    f->fs->routines.fs.info(f->file, &size, NULL);

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
fseek(FILE *f, long offset, int origin)
{
    int ret = 1;

    if (f && f->io)
        ret = fseek_io(f, offset, origin);
    else if (f && f->fs && f->file)
        ret = fseek_fs(f, offset, origin);
    else
        errno = EBADF;

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
    if (f)
    {
        f->error = 0;
        fseek(f, 0, SEEK_SET);
    }
}

/* Error Handling */

extern void
clearerr(FILE *f)
{
    if (f)
    {
        f->error = 0;
        f->eof = false;
    }
}

extern int
feof(FILE *f)
{
    return (f) ? f->eof : true;
}

extern int
ferror(FILE *f)
{
    return (f) ? f->error : EBADF;
}
