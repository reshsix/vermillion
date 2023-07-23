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

#ifndef _UTILS_H
#define _UTILS_H

#include <_types.h>
#include <vermillion/drivers.h>

void print(const char *s);
void print_hex(const u32 n);
void print_uint(const u32 n);

u32 clock(struct device *tmr);
void csleep(struct device *tmr, const u32 n);
void usleep(struct device *tmr, const u32 n);
void msleep(struct device *tmr, const u32 n);
void sleep(struct device *tmr, const u32 n);

bool pin_cfg(struct device *gpio, u16 pin, u8 role, u8 pull);
bool pin_set(struct device *gpio, u16 pin, bool data);
bool pin_get(struct device *gpio, u16 pin, bool *data);

#endif
