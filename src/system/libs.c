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

#include <general/mem.h>
#include <general/str.h>
#include <general/types.h>

struct lib
{
    char name[64];
    void *pointer;
    void *memory;
};

static struct lib libs[64] = {0};

extern bool
libs_load(const char *name, void *pointer, void *memory)
{
    bool ret = false;

    if (name && pointer && memory && (str_length(name) < sizeof(libs[0].name)))
    {
        s16 free = -1;
        for (size_t i = 0; i < (sizeof(libs) / sizeof(struct lib)); i++)
        {
            if (free < 0 && libs[i].name[0] == '\0')
                free = i;

            if (str_comp(name, libs[i].name, sizeof(libs[i].name)) == 0)
            {
                free = i;
                break;
            }
        }

        if (!ret && free >= 0)
        {
            str_copy(libs[free].name, name, sizeof(libs[free].name));
            libs[free].pointer = pointer;
            libs[free].memory  = memory;
            ret = true;
        }
    }

    return ret;
}

extern bool
libs_unload(const char *name)
{
    bool ret = false;

    if (name)
    {
        for (size_t i = 0; i < (sizeof(libs) / sizeof(struct lib)); i++)
        {
            if (str_comp(name, libs[i].name, sizeof(libs[i].name)) == 0)
            {
                libs[i].name[0] = '\0';
                libs[i].pointer = NULL;

                mem_del(libs[i].memory);
                libs[i].memory  = NULL;

                ret = true;
                break;
            }
        }
    }

    return ret;
}

extern void *
libs_pointer(const char *name)
{
    void *ret = NULL;

    if (name)
    {
        for (size_t i = 0; i < (sizeof(libs) / sizeof(struct lib)); i++)
        {
            if (str_comp(name, libs[i].name, sizeof(libs[i].name)) == 0)
            {
                ret = libs[i].pointer;
                break;
            }
        }
    }

    return ret;
}
