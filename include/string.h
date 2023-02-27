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

#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void *memchr(void *mem, int c, size_t length);
int memcmp(void *mem, void *mem2, size_t length);
void *memset(void *mem, int c, size_t length);
void *memcpy(void *dest, void *src, size_t length);
void *memmove(void *dest, void *src, size_t length);

size_t strlen(const char *str);
int strcmp(const char *str, const char *str2);
int strncmp(const char *str, const char *str2, size_t length);
int strcoll(const char *str, const char *str2);
char *strchr(const char *str, int c);
char *strrchr(const char *str, int c);
size_t strspn(const char *str, const char *chars);
size_t strcspn(const char *str, const char *chars);
char *strpbrk(const char *str, const char *chars);
char *strstr(const char *str, const char *str2);
char *strtok(char *str, const char *chars);

char *strcpy(char *dest, char *str);
char *strncpy(char *dest, char *src, size_t length);
char *strcat(char *dest, char *src);
char *strncat(char *dest, char *src, size_t length);
size_t strxfrm(char *dest, char *src, size_t length);

char *strerror(int id);

#endif
