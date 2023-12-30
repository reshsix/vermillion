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

#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <core/types.h>

#include <core/dev.h>

dev_stream *logger(union dev_stream_ptr log);
void log_c(const char c);
void log_s(const char *s);
void log_h(const u32 n);
void log_u(const u32 n);
void panic(const char *s);

u32 clock(dev_timer *tmr);
void csleep(dev_timer *tmr, const u32 n);
void usleep(dev_timer *tmr, const u32 n);
void msleep(dev_timer *tmr, const u32 n);
void sleep(dev_timer *tmr, const u32 n);

bool pin_cfg(dev_gpio *gpio, u16 pin, u8 role, u8 pull);
bool pin_set(dev_gpio *gpio, u16 pin, bool data);
bool pin_get(dev_gpio *gpio, u16 pin, bool *data);

#endif
