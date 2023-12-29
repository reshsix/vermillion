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

struct generator;
struct generator *generator_new(void (*f)(struct generator *), void *arg);
struct generator *generator_del(struct generator *g);
bool generator_next(struct generator *g);
void generator_rewind(struct generator *g);
void *generator_arg(struct generator *g);
void generator_yield(struct generator *g);
noreturn generator_finish(struct generator *g);

#endif
