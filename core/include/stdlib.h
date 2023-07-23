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

#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

typedef struct
{
    int quot;
    int rem;
} div_t;

typedef struct
{
    long quot;
    long rem;
} ldiv_t;

void abort(void);
int atexit(void (*f)(void));
void exit(int code);
int system(const char *command);
char *getenv(const char *name);

void *malloc(size_t size);
void *calloc(size_t n, size_t size);
void *realloc(void *mem, size_t size);
void free(void *mem);

int rand(void);
void srand(unsigned seed);
int abs(int x);
long labs(long x);
div_t div(int a, int b);
ldiv_t ldiv(long a, long b);

#endif
