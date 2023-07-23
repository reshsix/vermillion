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

#ifndef _STDIO_H
#define _STDIO_H

#include <_types.h>
#include <stddef.h>
#include <vermillion/drivers.h>

void _stdio_init(struct device *rootfs, char *fd0, char *fd1, char *fd2);
void _stdio_clean(void);

typedef struct _FILE FILE;
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

typedef u32 fpos_t;

#define EOF (-1)
#define FILENAME_MAX ((255 * 4) + 1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *f);

size_t fread(void *buffer, size_t size, size_t count, FILE *f);
size_t fwrite(const void *buffer, size_t size, size_t count, FILE *f);

int fgetc(FILE *f);
int getc(FILE *f);
int getchar(void);
int fputc(int c, FILE *f);
int putc(int c, FILE *f);
int putchar(int c);

long ftell(FILE *f);
int fgetpos(FILE *f, fpos_t *pos);
int fseek(FILE *f, long offset, int origin);
int fsetpos(FILE *f, const fpos_t *pos);
void rewind(FILE *f);

void clearerr(FILE *f);
int feof(FILE *f);
int ferror(FILE *f);

#endif
