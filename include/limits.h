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

#ifndef _LIMITS_H
#define _LIMITS_H

#define MB_LEN_MAX 4

#define CHAR_BIT   8
#define CHAR_MIN   0x0
#define CHAR_MAX   0xFF
#define UCHAR_MAX  CHAR_MAX
#define SCHAR_MIN  0x80
#define SCHAR_MAX  0x7F
#define USHRT_MAX  0xFFFF
#define SHRT_MIN   0x8000
#define SHRT_MAX   0x7FFF
#define UINT_MAX   0xFFFFFFFF
#define INT_MIN    0x80000000
#define INT_MAX    0x7FFFFFFF
#define ULONG_MAX  UINT_MAX
#define LONG_MIN   INT_MIN
#define LONG_MAX   INT_MAX
#define ULLONG_MAX 0xFFFFFFFFFFFFFFFF
#define LLONG_MIN  0x7FFFFFFFFFFFFFFF
#define LLONG_MAX  0x8000000000000000

#endif
