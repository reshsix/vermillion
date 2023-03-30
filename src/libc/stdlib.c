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

#include <signal.h>
#include <stdlib.h>
#include <_types.h>
#include <_utils.h>

#include <vermillion/drivers.h>

/* Initialization */

#define R_PRCM 0x01F01400
#define APB0_GATE *(volatile u32*)(R_PRCM + 0x28)
extern void exit(int);
extern int kernel_main(void);
static void init_malloc(void);
extern void
__init(void)
{
    int ret = 0;

    init_malloc();

    _drivers_init(DRIVER_TYPE_SERIAL);
    const struct driver *serial = driver_find(DRIVER_TYPE_SERIAL, 0);
    serial->routines.serial.config(115200, DRIVER_SERIAL_CHAR_8B,
                                   DRIVER_SERIAL_PARITY_NONE,
                                   DRIVER_SERIAL_STOP_1B);
    print("\r\nVermillion ");
    print(__VERMILLION__);
    print(" (");
    print(__COMPILATION__);
    print(")\r\n\r\n");

    _drivers_init(DRIVER_TYPE_GPIO);
    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.cfgpin(CONFIG_LED_SUCCESS_PIN,
                               DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.set(CONFIG_LED_SUCCESS_PIN, true);

    _drivers_init(DRIVER_TYPE_GIC);
    _drivers_init(DRIVER_TYPE_SPI);
    _drivers_init(DRIVER_TYPE_TIMER);
    _drivers_init(DRIVER_TYPE_VIDEO);
    _drivers_init(DRIVER_TYPE_AUDIO);
    _drivers_init(DRIVER_TYPE_STORAGE);
    _drivers_init(DRIVER_TYPE_FS);
    _drivers_init(DRIVER_TYPE_LOADER);

    print("Executing kernel_main\r\n");
    ret = kernel_main();

    _drivers_clean(DRIVER_TYPE_LOADER);
    _drivers_clean(DRIVER_TYPE_FS);
    _drivers_clean(DRIVER_TYPE_STORAGE);
    _drivers_clean(DRIVER_TYPE_AUDIO);
    _drivers_clean(DRIVER_TYPE_VIDEO);
    _drivers_clean(DRIVER_TYPE_TIMER);
    _drivers_clean(DRIVER_TYPE_SPI);
    _drivers_clean(DRIVER_TYPE_GIC);
    _drivers_clean(DRIVER_TYPE_GPIO);
    _drivers_clean(DRIVER_TYPE_SERIAL);

    exit(ret);
}

/* Process control */

extern void
abort(void)
{
    raise(SIGABRT);
}

static void (*__atexit[32])(void) = {NULL};
static u8 __atexit_c = 0;
extern int
atexit(void (*f)(void))
{
    int ret = 0;

    if (__atexit_c < 32)
        __atexit[__atexit_c++] = f;
    else
        ret = -1;

    return ret;
}

extern void __attribute__((noreturn))
exit(int code)
{
    while (__atexit_c)
        __atexit[--__atexit_c]();

    print("\r\nExited with code: ");
    print_hex(code);

    const struct driver *gpio = driver_find(DRIVER_TYPE_GPIO, 0);
    gpio->routines.gpio.cfgpin(CONFIG_LED_FAILURE_PIN,
                               DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.set(CONFIG_LED_FAILURE_PIN, true);
    gpio->routines.gpio.cfgpin(CONFIG_LED_SUCCESS_PIN,
                               DRIVER_GPIO_OUT, DRIVER_GPIO_PULLOFF);
    gpio->routines.gpio.set(CONFIG_LED_SUCCESS_PIN, false);

    const struct driver *gic = driver_find(DRIVER_TYPE_GIC, 0);
    for (;;)
        gic->routines.gic.wait();
}

extern int
system(const char *command)
{
    return (command == NULL);
}

extern char *
getenv(const char *name)
{
    (void)name;
    return NULL;
}

/* Memory management */

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
init_malloc(void)
{
    head = &__free;
    head->size = CONFIG_RAM_ADDRESS + CONFIG_RAM_SIZE + CONFIG_STACK_SIZE;
    head->size -= (u32)MEMBODY(head, 0);
    head->next = NULL;
}

extern void *
malloc(size_t size)
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
        }
    }

    return ret;
}

extern void *
calloc(size_t n, size_t size)
{
    void *ret = malloc(n * size);

    if (ret != NULL)
        for (size_t i = 0; i < n * size; i++)
            ((u8*)ret)[i] = 0;

    return ret;
}

extern void free(void * mem);
extern void *
realloc(void *mem, size_t size)
{
    void *ret = NULL;

    struct memblk *blk = MEMHEAD(mem, 0);
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
            blk->size += avaliable - size;

        if (prev == NULL)
            head = next;
        else
            prev->next = next;

        ret = mem;
    }
    else
    {
        ret = malloc(size);
        if (mem != NULL && ret != NULL)
        {
            for (size_t i = 0; i < blk->size; i++)
                ((u8*)ret)[i] = ((u8*)mem)[i];
            free(mem);
        }
    }

    return ret;
}

extern void
free(void *mem)
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
}

/* Numeric conversion */

/* Miscellaneous */

static int rseed = 1;
extern int
rand(void)
{
    rseed = (rseed * 1103515245) + 12345;
    int ret = (rseed >> 16) % 2048;

    for (int i = 0; i < 2; i++)
    {
        rseed = (rseed * 1103515245) + 12345;
        ret = (ret << 10) ^ ((rseed >> 16) % 1024);
    }

    return ret;
}

extern void
srand(unsigned seed)
{
    rseed = seed;
}

extern int
abs(int x)
{
    return (x < 0) ? -x : x;
}

extern long
labs(long x)
{
    return (x < 0) ? -x : x;
}

extern div_t
div(int a, int b)
{
    return (div_t){.quot = a / b, .rem = a % b};
}

extern ldiv_t
ldiv(long a, long b)
{
    return (ldiv_t){.quot = a / b, .rem = a % b};
}
