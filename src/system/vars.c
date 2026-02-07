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

#include <general/dict.h>
#include <general/types.h>

static dict *vars = NULL;

/* For devtree usage */

extern void
vars_init(void)
{
    vars = dict_new(sizeof(void *));
}

extern void
vars_clean(void)
{
    dict_del(vars);
}

/* For external usage */

extern void *
vars_get(const char *id)
{
    void *ret = NULL;
    dict_get(vars, id, &ret);
    return ret;
}

extern bool
vars_set(const char *id, void *data)
{
    if (!vars)
        vars = dict_new(sizeof(void *));

    return dict_set(vars, id, &data);
}
