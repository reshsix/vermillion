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
#include <general/mem.h>
#include <general/str.h>
#include <general/path.h>

extern char *
path_cleanup(const char *path)
{
    u32 length = str_length(path);
    char *ret = mem_new(length + 2);

    if (ret)
    {
        ret[0] = '/';
        if (path[0] == '/')
            str_copy(&(ret[1]), &(path[1]), 0);
        else
            str_copy(&(ret[1]), path, 0);

        char *next = ret;
        do
        {
            u32 move = 0;
            next = str_find_l(next, '/');
            if (next)
            {
                while (next[1 + move] == '/')
                    move++;

                if (move)
                    str_copy(next, &(next[move]), 0);

                if (next[1] == '\0')
                    next = NULL;
                else
                    next = &(next[1]);
            }
        }
        while (next);

        length = str_length(ret);
        if (ret[length - 1] == '/')
            ret[length - 1] = '\0';

        if (ret[0] == '\0')
            str_copy(ret, "/", 0);
    }

    return ret;
}

extern char *
path_dirname(const char *path)
{
    char *ret = str_dupl(path, 0);

    if (ret)
    {
        char *last = str_find_r(ret, '/');
        if (last)
            last[0] = '\0';
        else
            str_copy(ret, "/", 0);

        if (ret[0] == '\0')
            str_copy(ret, "/", 0);
    }

    return ret;
}

extern char *
path_filename(const char *path)
{
    char *ret = str_dupl(path, 0);

    if (ret)
    {
        char *last = str_find_r(ret, '/');
        if (last)
            str_copy(ret, &(last[1]), 0);
    }

    return ret;
}
