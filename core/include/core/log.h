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

#ifndef CORE_LOG_H
#define CORE_LOG_H

#include <core/types.h>

#include <core/dev.h>

void log_set_dev(union dev_stream_ptr logger);
dev_stream *log_get_dev(void);

void log_char(const char c);
void log_string(const char *s);
void log_bool(const bool n);
void log_unsigned(const u64 n);
void log_signed(const s64 n);

#define log(x) _Generic((x), char: log_char, char *: log_string, \
                             bool: log_bool,                     \
                             u8:  log_unsigned, s8:  log_signed, \
                             u16: log_unsigned, s16: log_signed, \
                             u32: log_unsigned, s32: log_signed, \
                             u64: log_unsigned, s64: log_signed)(x)

#endif
