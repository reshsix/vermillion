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

#ifndef _STDDEF_H
#define _STDDEF_H

typedef unsigned int size_t;
typedef signed int ptrdiff_t;
typedef signed int wchar_t;

#define NULL ((void*)0)
#define offsetof(t, x) (size_t)&(((type*)0)->x)

#endif
