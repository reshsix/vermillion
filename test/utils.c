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

static void *test_ctx = (void *)0xABCD;

/* Testing log helpers */

static char buffer[128] = {0};
static u32 buffer_s = 0;

static void
log_clear(void)
{
    mem_init(buffer, 0, sizeof(buffer));
    buffer_s = 0;
}

static bool
log_write(void *ctx, u8 c)
{
    bool ret = false;

    if (ctx == test_ctx)
    {
        buffer[buffer_s++] = c;
        ret = true;
    }

    return ret;
}

static char *test_log_str[] = {"log_c", "log_s", "log_h", "log_u"};
static u32
test_log(void)
{
    u32 ret = 0;

    struct driver logdrv = { .interface.stream.write = log_write };
    struct device logdev = { .context = test_ctx, .driver = &logdrv };

    struct device *log = logger(NULL);
    logger(&logdev);
    log_clear();

    log_c('a');
    if (buffer[0] != 'a')
        ret |= 0x1;
    log_clear();

    log_s("bcd");
    if (str_comp(buffer, "bcd", 0) != 0)
        ret |= 0x2;
    log_clear();

    log_h(0xABCDEF);
    if (str_comp(buffer, "0xABCDEF", 0) != 0)
        ret |= 0x4;
    log_clear();

    log_u(123456789);
    if (str_comp(buffer, "123456789", 0) != 0)
        ret |= 0x8;
    log_clear();

    logger(log);

    return ret;
}

/* Testing timing helpers */

static u64 elapsed = 0;

static bool
clk_getcfg(void *ctx, union config *cfg)
{
    bool ret = false;

    if (ctx == test_ctx && cfg)
    {
        cfg->timer.clock = 10000000;
        ret = true;
    }

    return ret;
}

static bool
clk_write(void *ctx, u8 *data, u32 block)
{
    bool ret = false;

    if (ctx == test_ctx && data && block == 0)
    {
        u32 t = 0;
        mem_copy(&t, data, sizeof(u32));
        elapsed += t;
        ret = true;
    }

    return ret;
}

static char *test_clock_str[] = {"clock", "csleep", "usleep",
                                 "msleep", "sleep"};
static u32
test_clock(void)
{
    u32 ret = 0;

    struct driver clk = {.config.get = clk_getcfg,
                         .interface.block.write = clk_write};
    struct device tmr = {.context = test_ctx, .driver = &clk};

    u32 rate = clock(&tmr);
    if (rate != 10000000)
        ret |= 0x1;

    csleep(&tmr, 1234);
    if (elapsed != 1234)
        ret |= 0x2;
    elapsed = 0;

    usleep(&tmr, 12);
    if (elapsed != 120)
        ret |= 0x4;
    elapsed = 0;

    msleep(&tmr, 12);
    if (elapsed != 120000)
        ret |= 0x8;
    elapsed = 0;

    sleep(&tmr, 12);
    if (elapsed != 120000000)
        ret |= 0x10;
    elapsed = 0;

    return ret;
}

/* Testing IO helpers */

static u32 io_ports[4] = {0};

static bool
io_pin(void *ctx, u16 pin, u8 role, u8 pull)
{
    bool ret = false;

    (void)pin, (void)role, (void)pull;
    if (ctx == test_ctx)
        ret = true;

    return ret;
}

static bool
io_getcfg(void *ctx, union config *cfg)
{
    bool ret = false;

    if (ctx == test_ctx && cfg)
    {
        cfg->gpio.pin = io_pin;
        ret = true;
    }

    return ret;
}

static bool
io_write(void *ctx, u8 *data, u32 block)
{
    bool ret = false;

    if (ctx == test_ctx && data && block < 4)
    {
        mem_copy(&(io_ports[block]), data, sizeof(u32));
        ret = true;
    }

    return ret;
}

static bool
io_read(void *ctx, u8 *data, u32 block)
{
    bool ret = false;

    if (ctx == test_ctx && data && block < 4)
    {
        mem_copy(data, &(io_ports[block]), sizeof(u32));
        ret = true;
    }

    return ret;
}

