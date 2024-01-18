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

typedef struct dict
{
    size_t type;
    u32 size;

    char **ids;
    u8 *meta, *data;

    size_t count;
} dict;

dict *dict_new(size_t type);
dict *dict_del(dict *d);
bool dict_get(dict *d, const char *id, void *data);
bool dict_set(dict *d, const char *id, void *data);
