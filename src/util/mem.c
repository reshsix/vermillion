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

#define VERMILLION_INTERNALS
#include <vermillion/util/mem.h>
#include <vermillion/util/types.h>

struct memblk
{
    uint32_t size;
    bool last, free;
    struct memblk *phys;
    struct memblk *prev;
    struct memblk *next;
};

#define MEMBODY(addr, offset) \
((void *)((uint32_t)(addr) + (offset) + sizeof(struct memblk)))

/* For devtree usage */

static struct memblk *head = NULL;
extern struct memblk __free;
extern void
mem_init(void)
{
    head = &__free;
    head->size = (CONFIG_RAM_ADDRESS + CONFIG_RAM_SIZE +
                  CONFIG_STACK_SIZE  - (uint32_t)MEMBODY(head, 0));

    head->last = true;
    head->free = true;

    head->phys = NULL;
    head->prev = NULL;
    head->next = NULL;
}

extern void
mem_clean(void)
{
    return;
}

/* For external usage */

extern void *
vrm_mem_new(size_t size)
{
    void *ret = NULL;

    size = (size >= 16) ? size : 16;

    struct memblk *blk = NULL;
    for (struct memblk *cur = head; cur != NULL; cur = cur->next)
    {
        if ((size_t)(cur->size) >= size)
        {
            blk = cur;
            break;
        }
    }

    if (blk)
    {
        if (blk->size >= (16 + size + sizeof(struct memblk)))
        {
            struct memblk *new = MEMBODY(blk, size);
            new->size = (blk->size - size - sizeof(struct memblk));
            new->last = true;
            new->free = true;
            new->phys = blk;
            new->prev = blk->prev;
            new->next = blk->next;

            if (blk->prev != NULL)
                blk->prev->next = new;
            else
                head = new;

            if (blk->next != NULL)
                blk->next->prev = new;

            blk->size = size;
            blk->last = false;
        }
        else
        {
            if (blk->prev != NULL)
                blk->prev->next = blk->next;
            else
                head = blk->next;

            if (blk->next != NULL)
                blk->next->prev = blk->prev;
        }

        blk->prev = NULL;
        blk->next = NULL;
        blk->free = false;

        ret = MEMBODY(blk, 0);
    }

    return ret;
}

extern void *
vrm_mem_del(void *mem)
{
    if (mem != NULL)
    {
        struct memblk *blk = mem;
        blk = &(blk[-1]);

        if (head != NULL)
            blk->next = head;
        else
            blk->next = NULL;

        blk->prev = NULL;
        blk->free = true;
        head = blk;

        /* Coalesce left */
        struct memblk *prev = blk->phys;
        if (prev && prev->free)
        {
            prev->size += blk->size + sizeof(struct memblk);
            prev->last  = blk->last;
            head = blk->next;
        }

        /* Absorb right */
        struct memblk *next = MEMBODY(blk, blk->size);
        if (!(blk->last) && next->free)
        {
            blk->size += next->size + sizeof(struct memblk);
            blk->last  = next->last;
            blk->next  = next->next;

            if (next->next != NULL)
                next->next->prev = blk;
        }
    }

    return NULL;
}

extern int
vrm_mem_comp(const void *mem, const void *mem2, size_t length)
{
    int ret = 0;

    for (size_t i = 0; i < length; i++)
    {
        uint8_t c = ((uint8_t*)mem)[i];
        uint8_t c2 = ((uint8_t*)mem2)[i];
        if (c != c2)
        {
            ret = c - c2;
            break;
        }
    }

    return ret;
}

extern void *
vrm_mem_find(const void *mem, uint8_t c, size_t length)
{
    void *ret = NULL;

    for (size_t i = 0; i < length; i++)
    {
        uint8_t *check = &(((uint8_t*)mem)[i]);
        if (*check == c)
        {
            ret = check;
            break;
        }
    }

    return ret;
}

extern void
vrm_mem_fill(void *mem, uint8_t c, size_t length)
{
    for (size_t i = 0; i < length; i++)
        ((uint8_t*)mem)[i] = c;
}

extern void
vrm_mem_copy(void *dest, const void *src, size_t length)
{
    if ((uint32_t)src           < (uint32_t)dest &&
       ((uint32_t)src + length) > (uint32_t)dest)
    {
        for (size_t i = 0; i != length; i++)
            ((uint8_t*)dest)[length - i - 1] = ((uint8_t*)src)[length - i -1];
    }
    else if (dest != src)
    {
        for (size_t i = 0; i < length; i++)
            ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
    }
}

__attribute__((weak))
extern void
memcpy(void * restrict dest, const void * restrict src, size_t length)
{
    vrm_mem_copy(dest, src, length);
}

__attribute__((weak))
extern void
memmove(void *dest, const void *src, size_t length)
{
    vrm_mem_copy(dest, src, length);
}

__attribute__((weak))
extern void
memset(void *mem, int c, size_t length)
{
    vrm_mem_fill(mem, c, length);
}

__attribute__((weak))
extern int
memcmp(const void *mem, const void *mem2, size_t length)
{
    return vrm_mem_comp(mem, mem2, length);
}