static char *test_io_str[] = {"pin_set", "pin_get", "pin_cfg"};
static int
test_io(void)
{
    int ret = 0;

    struct driver drv = {.config.get = io_getcfg,
                         .interface.block.read = io_read,
                         .interface.block.write = io_write};

    struct device gpio = {.context = test_ctx, .driver = &drv};
    io_ports[3] = 0xFFFFFFFF;

    if (!pin_set(&gpio, 123, false) ||
        io_ports[3] != (0xFFFFFFFF & ~(1 << (123 % 32))) ||
        !pin_set(&gpio, 123, true) || io_ports[3] != 0xFFFFFFFF)
        ret |= 0x1;

    bool data = true;
    if (!pin_get(&gpio, 64, &data) || data ||
        !pin_get(&gpio, 99, &data) || !data)
        ret |= 0x2;

    if (!pin_cfg(&gpio, 30, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLDOWN))
        ret |= 0x4;

    return ret;
}

/* Testing memory allocation helpers */

static char *test_alloc_str[] = {"mem_new", "mem_renew", "mem_del"};
static int
test_alloc(void)
{
    int ret = 0;

    u8 *a = mem_new(10);
    u8 *b = mem_new(20);
    u8 *c = mem_new(30);
    u8 *d = mem_new(40);
    u8 *e = mem_new(50);
    if (!a || !b || !c || !d || !e ||
        a + 10 > b || b + 20 > c ||
        c + 30 > d || d + 40 > e)
        ret |= 0x1;

    if (!(ret & 0x1))
    {
        for (u8 i = 0; i < 50; i++)
        {
            if ((i < 10 && a[i]) || (i < 20 && b[i]) ||
                (i < 30 && c[i]) || (i < 40 && d[i]) ||
                (i < 50 && e[i]))
            {
                ret |= 0x1;
                break;
            }
        }
    }

    u8 *f = mem_renew(a, 100);
    if (f && b >= f && b <= (f - 100))
        ret |= 0x2;

    if (!(ret & 0x2))
    {
        for (u8 i = 0; i < 100; i++)
        {
            if (f[i])
            {
                ret |= 0x2;
                break;
            }
        }
    }

    mem_del(b);
    mem_del(c);
    mem_del(d);
    mem_del(e);
    mem_del(f);
    b = mem_new(10);
    if (b != a)
        ret |= 0x4;

    return ret;
}

/* Testing memory logic helpers */

static char *test_memory_str[] = {"mem_comp", "mem_find",
                                  "mem_init", "mem_copy"};
static int
test_memory(void)
{
    int ret = 0;

    static u8 a[128] = {0};
    static u8 b[128] = {0};

    if (mem_comp(a, b, 128) != 0)
        ret |= 0x1;

    a[127] = 1;
    if (mem_comp(a, b, 128) == 0)
        ret |= 0x1;

    if (mem_find(a, 1, 128) != &(a[127]))
        ret |= 0x2;

    mem_init(a, 2, 128);
    for (u8 i = 0; i < 128; i++)
    {
        if (a[i] != 2)
        {
            ret |= 0x4;
            break;
        }
    }

    mem_copy(b, a, 128);
    for (u8 i = 0; i < 128; i++)
    {
        if (b[i] != a[i])
        {
            ret |= 0x8;
            break;
        }
    }

    if (!(ret & 0x8))
    {
        a[0] = 1;
        a[1] = 2;
        a[2] = 3;

        mem_copy(&(a[1]), a, 127);
        if (a[1] != 1 || a[2] != 2 || a[3] != 3)
            ret |= 0x8;
    }

    return ret;
}

/* Testing string helpers */

static char *test_string_str[] = {"str_length", "str_comp", "str_span",
                                  "str_find_l", "str_find_r",
                                  "str_find_m", "str_find_s",
                                  "str_token", "str_copy",
                                  "str_concat", "str_dupl"};
