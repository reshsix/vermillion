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

#include <vermillion/types.h>
#include <vermillion/utils.h>
#include <vermillion/drivers.h>
#include <vermillion/interrupts.h>

/* Logging helpers */

static struct device *logdev = NULL;

extern void
logger(struct device *log)
{
    logdev = log;
}

extern void
log_c(const char c)
{
    if (logdev != NULL)
        logdev->io->driver->interface.stream.write(logdev->io->context, c);
}

extern void
log_s(const char *s)
{
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            log_c(s[0]);
    }
}

static void
log_h8(const u8 n)
{
    for (u8 i = 1; i <= 1; i--)
    {
        u8 x = (n >> (i * 4)) & 0xF;
        if (x < 10)
            log_c(x + '0');
        else
            log_c(x - 10 + 'A');
    }
}

extern void
log_h(const u32 n)
{
    print("0x");
    if (n >= (1 << 24))
        print_h8(n >> 24);
    if (n >= (1 << 16))
        print_h8(n >> 16);
    if (n >= (1 << 8))
        print_h8(n >> 8);
    print_h8(n);
}

extern void
log_u(const u32 n)
{
    bool start = false;

    u32 a = n;
    for (int i = 1000000000;; i /= 10)
    {
        u8 d = a / i;
        if (d != 0)
            start = true;

        if (start)
        {
            log_c(d + '0');
            a -= i * d;
        }

        if (i == 1)
            break;
    }

    if (!start)
        log_c('0');
}

extern void
panic(const char *s)
{
    log_s(s);
    for (;;)
        intr_wait();
}

/* Timing helpers */

extern u32
clock(struct device *tmr)
{
    union config cfg = {0};
    tmr->driver->config.get(tmr->context, &cfg);
    return cfg.timer.clock;
}

extern void
csleep(struct device *tmr, const u32 n)
{
    tmr->driver->interface.block.write(tmr->context, (u8*)&n, 0);
}

static void
csleep2(struct device *tmr, const u32 n, u32 div)
{
    s64 clk = clock(tmr) / div;
    for (s64 a = clk * (s64)n; a > 0; a -= UINT32_MAX)
        csleep(tmr, (a < UINT32_MAX) ? a : UINT32_MAX);
}

extern void
usleep(struct device *tmr, const u32 n)
{
    csleep2(tmr, n, 1000000);
}

extern void
msleep(struct device *tmr, const u32 n)
{
    csleep2(tmr, n, 1000);
}

extern void
sleep(struct device *tmr, const u32 n)
{
    csleep2(tmr, n, 1);
}

/* IO helpers */

extern bool
pin_cfg(struct device *gpio, u16 pin, u8 role, u8 pull)
{
    bool ret = false;

    union config config = {0};
    gpio->driver->config.get(gpio->context, &config);
    ret = config.gpio.pin(gpio->context, pin, role, pull);

    return ret;
}

extern bool
pin_set(struct device *gpio, u16 pin, bool data)
{
    bool ret = false;

    u8 block = pin / 32;
    u8 bit = pin % 32;

    u32 reg = 0;
    if (gpio->driver->interface.block.read(gpio->context, (u8*)&reg, block))
    {
        if (data)
            reg |= (1 << bit);
        else
            reg &= ~(1 << bit);

        ret = gpio->driver->interface.block.write(gpio->context,
                                                  (u8*)&reg, block);
    }

    return ret;
}

extern bool
pin_get(struct device *gpio, u16 pin, bool *data)
{
    bool ret = false;

    u8 block = pin / 32;
    u8 bit = pin % 32;

    u32 reg = 0;
    if (gpio->driver->interface.block.read(gpio->context, (u8*)&reg, block))
        *data = reg & (1 << bit);

    return ret;
}

/* Memory helpers */

struct memblk
{
    u32 size;
    struct memblk *next;
};

#define MEMHEAD(addr, offset) \
    ((struct memblk *)((u32)(addr) + (s32)(offset) - sizeof(struct memblk)))
#define MEMBODY(addr, offset) \
    ((void *)((u32)(addr) + (s32)(offset) + sizeof(struct memblk)))
#define MEMTOTAL(size) \
    ((size_t)size + sizeof(struct memblk))
#define MEMEND(head) \
    ((struct memblk *)((u32)(head) + MEMTOTAL(((struct memblk *)head)->size)))

static struct memblk *head = NULL;
extern struct memblk __free;
static void
init_mem_new(void)
{
    head = &__free;
    head->size = CONFIG_RAM_ADDRESS + CONFIG_RAM_SIZE + CONFIG_STACK_SIZE;
    head->size -= (u32)MEMBODY(head, 0);
    head->next = NULL;
}

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
            mem_init(ret, 0, size);
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
                new->size = (u32)MEMEND(target) - MEMTOTAL(new);
                new->next = next;
                next = new;
            }
            else
                blk->size = avaliable;

            if (prev == NULL)
                head = next;
            else
                prev->next = next;

            mem_init(&(((u8*)mem)[previous]), 0, blk->size - previous);
            ret = mem;
        }
        else
        {
            ret = mem_new(size);
            if (mem != NULL && ret != NULL)
            {
                mem_copy(ret, mem, blk->size);
                mem_del(mem);
            }
        }
    }
    else
    {
        ret = mem;
        if ((blk->size - size) >= 32)
        {
            blk->size = size;

            struct memblk *new = MEMEND(blk);
            new->size = (u32)MEMEND(blk) - MEMTOTAL(new);
            new->next = blk->next;
            blk->next = new;
        }
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
mem_comp(const void *mem, const void *mem2, size_t length)
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
mem_find(const void *mem, int c, size_t length)
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

extern void
mem_init(void *mem, int c, size_t length)
{
    for (size_t i = 0; i < length; i++)
        ((u8*)mem)[i] = c;
}

extern void
mem_copy(void *dest, void *src, size_t length)
{
    for (size_t i = 0; i < length; i++)
        ((u8*)dest)[i] = ((u8*)src)[i];
}

extern void
mem_move(void *dest, void *src, size_t length)
{
    if (((u32)src + length) > (u32)dest)
    {
        for (size_t i = length; i > 0; i--)
            ((u8*)dest)[i-1] = ((u8*)src)[i-1];
    }
    else
        mem_copy(dest, src, length);
}

/* String helpers */

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

extern char *
str_find_l(const char *str, int c)
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
str_find_r(const char *str, int c)
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

    length = (length != 0) ? length : SIZE_MAX;
    if (length > l)
    {
        mem_copy(dest, src, l);
        mem_init(&(dest[l]), '\0', length - l);
    }
    else
        mem_copy(dest, src, length);
}

extern void
str_concat(char *dest, char *src, size_t length)
{
    size_t l = str_length(dest);
    size_t l2 = str_length(src);

    length = (length != 0) ? length : SIZE_MAX;
    if (length <= l)
    {
        mem_copy(&(dest[l]), src, length);
        dest[l + length] = '\0';
    }
    else
        mem_copy(&(dest[l]), src, l2);
}

/* Initialization helpers */

extern void
_utils_init(void)
{
    init_mem_new();
}

extern void
_utils_clean(void)
{
    return;
}
