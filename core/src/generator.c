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

#include <core/types.h>

#include <core/mem.h>
#include <core/fork.h>
#include <core/state.h>
#include <core/generator.h>

extern generator *
generator_new(void (*f)(generator *), void *arg)
{
    generator *ret = mem_new(sizeof(generator));

    if (ret)
    {
        ret->arg = arg;
        ret->fk = fork_new((void (*)(void *))f, ret);
        if (!(ret->fk))
            ret = generator_del(ret);
    }

    return ret;
}

extern generator *
generator_del(generator *g)
{
    if (g)
        fork_del(g->fk);

    return mem_del(g);
}

extern bool
generator_next(generator *g)
{
    if (!(g->finished))
    {
        if (state_save(&(g->caller)) == NULL)
        {
            if (g->active)
                state_load(&(g->callee), (void*)0x1);
            else
            {
                g->active = true;
                fork_run(g->fk);
            }
        }
    }

    return !(g->finished);
}

extern void
generator_rewind(generator *g)
{
    g->active = false;
    g->finished = false;
}

extern void *
generator_arg(generator *g)
{
    return g->arg;
}

extern void
generator_yield(generator *g)
{
    if (state_save(&(g->callee)) == NULL)
        state_load(&(g->caller), (void*)0x1);
}

[[noreturn]]
extern void
generator_finish(generator *g)
{
    g->finished = true;
    generator_yield(g);
    while (true);
}