static int
test_string(void)
{
    int ret = 0;

    char str[] = "test1234";
    char str2[] = "test5678";

    if (str_length(str) != sizeof(str) - 1)
        ret |= 0x1;

    if (str_comp(str, str2, 0) == 0)
        ret |= 0x2;

    if (!(ret & 0x2) && str_comp(str, str2, 4) != 0)
        ret |= 0x2;

    if (str_span(str, "tes", false) != 4)
        ret |= 0x4;

    if (!(ret & 0x10) && str_span(str, "234", true) != 5)
        ret |= 0x4;

    if (str_find_l(str, 'e') != &(str[1]))
        ret |= 0x8;

    if (str_find_r(str, 't') != &(str[3]))
        ret |= 0x10;

    if (str_find_m(str, "abs12") != &(str[2]))
        ret |= 0x20;

    if (str_find_s(str, "34") != &(str[6]))
        ret |= 0x40;

    char *state = NULL;
    if (str_token(str, "15", &state) != str || str[4] != '\0' ||
        state != &(str[5]))
        ret |= 0x80;

    if (!(ret & 0x80))
    {
        if (str_token(NULL, "15", &state) != &(str[5]) || state != &(str[8]))
            ret |= 0x80;
    }

    char buf[32] = {0}, zeros[32] = {0};
    str_copy(buf, str2, 0);
    ret |= (mem_comp(buf, str2, sizeof(str2))) ? 0x100 : 0x0;

    if (!(ret & 0x100))
    {
        char buf2[32] = {0};
        str_copy(buf2, str2, 4);
        ret |= (mem_comp(buf2, str2, 4) ||
                mem_comp(&(buf2[4]), zeros, sizeof(buf) - 4)) ? 0x100 : 0x0;
    }

    if (!(ret & 0x100))
    {
        char buf2[32] = {[0 ... 31] = 0xFF};
        str_copy(buf2, str2, sizeof(buf2));
        ret |= (mem_comp(buf2, str2, sizeof(str2)) ||
                mem_comp(&(buf2[sizeof(str2)]), zeros,
                         sizeof(buf) - sizeof(str2)))
                ? 0x100 : 0x0;
    }

    char buf2[32] = "testing";
    str_concat(buf2, str2, 0);
    ret |= (mem_comp(buf2, "testing", 7) ||
            mem_comp(&(buf2[7]), str2, 15 - 7) ||
            mem_comp(&(buf2[15]), zeros, sizeof(buf2) - 15)) ? 0x200 : 0x0;

    if (!(ret & 0x200))
    {
        char buf3[32] = "testing";
        buf3[15] = 0xFF;

        str_concat(buf3, str2, 32);
        ret |= (mem_comp(buf3, "testing", 7) ||
                mem_comp(&(buf3[7]), str2, 15 - 7) ||
                mem_comp(&(buf3[15]), zeros, sizeof(buf3) - 15)) ? 0x200 : 0x0;
    }

    if (!(ret & 0x200))
    {
        char buf3[32] = "testing";
        buf3[11] = 0xFF;

        str_concat(buf3, str2, 4);
        ret |= (mem_comp(buf3, "testing", 7) ||
                mem_comp(&(buf3[7]), str2, 11 - 7) ||
                mem_comp(&(buf3[11]), zeros, sizeof(buf3) - 15)) ? 0x200 : 0x0;
    }

    char *dup = str_dupl(str2, 0);
    ret |= (dup == str2 || mem_comp(dup, str2, sizeof(str2)))
           ? 0x400 : 0x0;
    mem_del(dup);

    if (!(ret & 0x400))
    {
        dup = str_dupl(str2, 4);
        ret |= (dup == str2 || mem_comp(dup, str2, 4) || dup[5])
               ? 0x400 : 0x0;
    }

    return ret;
}

/* Init tests */

extern int
main(void)
{
    log_s("\r\n[ Testing utils.o ]\r\n");
    #define UNIT_TEST(x) \
    { \
        u32 result = x(); \
        for (u8 i = 0; i < sizeof(x##_str) / sizeof(char *); i++) \
        { \
            log_s("  "); \
            log_s(x##_str[i]); \
            log_s("(): "); \
            log_s(!(result & (1 << i)) ? "Pass" : "Failed"); \
            log_s("\r\n"); \
        } \
        log_s("\r\n"); \
    }

    UNIT_TEST(test_log)
    UNIT_TEST(test_clock)
    UNIT_TEST(test_io)
    UNIT_TEST(test_alloc)
    UNIT_TEST(test_memory)
    UNIT_TEST(test_string)

    return 0;
}
