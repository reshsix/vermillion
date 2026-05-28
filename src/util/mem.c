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
    struct memblk *next;
};

#define MEMHEAD(addr, offset) \
((struct memblk *)((uint32_t)(addr) + \
    (int32_t)(offset) - sizeof(struct memblk)))
#define MEMBODY(addr, offset) \
((void *)((uint32_t)(addr) + (int32_t)(offset) + sizeof(struct memblk)))
#define MEMTOTAL(size) \
((size_t)size + sizeof(struct memblk))
#define MEMEND(head) \
((struct memblk *)((uint32_t)(head) + MEMTOTAL(((struct memblk *)head)->size)))

/* For devtree usage */

static struct memblk *head = NULL;
extern struct memblk __free;
extern void
mem_init(void)
{
    head = &__free;
    head->size = CONFIG_RAM_ADDRESS + CONFIG_RAM_SIZE + CONFIG_STACK_SIZE;
    head->size -= (uint32_t)MEMBODY(head, 0);
    head->next = NULL;
}

extern void
mem_clean(void)
{
    return;
}

/* For external usage */

extern void *
mem_new(size_t size)
{
    void *ret = NULL;

    if (size > 0 && head != NULL)
    {
        size = (size >= 32) ? size : 32;

        struct memblk *blk = NULL, *prev = NULL;
        for (struct memblk *cur = head; cur != NULL; cur = cur->next)
        {
            if (cur->size >= size)
            {
                blk = cur;
                break;
            }
            prev = cur;
        }

        if (blk != NULL)
        {
            struct memblk *next = blk->next;
            if ((blk->size - size) >= 32)
            {
                struct memblk *new = MEMBODY(blk, size);
                new->size = blk->size - MEMTOTAL(size);
                new->next = blk->next;
                next = new;

                blk->size = size;
            }

            if (prev == NULL || prev == blk)
                head = next;
            else
                prev->next = next;

            ret = MEMBODY(blk, 0);
            vrm_mem_fill(ret, 0, size);
        }
    }

    return ret;
}

extern void *
mem_renew(void *mem, size_t size)
{
    void *ret = NULL;

    struct memblk *blk = MEMHEAD(mem, 0);
    if (size > blk->size)
    {
        struct memblk *target = NULL, *prev = NULL;
        if (mem != NULL)
        {
            for (struct memblk *cur = head; cur != NULL; cur = cur->next)
            {
                if (cur == MEMEND(blk))
                {
                    if (MEMTOTAL(blk->size + cur->size) >= size)
                        target = cur;
                    break;
                }
                else if (cur > MEMEND(blk))
                    break;

                prev = cur;
            }
        }

        if (target != NULL)
        {
            size_t previous = blk->size;
            size_t avaliable = MEMTOTAL(blk->size + target->size);

            struct memblk *next = target->next;
            blk->size = size;
            if ((avaliable - size) >= 32)
            {
                struct memblk *new = MEMEND(blk);
                new->size = (uint32_t)MEMEND(target) - MEMTOTAL(new);
                new->next = next;
                next = new;
            }
            else
                blk->size = avaliable;

            if (prev == NULL)
                head = next;
            else
                prev->next = next;

            vrm_mem_fill(&(((uint8_t*)mem)[previous]), 0, blk->size - previous);
            ret = mem;
        }
        else
        {
            ret = mem_new(size);
            if (mem != NULL && ret != NULL)
            {
                vrm_mem_copy(ret, mem, blk->size);
                mem_del(mem);
            }
        }
    }
    else
    {
        if (mem)
        {
            ret = mem;
            if ((blk->size - size) >= 32)
            {
                blk->size = size;

                struct memblk *new = MEMEND(blk);
                new->size = (uint32_t)MEMEND(blk) - MEMTOTAL(new);
                new->next = blk->next;
                blk->next = new;
            }
        }
        else
            ret = mem_new(size);
    }

    return ret;
}

extern void *
mem_del(void *mem)
{
    if (mem != NULL)
    {
        struct memblk *blk = MEMHEAD(mem, 0);
        if (head != NULL)
        {
            struct memblk *end = MEMEND(blk);
            struct memblk *prev = NULL;
            struct memblk *next = head;

            while (next < end)
            {
                prev = next;
                next = next->next;
            }

            if (next == end)
            {
                blk->size += MEMTOTAL(next->size);
                blk->next = next->next;
            }
            else
                blk->next = next;

            if (prev != NULL)
            {
                prev->next = blk;
                if (MEMEND(prev) == blk)
                {
                    prev->size += MEMTOTAL(blk->size);
                    prev->next = blk->next;
                }
            }
            else
                head = blk;
        }
        else
        {
            blk->next = NULL;
            head = blk;
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
