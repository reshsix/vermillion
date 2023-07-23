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
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <vermillion/drivers.h>

typedef struct _FILE
{
    bool readonly;
    size_t position;
    int error;
    bool eof;

    int unget;

    struct device *io;

    struct device *fs;
    struct file *file;

    u8 cache[0x200];
    u32 cachedsect;
    bool cached;

} FILE;

/* Initialization */

struct device *root;

FILE *stdin;
FILE *stdout;
FILE *stderr;

static FILE *fopen_io(const char *path, const char *mode);
extern void
_stdio_init(struct device *rootfs, char *fd0, char *fd1, char *fd2)
{
    root = rootfs;

    stdin  = fopen_io(fd0,  "r");
    stdout = fopen_io(fd1,  "w");
    stderr = fopen_io(fd2,  "w");
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

        ret->io = device_find(path);
        if (!(ret->io) ||
            !(ret->io->driver->api == DRIVER_API_BLOCK ||
              ret->io->driver->api == DRIVER_API_STREAM))
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
            f->fs->driver->interface.fs.close(f->file);
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
        ret->fs = root;
        if (ret->fs)
        {
            ret->readonly = true;
            ret->file = ret->fs->driver->interface.fs.open(ret->fs->context,
                                                           (char*)path);
        }

        if (!(ret->file))
        {
            errno = ENOENT;
            free(ret);
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

        ret->unget = EOF;
    }

    return ret;
}

/* Direct input/output */

static size_t
fread_io(void *buffer, size_t length, FILE *f)
{
    size_t ret = 0;

    size_t bytes = length;
    if (f->io->driver->api == DRIVER_API_BLOCK)
    {
        while (bytes != 0)
        {
            u32 sector = f->position / 0x200;
            if (f->io->driver->interface.block.read(f->io->context,
                                                    f->cache, sector))
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
            if (f->io->driver->interface.stream.read(f->io->context, &c))
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
    if (f->io->driver->api == DRIVER_API_BLOCK)
    {
        while (bytes != 0)
        {
            bool failure = false;

            u32 sector = f->position / 0x200;
            if ((bytes == 0x200 && !(f->position % 0x200)) ||
                f->io->driver->interface.block.read(f->io->context,
                                                    f->cache, sector))
            {
                u16 partial = (bytes > 0x200 - (f->position % 0x200)) ?
                                       0x200 - (f->position % 0x200)  : bytes;

                memcpy(&(f->cache[f->position % 0x200]),
                       (void*)buffer, partial);
                f->position += partial;
                buffer = &(((u8*)buffer)[partial]);
                bytes -= partial;

                if (!(f->io->driver->interface.block.write(f->io->context,
                                                           f->cache, sector)))
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
            if (f->io->driver->interface.stream.write(f->io->context,
                                                      ((u8*)buffer)[i]))
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
fread_fs(void *buffer, size_t length, FILE *f)
{
    size_t bytes = length;

    size_t fsize = 0;
    f->fs->driver->interface.fs.info(f->file, &fsize, NULL);
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
            if (f->fs->driver->interface.fs.read(f->file, sector, f->cache))
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

    return length - bytes;
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

    size_t length = size * count;
    if (length != 0)
    {
        if (f && f->unget != EOF)
        {
            ((u8*)buffer)[0] = f->unget;
            f->unget = EOF;
            length--;
        }
    }

    if (length != 0)
    {
        if (f && f->io)
            ret = fread_io(buffer, length, f);
        else if (f && f->fs && f->file)
            ret = fread_fs(buffer, length, f);
        else
            errno = EBADF;
    }

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

extern char *
fgets(char *s, int n, FILE *f)
{
    char *ret = NULL;

    if (n > 0 && f && (f->eof))
    {
        int i = 0;
        for (; i < n - 1; i++)
        {
            int c = fgetc(f);
            if (c == EOF || c == '\n')
                break;

            s[i] = c;
        }

        if (i != 0)
        {
            s[i] = '\0';
            ret = s;
        }
    }

    return ret;
}

extern char *
gets(char *s)
{
    return fgets(s, INT_MAX, stdin);
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

extern int
fputs(const char *s, FILE *f)
{
    int ret = 0;

    for (size_t i = 0; ret >= 0 && s[i] != '\0'; i++)
        ret = fputc(s[i], f);

    return ret;
}

extern int
puts(const char *s)
{
    return fputs(s, stdout);
}

extern int
ungetc(int c, FILE *f)
{
    int ret = EOF;

    if (c >= 0 && f && f->unget == EOF && f->position != 0)
    {
        f->unget = c;
        f->eof = false;
        f->position--;

        ret = c;
    }

    return ret;
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

    if (f->io->driver->api == DRIVER_API_BLOCK)
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
                f->position = offset + UINT32_MAX;
                break;

            default:
                ret = 1;
                errno = EINVAL;
                break;
        }

        if (!ret && f->position == UINT32_MAX)
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
    f->fs->driver->interface.fs.info(f->file, &size, NULL);

    switch (origin)
    {
        case SEEK_SET:
            f->position = offset;
            break;

        case SEEK_CUR:
            f->position += offset;
            break;

        case SEEK_END:;
            f->position = offset + size;
            break;

        default:
            ret = 1;
            errno = EINVAL;
            break;
    }

    if (!ret && f->position >= size)
    {
        f->position = size;
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

    if (f)
        f->unget = EOF;

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
