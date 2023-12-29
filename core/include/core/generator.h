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

#ifndef CORE_GENERATOR_H
#define CORE_GENERATOR_H

#include <core/types.h>

typedef struct _generator generator;
generator *generator_new(void (*f)(generator *), void *arg);
generator *generator_del(generator *g);
bool generator_next(generator *g);
void generator_rewind(generator *g);
void *generator_arg(generator *g);
void generator_yield(generator *g);
noreturn generator_finish(generator *g);

#endif
