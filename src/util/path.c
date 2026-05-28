/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <vermillion/util/mem.h>
#include <vermillion/util/str.h>
#include <vermillion/util/types.h>

#include <vermillion/util/path.h>

static char path_buffer[VRM_PATH_MAX] = {0};

extern bool
vrm_path_validate(const char *path)
{
    return (vrm_str_length(path) < VRM_PATH_MAX);
}

extern void
vrm_path_cleanup(char *path)
{
    path_buffer[0] = '/';
    if (path[0] == '/')
        vrm_str_copy(&(path_buffer[1]), &(path[1]), 0);
    else
        vrm_str_copy(&(path_buffer[1]), path, 0);

    char *next = path_buffer;
    do
    {
        uint32_t move = 0;
        next = vrm_str_find_l(next, '/');
        if (next)
        {
            while (next[1 + move] == '/')
                move++;

            if (move)
                vrm_str_copy(next, &(next[move]), 0);

            if (next[1] == '\0')
                next = NULL;
            else
                next = &(next[1]);
        }
    }
    while (next);

    uint32_t length = vrm_str_length(path_buffer);
    if (path_buffer[length - 1] == '/')
        path_buffer[length - 1] = '\0';

    if (path_buffer[0] == '\0')
        vrm_str_copy(path_buffer, "/", 0);

    vrm_str_copy(path, path_buffer, 0);
}

extern void
vrm_path_dirname(char *path)
{
    char *last = vrm_str_find_r(path, '/');
    if (last)
        last[0] = '\0';
    else
        vrm_str_copy(path, "/", 0);

    if (path[0] == '\0')
        vrm_str_copy(path, "/", 0);
}

extern void
vrm_path_filename(char *path)
{
    char *last = vrm_str_find_r(path, '/');
    if (last)
        vrm_str_copy(path, &(last[1]), 0);
}
