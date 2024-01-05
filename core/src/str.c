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
#include <core/str.h>

extern size_t
str_length(const char *str)
{
    return ((char*)mem_find(str, '\0', SIZE_MAX)) - str;
}

extern int
str_comp(const char *str, const char *str2, size_t length)
{
    length = (length != 0) ? length : SIZE_MAX;
    size_t l = str_length(str);
    size_t l2 = str_length(str2);
    size_t ln = ((l < l2) ? l : l2) + 1;
    return mem_comp(str, str2, (ln < length) ? ln : length);
}

extern size_t
str_span(const char *str, const char *chars, bool complement)
{
    size_t ret = 0;

    size_t l = str_length(str);
    size_t l2 = str_length(chars);
    ret = l;

    for (size_t i = 0; i < l; i++)
    {
        bool has = false;
        for (size_t j = 0; j < l2; j++)
        {
            if (str[i] == chars[j])
            {
                has = true;
                break;
            }
        }

        if (complement)
            has = !has;

        if (!has)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

extern char *
str_find_l(const char *str, char c)
{
    char *ret = NULL;

    size_t l = str_length(str);
    if (c != '\0')
        ret = mem_find(str, c, l);
    else
        ret = (char*)&(str[l]);

    return ret;
}

extern char *
str_find_r(const char *str, char c)
{
    char *ret = NULL;

    size_t l = str_length(str);
    if (c != '\0')
    {
        for (size_t i = l; i > 0; i--)
        {
            if (str[i-1] == c)
            {
                ret = (char*)&(str[i-1]);
                break;
            }
        }
    }
    else
        ret = (char*)&(str[l]);

    return ret;
}

extern char *
str_find_m(const char *str, const char *chars)
{
    char *ret = NULL;

    char *maybe = (char*)&(str[str_span(str, chars, true)]);
    if (*maybe != '\0')
        ret = maybe;

    return ret;
}

extern char *
str_find_s(const char *str, const char *str2)
{
    char *ret = NULL;

    size_t l = str_length(str);
    size_t l2 = str_length(str2);
    for (size_t i = 0; i < l; i++)
    {
        if (str_comp(&(str[i]), str2, l2) == 0)
        {
            ret = (char*)&(str[i]);
            break;
        }
    }

    return ret;
}

extern char *
str_token(char *str, const char *chars, char **saveptr)
{
    char *ret = NULL;

    char *state = *saveptr;
    if (str != NULL)
        state = str;

    if (state != NULL)
    {
        state = &(state[str_span(state, chars, false)]);
        if (state[0] != '\0')
        {
            ret = state;
            state = &(state[str_span(state, chars, true)]);
            if (state[0] != '\0')
            {
                state[0] = '\0';
                state = &(state[1]);
                if (state[0] == '\0')
                    state = NULL;
            }
        }
    }
    *saveptr = state;

    return ret;
}

extern void
str_copy(char *dest, char *src, size_t length)
{
    size_t l = str_length(src);

    if (length > l)
    {
        mem_copy(dest, src, l);
        mem_init(&(dest[l]), '\0', length - l);
    }
    else if (length != 0)
        mem_copy(dest, src, length);
    else
        mem_copy(dest, src, l + 1);
}

extern void
str_concat(char *dest, char *src, size_t length)
{
    size_t l = str_length(dest);

    if (length != 0)
    {
        size_t l2 = str_length(src);
        if (length < l2)
        {
            mem_copy(&(dest[l]), src, length);
            dest[l + length] = '\0';
        }
        else
            mem_copy(&(dest[l]), src, l2 + 1);
    }
    else
        str_copy(&(dest[l]), src, 0);
}

extern char *
str_dupl(char *str, size_t length)
{
    char *ret = NULL;

    length = (length == 0) ? str_length(str) : length;

    if ((ret = mem_new(length + 1)))
    {
        mem_copy(ret, str, length + 1);
        ret[length] = '\0';
    }

    return ret;
}
