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

#pragma once

#include <general/types.h>

#include <environ/fork.h>
#include <environ/state.h>

typedef struct generator
{
    bool active, finished;

    void *arg;
    fork *fk;

    state caller, callee;
} generator;

generator *generator_new(void (*f)(generator *), void *arg);
generator *generator_del(generator *g);
bool generator_next(generator *g);
void generator_rewind(generator *g);
void *generator_arg(generator *g);
void generator_yield(generator *g);
[[noreturn]] void generator_finish(generator *g);