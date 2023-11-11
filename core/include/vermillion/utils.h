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

#ifndef VERMILLION_UTILS_H
#define VERMILLION_UTILS_H

#include <vermillion/types.h>
#include <vermillion/drivers.h>

u32 clock(struct device *tmr);
void csleep(struct device *tmr, const u32 n);
void usleep(struct device *tmr, const u32 n);
void msleep(struct device *tmr, const u32 n);
void sleep(struct device *tmr, const u32 n);

bool pin_cfg(struct device *gpio, u16 pin, u8 role, u8 pull);
bool pin_set(struct device *gpio, u16 pin, bool data);
bool pin_get(struct device *gpio, u16 pin, bool *data);

void *mem_new(size_t size);
void *mem_renew(void *mem, size_t size);
void *mem_del(void *mem);
int mem_comp(const void *mem, const void *mem2, size_t length);
void *mem_find(const void *mem, int c, size_t length);
void mem_init(void *mem, int c, size_t length);
void mem_copy(void *dest, void *src, size_t length);
void mem_move(void *dest, void *src, size_t length);

size_t str_length(const char *str);
int str_comp(const char *str, const char *str2, size_t length);
char *str_find_l(const char *str, int c);
char *str_find_r(const char *str, int c);
size_t str_span(const char *str, const char *chars, bool complement);
char *str_find_m(const char *str, const char *chars);
char *str_find_s(const char *str, const char *str2);
char *str_token(char *str, const char *chars, char **saveptr);
void str_copy(char *dest, char *src, size_t length);
void str_concat(char *dest, char *src, size_t length);

void init_utils(void);

#endif
