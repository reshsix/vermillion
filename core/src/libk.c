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

/* Logging helpers */

static struct device *logdev = NULL;

extern struct device *
logger(struct device *log)
{
    if (log != NULL)
        logdev = log;

    return logdev;
}

extern void
log_c(const char c)
{
    if (logdev != NULL)
        logdev->driver->interface.stream.write(logdev->context, c);
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
    log_s("0x");
    if (n >= (1 << 24))
        log_h8(n >> 24);
    if (n >= (1 << 16))
        log_h8(n >> 16);
    if (n >= (1 << 8))
        log_h8(n >> 8);
    log_h8(n);
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
    {
        *data = reg & (1 << bit);
        ret = true;
    }

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
init_mem(void)
{
    head = &__free;
    head->size = CONFIG_RAM_ADDRESS + CONFIG_RAM_SIZE + CONFIG_STACK_SIZE;
    head->size -= (u32)MEMBODY(head, 0);
    head->next = NULL;
}

static void
clean_mem(void)
{
    return;
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
        if (mem)
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
mem_find(const void *mem, u8 c, size_t length)
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
mem_init(void *mem, u8 c, size_t length)
{
    for (size_t i = 0; i < length; i++)
        ((u8*)mem)[i] = c;
}

extern void
mem_copy(void *dest, const void *src, size_t length)
{
    if (((u32)src + length) > (u32)dest)
    {
        for (size_t i = length; i > 0; i--)
            ((u8*)dest)[i-1] = ((u8*)src)[i-1];
    }
    else if (dest != src)
    {
        for (size_t i = 0; i < length; i++)
            ((u8*)dest)[i] = ((u8*)src)[i];
    }
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

/* For GCC optimizations */

extern void
memcpy(void * restrict dest, const void * restrict src, size_t length)
{
    mem_copy(dest, src, length);
}

extern void
memmove(void *dest, const void *src, size_t length)
{
    mem_copy(dest, src, length);
}

extern void
memset(void *mem, int c, size_t length)
{
    mem_init(mem, c, length);
}

extern int
memcmp(const void *mem, const void *mem2, size_t length)
{
    return mem_comp(mem, mem2, length);
}

/* Processor state helpers */

struct __attribute__((packed)) state
{
    #if defined(CONFIG_ARCH_ARM)
    void *gpr[12];
    #elif defined(CONFIG_ARCH_I686)
    void *gpr[6], *retaddr;
    #endif
};

extern struct state *
state_new(void)
{
    struct state *ret = mem_new(sizeof(struct state));
    return ret;
}

extern struct state *
state_del(struct state *st)
{
    return mem_del(st);
}

extern void * __attribute__((naked))
state_save(struct state *st)
{
    (void)st;

    #if defined(CONFIG_ARCH_ARM)

    /* Call parameters: st -> r0 */

    /* Saving low callee-saved registers in st->gpr[0 -> 3] */
    asm volatile ("stmia r0!, {r4-r7}");
    /* Saving high callee-saved registers in st->gpr[4 -> 10] */
    asm volatile ("mov r1,  r8\n" "mov r2,  r9\n" "mov r3, r10\n" \
                  "mov r4, r11\n" "mov r5, r12\n" "mov r6, r13\n" \
                  "mov r7, r14\n");
    asm volatile ("stmia r0!, {r1-r7}");
    /* Restoring low registers */
    asm volatile ("sub r0, r0, #44");
    asm volatile ("ldmia r0!, {r4-r7}");
    /* Setting r1 to st->gpr[11] */
    asm volatile ("mov r1, r0");
    asm volatile ("add r1, r1, #28");
    /* Setting r0 to NULL for first return */
    asm volatile ("mov r0, $0");
    /* Saving program counter in st->gpr[11] */
    asm volatile ("str pc, [r1]");
    /* Return with NULL or the value from state_load */
    asm volatile ("bx lr");
    /* Again if coming from state_load */
    asm volatile ("bx lr");

    #elif defined(CONFIG_ARCH_I686)

    /* Call parameters st -> [esp + 4] */

    /* Moving st to edx */
    asm volatile ("movl 4(%esp), %edx");
    /* Saving all callee-saved registers */
    asm volatile ("movl %ebx, (%edx)");
    asm volatile ("movl %esi, 4(%edx)");
    asm volatile ("movl %edi, 8(%edx)");
    asm volatile ("movl %ebp, 12(%edx)");
    asm volatile ("movl %esp, 16(%edx)");
    /* Saving return address */
    asm volatile ("movl (%esp), %eax");
    asm volatile ("movl %eax, 24(%edx)");
    /* Setting eax to NULL for first return */
    asm volatile ("xor %eax, %eax");
    /* Saving program counter */
    asm volatile ("movl $_ret, %ecx");
    asm volatile ("movl %ecx, 20(%edx)");
    asm volatile ("_ret:");
    /* Return with NULL or the value from state_load */
    asm volatile ("ret");

    #endif

    /* Supressing compiler warning */
    return NULL;
}

extern void __attribute__((naked, noreturn))
state_load(struct state *st, void *ret)
{
    (void)st, (void)ret;

    #if defined(CONFIG_ARCH_ARM)

    /* Call parameters: st -> r0, ret -> r1 */

    /* Loading high callee-saved registers */
    asm volatile ("add r0, r0, #16");
    asm volatile ("ldmia r0!, {r2-r7}");
    asm volatile ("mov r8,  r2\n" "mov r9,  r3\n" "mov r10, r4\n" \
                  "mov r11, r5\n" "mov r12, r6\n" "mov r13, r7\n");
    asm volatile ("ldr r2, [r0]");
    asm volatile ("mov r14, r2");
    /* Loading low callee-saved registers */
    asm volatile ("sub r0, r0, #40");
    asm volatile ("ldmia r0!, {r4-r7}");
    /* Setting r2 to program counter */
    asm volatile ("add r0, r0, #28");
    asm volatile ("ldr r2, [r0]");
    /* Setting r0 to ret */
    asm volatile ("mov r0, r1");
    /* Jumping to address */
    asm volatile ("mov pc, r2");

    #elif defined(CONFIG_ARCH_I686)

    /* Call parameters st -> [esp + 4], ret -> [esp + 8] */

    /* Moving st to edx and ret to ecx */
    asm volatile ("movl 4(%esp), %edx");
    asm volatile ("movl 8(%esp), %ecx");
    /* Loading all callee-saved registers */
    asm volatile ("movl (%edx),   %ebx");
    asm volatile ("movl 4(%edx),  %esi");
    asm volatile ("movl 8(%edx),  %edi");
    asm volatile ("movl 12(%edx), %ebp");
    asm volatile ("movl 16(%edx), %esp");
    /* Setting return address */
    asm volatile ("movl 24(%edx), %eax");
    asm volatile ("movl %eax, (%esp)");
    /* Loading address to jump */
    asm volatile ("movl 20(%edx), %eax");
    asm volatile ("movl %eax, %edx");
    /* Setting eax to ret */
    asm volatile ("movl %ecx, %eax");
    /* Jumping to address */
    asm volatile ("jmp *%edx");

    #endif

    /* Supressing compiler warning */
    while (true);
}

/* Forking helpers */

struct fork
{
    void *stack;
    void (*f)(void*), *arg;

    #if defined(CONFIG_ARCH_ARM)
    void *fp, *sp;
    #elif defined(CONFIG_ARCH_I686)
    void *ebp, *esp;
    #endif

    struct fork *previous;
};

extern struct fork *
fork_new(void (*f)(void *), void *arg)
{
    struct fork *ret = mem_new(sizeof(struct fork));

    if (ret)
    {
        ret->f = f;
        ret->arg = arg;

        ret->stack = mem_new(CONFIG_STACK_SIZE);
        if (!(ret->stack))
            ret = mem_del(ret);
    }

    return ret;
}

extern struct fork *
fork_del(struct fork *fk)
{
    if (fk)
        mem_del(fk->stack);

    return mem_del(fk);
}

extern void
fork_run(struct fork *fk)
{
    /* Saving both current and previous fork as static for recursion */
    static struct fork *sfk = NULL;
    fk->previous = sfk;
    sfk = fk;

    /* Pointer to the end/base of the stack */
    static void *stack = NULL;
    stack = (void*)((u32)sfk->stack + CONFIG_STACK_SIZE);

    #if defined(CONFIG_ARCH_ARM)

    /* Saving current stack */
    asm volatile ("mov %0, fp" : "=r"(sfk->fp));
    asm volatile ("mov %0, sp" : "=r"(sfk->sp));
    /* Setting stack to allocated address */
    asm volatile ("mov fp, %0" :: "r"(stack));
    asm volatile ("mov sp, fp");

    #elif defined(CONFIG_ARCH_I686)

    /* Saving current stack */
    asm volatile ("movl %%ebp, %0" : "=r"(sfk->ebp));
    asm volatile ("movl %%esp, %0" : "=r"(sfk->esp));
    /* Setting stack to allocated address */
    asm volatile ("movl %0, %%ebp" :: "r"(stack));
    asm volatile ("movl %ebp, %esp");

    #endif

    /* Jumping to function */
    sfk->f(sfk->arg);

    #if defined(CONFIG_ARCH_ARM)

    /* Turning the stack back to normal */
    asm volatile ("mov fp, %0" :: "r"(sfk->fp));
    asm volatile ("mov sp, %0" :: "r"(sfk->sp));

    #elif defined(CONFIG_ARCH_I686)

    /* Turning the stack back to normal */
    asm volatile ("movl %0, %%ebp" :: "r"(sfk->ebp));
    asm volatile ("movl %0, %%esp" :: "r"(sfk->esp));

    #endif

    /* Loading previous fork for next run */
    struct fork *prev = sfk->previous;
    sfk->previous = NULL;
    sfk = prev;
}

/* Generator helpers */

struct generator
{
    bool active, finished;

    void *arg;
    struct fork *fk;

    struct state *caller, *callee;
};

extern struct generator *
generator_new(void (*f)(struct generator *), void *arg)
{
    struct generator *ret = mem_new(sizeof(struct generator));

    if (ret)
    {
        ret->arg = arg;
        ret->fk = fork_new((void (*)(void *))f, ret);
        if (!(ret->fk))
            ret = generator_del(ret);
    }

    if (ret)
    {
        ret->caller = state_new();
        ret->callee = state_new();
        if (!(ret->caller && ret->callee))
            ret = generator_del(ret);
    }

    return ret;
}

extern struct generator *
generator_del(struct generator *g)
{
    if (g)
    {
        fork_del(g->fk);
        state_del(g->caller);
        state_del(g->callee);
    }

    return mem_del(g);
}

extern bool
generator_next(struct generator *g)
{
    if (!(g->finished))
    {
        if (state_save(g->caller) == NULL)
        {
            if (g->active)
                state_load(g->callee, (void*)0x1);
            else
            {
                g->active = true;
                fork_run(g->fk);
            }
        }
    }

    return !(g->finished);
}

extern void
generator_rewind(struct generator *g)
{
    g->active = false;
    g->finished = false;
}

extern void *
generator_arg(struct generator *g)
{
    return g->arg;
}

extern void
generator_yield(struct generator *g)
{
    if (state_save(g->callee) == NULL)
        state_load(g->caller, (void*)0x1);
}

extern noreturn
generator_finish(struct generator *g)
{
    g->finished = true;
    generator_yield(g);
    while (true);
}

/* Thread functions */

struct thread
{
    size_t step;
    bool persistent;
    struct generator *gen;
    u8 counter, priority;

    struct thread *prev, *next;
};

static struct
{
    struct thread *head, *cur, *tail;
    bool blocked;
} threads = {0};

extern struct thread *
thread_new(THREAD(f), void *arg, bool persistent, u8 priority)
{
    struct thread *ret = mem_new(sizeof(struct thread));

    if (ret)
    {
        ret->priority = priority;
        ret->persistent = persistent;
        ret->gen = generator_new(f, arg);
        if (!(ret->gen))
            ret = mem_del(ret);
    }

    if (ret)
    {
        if (threads.tail)
            threads.tail->next = ret;
        else
        {
            threads.head = ret;
            threads.cur = ret;
        }

        ret->prev = threads.tail;
        threads.tail = ret;
    }

    if (ret && !persistent)
        ret = (struct thread *)0x1;

    return ret;
}

extern struct thread *
thread_del(struct thread *t)
{
    if ((u32)t > 0x1)
    {
        if (t->prev)
            t->prev->next = t->next;
        if (t->next)
            t->next->prev = t->prev;

        if (threads.head == t)
            threads.head = (t->prev) ? t->prev : t->next;
        if (threads.cur == t)
            threads.cur = (t->next) ? t->next : threads.head;
        if (threads.tail == t)
            threads.tail = (t->next) ? t->next : threads.head;

        mem_del(t);
    }

    return NULL;
}

extern size_t
thread_sync(struct thread *t, size_t step)
{
    size_t ret = 0;

    threads.cur->step++;
    if (!threads.blocked && threads.cur != t && (u32)t > 0x1 && t->persistent)
    {
        while (t->step < step && !(t->gen->finished))
            generator_yield(threads.cur->gen);

        ret = t->step;
    }

    return ret;
}

extern size_t
thread_wait(struct thread *t)
{
    size_t ret = 0;

    threads.cur->step++;
    if (!threads.blocked && threads.cur != t && (u32)t > 0x1 && t->persistent)
    {
        while (!(t->gen->finished))
            generator_yield(threads.cur->gen);

        ret = t->step;
    }

    return ret;
}

extern bool
thread_rewind(struct thread *t)
{
    bool ret = false;

    if (!threads.blocked && threads.cur != t && (u32)t > 0x1 && t->persistent)
    {
        t->step = 0;
        generator_rewind(t->gen);
        ret = true;
    }

    return ret;
}

extern void *
thread_arg(void)
{
    return threads.cur->gen->arg;
}

extern void
thread_block(bool state)
{
    threads.blocked = state;
}

extern void
thread_yield(void)
{
    threads.cur->step++;
    if (!threads.blocked)
        generator_yield(threads.cur->gen);
}

extern noreturn
thread_loop(void)
{
    threads.cur->step = 0;
    generator_rewind(threads.cur->gen);

    threads.blocked = false;
    generator_yield(threads.cur->gen);
}

extern noreturn
thread_finish(void)
{
    threads.cur->step++;
    threads.blocked = false;
    generator_finish(threads.cur->gen);
}

/* Synchronization helpers */

extern void
semaphore_wait(int *s)
{
    while (*s == 0)
        thread_yield();
    (*s)--;
}

extern void
semaphore_signal(int *s)
{
    (*s)++;
}

extern void
mutex_lock(void **m, void *param)
{
    while (*m != NULL)
        thread_yield();

    *m = ((param) ? param : threads.cur);
}

extern void
mutex_unlock(void **m, void *param)
{
    if (*m == ((param) ? param : threads.cur))
        *m = NULL;
}

extern void
critical_lock(void)
{
    thread_block(true);
}

extern void
critical_unlock(void)
{
    thread_block(false);
}

/* Inter-thread communication */

struct channel
{
    size_t type, size, count;
    u8 *buffer;
};

extern struct channel *
channel_new(size_t type, size_t size)
{
    struct channel *ret = mem_new(sizeof(struct channel));

    if (ret)
    {
        ret->type = type;
        ret->size = size + 1;

        ret->buffer = mem_new(type * ret->size);
        if (!(ret->buffer))
            ret = mem_del(ret);
    }

    return ret;
}

extern struct channel *
channel_del(struct channel *ch)
{
    if (ch)
        mem_del(ch->buffer);

    return mem_del(ch);
}

extern void
channel_read(struct channel *ch, void *data)
{
    threads.cur->step++;
    if (!(threads.blocked))
    {
        while (ch->count == 0)
            generator_yield(threads.cur->gen);

        mem_copy(data, &(ch->buffer[--(ch->count)]), ch->type);
        generator_yield(threads.cur->gen);
    }
}

extern void
channel_write(struct channel *ch, void *data)
{
    threads.cur->step++;
    if (!(threads.blocked))
    {
        while (ch->count >= ch->size)
            generator_yield(threads.cur->gen);

        mem_copy(&(ch->buffer[ch->count++]), data, ch->type);
        while (ch->count >= ch->size)
            thread_yield();
    }
}

/* Initialization helpers */

static void
init_utils(void)
{
    init_mem();
}

static void
clean_utils(void)
{
    clean_mem();
}

/* Initialization function */

extern THREAD(main);
extern void
__init(void)
{
    init_utils();
    _devtree_init();

    thread_new(main, NULL, false, 255);
    while (threads.cur)
    {
        struct thread *cur = threads.cur;

        if (!((cur->counter * cur->priority) % 255))
        {
            if (generator_next(cur->gen))
                threads.cur = (cur->next) ? cur->next : threads.head;
            else
            {
                if (cur->persistent)
                    threads.cur = (cur->next) ? cur->next : threads.head;
                else
                    cur = thread_del(cur);
            }
        }

        if (cur)
            cur->counter++;
    }

    _devtree_clean();
    clean_utils();

    while (true);
}
