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

#include <general/types.h>

#include <environ/generator.h>

#include <debug/assert.h>

[[noreturn]]
static void
test_generator(generator *g)
{
    bool *arg = generator_arg(g);

    generator_yield(g);

    *arg = true;
    generator_yield(g);

    *arg = false;
    generator_yield(g);

    generator_yield(g);
    generator_yield(g);

    *arg = true;
    generator_finish(g);
}

extern void
test_environ_generator(void)
{
    bool arg = false;
    generator *g = generator_new(test_generator, &arg);
    generator *pg = g;
    assert (g != NULL);

    g = generator_del(g);
    assert (g == NULL);

    g = generator_new(test_generator, &arg);
    assert (g != NULL);
    assert (g == pg);

    assert (generator_next(g));
    assert (generator_next(g));
    assert (arg);

    generator_rewind(g);
    arg = false;

    assert (generator_next(g));
    assert (generator_next(g));
    assert (arg);

    arg = true;
    assert (generator_next(g));
    assert (!arg);

    arg = false;
    assert (generator_next(g));
    assert (generator_next(g));
    assert (!generator_next(g));
    assert (arg);

    generator_del(g);
}
