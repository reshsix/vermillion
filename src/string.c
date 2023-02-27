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

#include <errno.h>
#include <types.h>

/* Memory manipulation */

extern void *
memchr(const void *mem, int c, size_t length)
{
    void *ret = NULL;

    for (size_t i = 0; i < length; i++)
    {
        u8 *check = &(((u8*)mem)[i]);
        if (*check == c)
        {
            ret = check;
            break;
        }
    }

    return ret;
}

extern int
memcmp(const void *mem, const void *mem2, size_t length)
{
    int ret = 0;

    for (size_t i = 0; i < length; i++)
    {
        u8 c = ((u8*)mem)[i];
        u8 c2 = ((u8*)mem2)[i];
        if (c != c2)
        {
            ret = c - c2;
            break;
        }
    }

    return ret;
}

extern void *
memset(void *mem, int c, size_t length)
{
    for (size_t i = 0; i < length; i++)
        ((u8*)mem)[i] = c;

    return mem;
}

extern void *
memcpy(void *dest, void *src, size_t length)
{
    for (size_t i = 0; i < length; i++)
        ((u8*)dest)[i] = ((u8*)src)[i];

    return dest;
}

extern void *
memmove(void *dest, void *src, size_t length)
{
    if (((u32)src + length) > (u32)dest)
    {
        for (size_t i = length; i > 0; i--)
            ((u8*)dest)[i-1] = ((u8*)src)[i-1];
    }
    else
        memcpy(dest, src, length);

    return dest;
}

/* String examination */

extern size_t
strlen(const char *str)
{
    return ((char*)memchr(str, '\0', SIZE_MAX)) - str;
}

extern int
strcmp(const char *str, const char *str2)
{
    size_t l = strlen(str);
    size_t l2 = strlen(str2);
    return memcmp(str, str2, ((l < l2) ? l : l2) + 1);
}

extern int
strncmp(const char *str, const char *str2, size_t length)
{
    size_t l = strlen(str);
    size_t l2 = strlen(str2);
    size_t ln = ((l < l2) ? l : l2) + 1;
    return memcmp(str, str2, (ln < length) ? ln : length);
}

extern int
strcoll(const char *str, const char *str2)
{
    return strcmp(str, str2);
}

extern char *
strchr(const char *str, int c)
{
    char *ret = NULL;

    size_t l = strlen(str);
    if (c != '\0')
        ret = memchr(str, c, l);
    else
        ret = (char*)&(str[l]);

    return ret;
}

extern char *
strrchr(const char *str, int c)
{
    char *ret = NULL;

    size_t l = strlen(str);
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

static size_t
__strspn(const char *str, const char *chars, bool complement)
{
    size_t ret = 0;

    size_t l = strlen(str);
    size_t l2 = strlen(chars);
    ret = l;

    for (size_t i = 0; i < l; i++)
    {
        bool has = false;
        for (size_t j = 0; i < l2; j++)
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

extern size_t
strspn(const char *str, const char *chars)
{
    return __strspn(str, chars, false);
}

extern size_t
strcspn(const char *str, const char *chars)
{
    return __strspn(str, chars, true);
}

extern char *
strpbrk(const char *str, const char *chars)
{
    char *ret = NULL;

    char *maybe = (char*)&(str[strcspn(str, chars)]);
    if (*maybe != '\0')
        ret = maybe;

    return ret;
}

extern char *
strstr(const char *str, const char *str2)
{
    char *ret = NULL;

    size_t l = strlen(str);
    size_t l2 = strlen(str2);
    for (size_t i = 0; i < l; i++)
    {
        if (strncmp(&(str[i]), str2, l2) == 0)
        {
            ret = (char*)&(str[i]);
            break;
        }
    }

    return ret;
}

extern char *
strtok(char *str, const char *chars)
{
    char *ret = NULL;

    static char *state = NULL;
    if (str != NULL)
        state = str;

    if (state != NULL)
    {
        state = &(state[strspn(state, chars)]);
        if (state[0] != '\0')
        {
            ret = state;
            state = &(state[strcspn(state, chars)]);
            if (state[0] != '\0')
            {
                state = &(state[1]);
                state[0] = '\0';
            }
        }
    }

    return ret;
}

/* String manipulation */

extern char *
strcpy(char *dest, char *src)
{
    return memcpy(dest, src, strlen(src) + 1);
}

extern char *
strncpy(char *dest, char *src, size_t length)
{
    size_t l = strlen(src);

    if (length > l)
    {
        memcpy(dest, src, l);
        memset(&(dest[l]), '\0', length - l);
    }
    else
        memcpy(dest, src, length);

    return dest;
}

extern char *
strcat(char *dest, char *src)
{
    size_t l = strlen(dest);
    strcpy(&(dest[l]), src);
    return dest;
}

extern char *
strncat(char *dest, char *src, size_t length)
{
    size_t l = strlen(dest);
    size_t l2 = strlen(src);

    if (length <= l)
    {
        memcpy(&(dest[l]), src, length);
        dest[l + length] = '\0';
    }
    else
        memcpy(&(dest[l]), src, l2);

    return dest;
}

extern size_t
strxfrm(char *dest, char *src, size_t length)
{
    size_t ret = strlen(src);

    if (length != 0 || dest == NULL)
    {
        dest[0] = '\0';
        strncat(dest, src, length);
    }

    return ret;
}

/* Miscellaneous */

extern char *
strerror(int id)
{
    char *ret = NULL;

    switch (id)
    {
        case EDOM:
            ret = "Domain error";
            break;
        case ERANGE:
            ret = "Range error";
            break;
        default:
            ret = "Unknown error";
            break;
    }

    return ret;
}
