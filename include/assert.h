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

#ifndef _ASSERT_H
#define _ASSERT_H

int __assert(const char *f, int l, const char *fn, const char *e);

#ifndef NDEBUG
#define assert(x) (void)((x) || __assert(__FILE__, __LINE__, __func__, #x))
#else
#define assert(x) ((void)0)
#endif

#endif
