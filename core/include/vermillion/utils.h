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

struct device *logger(struct device *log);
void log_c(const char c);
void log_s(const char *s);
void log_h(const u32 n);
void log_u(const u32 n);
void panic(const char *s);

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
void *mem_find(const void *mem, u8 c, size_t length);
void mem_init(void *mem, u8 c, size_t length);
void mem_copy(void *dest, const void *src, size_t length);

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

void memcpy(void *dest, const void *src, size_t length);
void memmove(void *dest, const void *src, size_t length);
void memset(void *mem, int c, size_t length);
int memcmp(const void *mem, const void *mem2, size_t length);

void _utils_init(void);
void _utils_clean(void);

#endif
