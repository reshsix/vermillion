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
#include <general/dict.h>

struct dict
{
    size_t type;
    u32 size;

    char **ids;
    u8 *meta, *data;

    size_t count;
};

static u32
dict_hash(const char *s)
{
    /* djb2 hash function */
    u32 ret = 5381;

    for (u32 i = 0; s[i] != '\0'; i++)
        ret = ret * 33 + s[i];

    return ret;
}

static dict *
dict_new_st(size_t type, size_t size)
{
    dict *ret = mem_new(sizeof(dict));

    if (ret)
    {
        ret->type = type;
        ret->size = size;

        ret->meta = mem_new(sizeof(u8) * ret->size);
        ret->ids = mem_new(sizeof(char *) * ret->size);
        ret->data = mem_new(type * ret->size);
        if (!(ret->meta && ret->ids && ret->data))
        {
            ret->meta = mem_del(ret->meta);
            ret->ids = mem_del(ret->ids);
            ret->data = mem_del(ret->data);
            ret = mem_del(ret);
        }
    }

    if (ret)
        mem_fill(ret->meta, 0x80, sizeof(u8) * ret->size);

    return ret;
}

static dict *
dict_del_st(dict *d, bool base)
{
    if (d)
    {
        for (u32 i = 0; i < d->size; i++)
            mem_del(d->ids[i]);
        mem_del(d->ids);

        mem_del(d->meta);
        mem_del(d->data);

        if (base)
            mem_del(d);
    }

    return NULL;
}

static bool
dict_get_st(dict *d, const char *id, void *data)
{
    bool ret = false;

    u32 hash = dict_hash(id) % d->size;
    for (u32 i = hash; i < d->size; i++)
    {
        if (d->meta[i] > 0x7f)
            break;

        if (d->meta[i] != (hash & 0x7f))
            continue;

        if (d->ids[i] && str_comp(id, d->ids[i], 0) == 0)
        {
            if (data)
                mem_copy(data, &(d->data[i * d->type]), d->type);
            ret = true;
            break;
        }
    }

    return ret;
}

static bool
dict_set_st(dict *d, const char *id, void *data)
{
    bool ret = false;

    bool found = false, insert = false;
    u32 hash = dict_hash(id) % d->size;
    for (u32 i = hash; i < d->size; i++)
    {
        if (d->meta[i] == (hash & 0x7f) && d->ids[i] &&
                                           str_comp(id, d->ids[i], 0) == 0)
            found = true;
        else if (d->meta[i] > 0x7f)
            insert = true;

        if (found || insert)
        {
            if (data)
            {
                d->ids[i] = str_dupl((char *)id, 0);
                d->meta[i] = hash & 0x7f;
                mem_copy(&(d->data[i * d->type]), data, d->type);
                if (insert)
                    d->count++;
            }
            else
            {
                d->ids[i] = mem_del(d->ids[i]);
                d->meta[i] = 0x80;
                mem_fill(&(d->data[i * d->type]), 0, d->type);
                if (insert)
                    d->count--;
            }
            ret = true;
            break;
        }
    }

    return ret;
}

static bool
dict_resize(dict *d)
{
    bool ret = true;

    dict *d2 = dict_new_st(d->type, d->size * 2);
    if (!d2)
        ret = false;

    for (u32 i = 0; ret && i < d->size; i++)
    {
        if (d->meta[i] <= 0x7f && d->ids[i])
            ret = dict_set_st(d2, d->ids[i], &(d->data[i * d->type]));
    }

    if (ret)
    {
        dict_del_st(d, false);
        mem_copy(d, d2, sizeof(dict));
    }

    return ret;
}

extern dict *
dict_new(size_t type)
{
    return dict_new_st(type, 32);
}

extern dict *
dict_del(dict *d)
{
    return dict_del_st(d, true);
}

extern bool
dict_get(dict *d, const char *id, void *data)
{
    bool ret = (d && id);

    if (ret)
        ret = dict_get_st(d, id, data);

    return ret;
}

extern bool
dict_set(dict *d, const char *id, void *data)
{
    bool ret = (d && id);

    if (ret && d->count >= ((d->size * 3) / 4))
        ret = dict_resize(d);

    if (ret)
        ret = dict_set_st(d, id, data);

    return ret;
}
