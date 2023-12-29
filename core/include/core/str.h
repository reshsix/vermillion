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

#ifndef CORE_STR_H
#define CORE_STR_H

#include <core/types.h>

size_t str_length(const char *str);
int str_comp(const char *str, const char *str2, size_t length);
size_t str_span(const char *str, const char *chars, bool complement);
char *str_find_l(const char *str, char c);
char *str_find_r(const char *str, char c);
char *str_find_m(const char *str, const char *chars);
char *str_find_s(const char *str, const char *str2);
char *str_token(char *str, const char *chars, char **saveptr);
void str_copy(char *dest, char *src, size_t length);
void str_concat(char *dest, char *src, size_t length);
char *str_dupl(char *str, size_t length);

#endif
