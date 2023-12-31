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
#include <core/utils.h>

#include <core/dev.h>
#include <core/drv.h>
#include <core/log.h>
#include <core/mem.h>
#include <core/str.h>
#include <core/fork.h>
#include <core/mutex.h>
#include <core/state.h>
#include <core/thread.h>
#include <core/channel.h>
#include <core/critical.h>
#include <core/generator.h>
#include <core/semaphore.h>

static void *test_ctx = (void *)0xABCD;

/* Testing log helpers */

static char buffer[128] = {0};
static u32 buffer_s = 0;

static void
test_log_clear(void)
{
    mem_init(buffer, 0, sizeof(buffer));
    buffer_s = 0;
}

static bool
test_log_write(void *ctx, u32 idx, void *data)
{
    bool ret = (ctx == test_ctx && idx == 0);

    if (ret)
        buffer[buffer_s++] = *((u8*)(data));

    return ret;
}

static char *test_log_str[] = {"log_set_dev", "log_get_dev",
                               "log_char", "log_string", "log_bool",
                               "log_unsigned", "log_signed"};
static u32
test_log(void)
{
    u32 ret = 0;

    drv_stream logdrv = { .stream.write = test_log_write };
    dev_stream logdev = { .context = test_ctx, .driver = &logdrv };

    dev_stream *logger = log_get_dev();
    log_set_dev(&logdev);

    if (log_get_dev() != &logdev)
        ret |= 0x1 | 0x2;

    log((char)'a');    if (buffer[0] != 'a')                   ret |= 0x4;
    test_log_clear();
    log("bcd");        if (str_comp(buffer, "bcd", 0) != 0)    ret |= 0x8;
    test_log_clear();
    log((bool)true);   if (str_comp(buffer, "true", 0) != 0)   ret |= 0x10;
    test_log_clear();
    log((u64)0x2345678910); if (str_comp(buffer, "0x2345678910", 0))
                                ret |= 0x20;
    test_log_clear();
    log((s64)-12345678901); if (str_comp(buffer, "-12345678901", 0) != 0)
                                ret |= 0x40;

    log_set_dev(logger);

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
clk_write(void *ctx, u32 idx, void *data, u32 block)
{
    bool ret = (ctx == test_ctx && idx == 0 && data && block == 0);

    if (ret)
    {
        u32 t = 0;
        mem_copy(&t, data, sizeof(u32));
        elapsed += t;
    }

    return ret;
}

static char *test_clock_str[] = {"clock", "csleep", "usleep",
                                 "msleep", "sleep"};
static u32
test_clock(void)
{
    u32 ret = 0;

    drv_timer clk = {.config.get = clk_getcfg, .block.write = clk_write};
    dev_timer tmr = {.context = test_ctx, .driver = &clk};

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
io_write(void *ctx, u32 idx, void *data, u32 block)
{
    bool ret = (ctx == test_ctx && idx == 0 && data && block < 4);

    if (ret)
        mem_copy(&(io_ports[block]), data, sizeof(u32));

    return ret;
}

static bool
io_read(void *ctx, u32 idx, void *data, u32 block)
{
    bool ret = (ctx == test_ctx && idx == 0 && data && block < 4);

    if (ret)
        mem_copy(data, &(io_ports[block]), sizeof(u32));

    return ret;
}

static char *test_io_str[] = {"pin_set", "pin_get", "pin_cfg"};
static u32
test_io(void)
{
    u32 ret = 0;

    drv_gpio drv = {.config.get = io_getcfg, .block.read = io_read,
                                             .block.write = io_write};
    dev_gpio gpio = {.context = test_ctx, .driver = &drv};
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
static u32
test_alloc(void)
{
    u32 ret = 0;

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
static u32
test_memory(void)
{
    u32 ret = 0;

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
static u32
test_string(void)
{
    u32 ret = 0;

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

/* Testing state helpers */

static char *test_state_str[] = {"state_new", "state_del",
                                 "state_save", "state_load"};
static u32
test_state(void)
{
    u32 ret = 0;

    state *st = state_new();
    state *pst = st;
    if (st == NULL)
        ret |= 0x1;
    st = state_del(st);
    if (st != NULL)
        ret |= 0x2;
    st = state_new();
    if (st == NULL || st != pst)
        ret |= 0x1;

    volatile void *test = (void*)987654321;
    register volatile void *test2 = (void*)0xAABBCC;
    ret |= 0x4 | 0x8;

    void *x = state_save(st);
    if (x == NULL)
    {
        ret &= ~0x4;
        state_load(st, (void*)123456789);
    }
    else if (x == (void*)123456789)
        ret &= ~0x8;
    else
        ret |= 0x4 | 0x8;

    if (test != (void*)987654321 && test2 != (void*)0xAABBCC)
        ret |= 0x4 | 0x8;

    return ret;
}

/* Testing forking helpers */

static void *result = 0;

static void
test_fork_f2(void *arg)
{
    result = arg;
}

static void
test_fork_f1(void *arg)
{
    fork *fk = fork_new(test_fork_f2, arg);
    if (fk)
        fork_run(fk);
    fork_del(fk);
}

static void
test_fork_f0(void *arg)
{
    fork *fk = fork_new(test_fork_f1, arg);
    if (fk)
        fork_run(fk);
    fork_del(fk);
}

static char *test_fork_str[] = {"fork_new", "fork_del", "fork_run"};
static u32
test_fork(void)
{
    u32 ret = 0;

    fork *fk = fork_new(test_fork_f0, (void*)0x12345);
    fork *pfk = fk;
    if (fk == NULL)
        ret |= 0x1;
    fk = fork_del(fk);
    if (fk != NULL)
        ret |= 0x2;
    fk = fork_new(test_fork_f0, (void*)0x12345);
    if (fk == NULL || fk != pfk)
        ret |= 0x1;

    fork_run(fk);
    if (result != (void*)0x12345)
        ret |= 0x4;
    fork_del(fk);

    return ret;
}

/* Testing generator helpers */

static char *test_generator_str[] = {"generator_new", "generator_del",
                                     "generator_next", "generator_rewind",
                                     "generator_arg", "generator_yield",
                                     "generator_finish"};

static noreturn
test_generator_g0(generator *g)
{
    u32 *ret = generator_arg(g);

    *ret &= ~0x8;
    generator_yield(g);

    *ret &= ~0x4;
    generator_yield(g);

    *ret &= ~0x10;
    generator_yield(g);

    generator_yield(g);
    generator_yield(g);
    generator_yield(g);
    *ret &= ~0x20;

    generator_finish(g);
}

static u32
test_generator(void)
{
    u32 ret = 0;

    generator *g = generator_new(test_generator_g0, &ret);
    generator *pg = g;
    if (g == NULL)
        ret |= 0x1;
    g = generator_del(g);
    if (g != NULL)
        ret |= 0x2;
    g = generator_new(test_generator_g0, &ret);
    if (g == NULL || g != pg)
        ret |= 0x1;

    ret |= 0x4;
    if (!generator_next(g) || !generator_next(g))
        ret |= 0x4;

    ret |= 0x8;
    generator_rewind(g);
    if (!generator_next(g) || !generator_next(g))
        ret |= 0x4;

    ret |= 0x10;
    if (!generator_next(g))
        ret |= 0x4;

    ret |= 0x20;
    if (!generator_next(g) || !generator_next(g) || !generator_next(g))
        ret |= 0x4;

    if (generator_next(g))
        ret |= 0x40;
    generator_del(g);

    return ret;
}

/* Testing thread helpers */

static char *test_thread_str[] = {"thread_new", "thread_del",
                                  "thread_sync", "thread_wait",
                                  "thread_rewind", "thread_arg",
                                  "thread_block", "thread_yield",
                                  "thread_loop", "thread_finish"};

static bool test_thread_loop = false;
static bool test_thread_finish = false;
thread_task (test_thread_t0)
{
    u32 *ret = thread_arg();

    if (!test_thread_finish)
    {
        if (!test_thread_loop)
            *ret &= ~(0x10 | 0x20); /* thread_rewind + thread_arg */
        else
            *ret &= ~0x100; /* thread_loop */

        thread_yield();
        thread_block(true);
        thread_yield();
        *ret &= ~(0x40 | 0x80); /* thread_block + thread_yield */
        thread_block(false);
        thread_yield();

        *ret &= ~(0x4 | 0x20); /* thread_sync + thread_arg */
        thread_yield();

        thread_yield();
        thread_yield();
        thread_yield();
        thread_yield();
        thread_yield();
        thread_yield();

        *ret &= ~0x8; /* thread_wait */
    }

    if (!test_thread_loop)
    {
        *ret &= ~(0x100 | 0x200); /* thread_loop + thread_finish */
        thread_finish();
    }
    else
    {
        test_thread_loop = false;
        test_thread_finish = true;
        thread_loop();
    }
}

static u32
test_thread(void)
{
    u32 ret = 0;

    thread *t = thread_new(test_thread_t0, &ret, false, 98);
    if (t != (thread *)0x1)
        ret |= 0x1;
    t = thread_del(t);
    if (t != NULL)
        ret |= 0x2;

    t = thread_new(test_thread_t0, &ret, true, 76);
    thread *pt = t;
    t = thread_del(t);
    t = thread_new(test_thread_t0, &ret, true, 54);
    if (t == NULL || t == (thread *)0x1 || t != pt)
        ret |= 0x1;

    ret |= 0x4;
    if (thread_sync(t, 3) < 3)
        ret |= 0x4;

    ret |= 0x8;
    if (!thread_wait(t))
        ret |= 0x8;

    ret |= 0x10 | 0x20;
    if (!thread_rewind(t))
        ret |= 0x10 | 0x20;
    thread_yield();

    ret |= 0x40 | 0x80;
    thread_yield();

    test_thread_loop = true;
    ret |= 0x100 | 0x200;
    if (!thread_wait(t))
        ret |= 0x100;

    return ret;
}

/* Testing synchronization helpers */

static char *test_sync_str[] = {"semaphore_wait", "semaphore_signal",
                                "mutex_lock", "mutex_unlock",
                                "critical_lock", "critical_unlock"};

static u8 test_sync_st0 = 0;
static bool test_sync_st1 = false;
static bool test_sync_st2[3] = {false};
static bool test_sync_st2r = true;

thread_task (test_sync_s0)
{
    u32 *arg = thread_arg();

    semaphore (5)
    {
        test_sync_st0++;

        for (u8 i = 0; i < 10; i++)
            thread_yield();

        if (test_sync_st0 > 5 || !test_sync_st0)
            *arg |= 0x1 + 0x2;

        test_sync_st0--;
    }

    thread_finish();
}

thread_task (test_sync_s1)
{
    u32 *arg = thread_arg();

    mutex ()
    {
        test_sync_st1 = !test_sync_st1;
        for (u8 i = 0; i < 10; i++)
            thread_yield();

        if (!test_sync_st1)
            *arg |= 0x4 + 0x8;

        test_sync_st1 = !test_sync_st1;
    }

    thread_finish();
}

thread_task (test_sync_s2)
{
    u32 arg = (u32)thread_arg();

    mutex (arg)
    {
        if (test_sync_st2[arg])
            test_sync_st2r = false;

        test_sync_st2[arg] = !test_sync_st2[arg];
        for (u8 i = 0; i < 10; i++)
            thread_yield();

        if (!test_sync_st2[arg])
            test_sync_st2r = false;

        test_sync_st2[arg] = !test_sync_st2[arg];
    }

    thread_finish();
}

thread_task (test_sync_s3)
{
    u32 arg = (u32)thread_arg();

    critical
    {
        for (u32 i = 0; i < arg; i++)
            thread_yield();
    }

    thread_finish();
}

static u32
test_sync(void)
{
    u32 ret = 0;

    thread *th[16] = {NULL};
    for (u8 i = 0; i < 16; i++)
        th[i] = thread_new(test_sync_s0, &ret, true, 123);
    for (u8 i = 0; i < 16; i++)
        thread_wait(th[i]);
    for (u8 i = 0; i < 16; i++)
        th[i] = thread_del(th[i]);

    for (u8 i = 0; i < 16; i++)
        th[i] = thread_new(test_sync_s1, &ret, true, 45);
    for (u8 i = 0; i < 16; i++)
        thread_wait(th[i]);
    for (u8 i = 0; i < 16; i++)
        th[i] = thread_del(th[i]);

    for (u8 i = 0; i < 16; i++)
        th[i] = thread_new(test_sync_s2, (void *)(i % 3), true, 67);
    for (u8 i = 0; i < 16; i++)
        thread_wait(th[i]);
    for (u8 i = 0; i < 16; i++)
        th[i] = thread_del(th[i]);
    if (!test_sync_st2r)
        ret |= 0x4 + 0x8;

    th[0] = thread_new(test_sync_s3, (void *)10, true, 89);
    if (thread_sync(th[0], 1) != 11)
        ret |= 0x10 + 0x20;
    th[0] = thread_del(th[0]);

    return ret;
}

/* Testing inter-thread communication */

static char *test_channel_str[] = {"channel_new",  "channel_del",
                                   "channel_read", "channel_write"};

thread_task (test_channel_c0)
{
    channel *ch = thread_arg();

    u32 test = 0;
    channel_read(ch, &test);
    if (test == 0xAB)
    {
        test = 0xBC;
        channel_write(ch, &test);
        test = 0xCD;
        channel_write(ch, &test);
    }

    channel_read(ch, &test);
    if (test == 0xDE)
    {
        test = 0xFA;
        channel_write(ch, &test);
    }

    thread_finish();
}

static u32
test_channel(void)
{
    u32 ret = 0;

    channel *ch = channel_new(sizeof(u32), 0);
    channel *pch = ch;
    if (ch == NULL)
        ret |= 0x1;
    ch = channel_del(ch);
    if (ch != NULL)
        ret |= 0x2;
    ch = channel_new(sizeof(u32), 0);
    if (ch == NULL || ch != pch)
        ret |= 0x1;

    thread_new(test_channel_c0, ch, false, 123);

    u32 test = 0xAB;
    channel_write(ch, &test);
    channel_read(ch, &test);
    if (test != 0xBC)
        ret |= 0x4 | 0x8;

    channel_read(ch, &test);
    if (test != 0xCD)
        ret |= 0x4 | 0x8;

    test = 0xDE;
    channel_write(ch, &test);
    channel_read(ch, &test);
    if (test != 0xFA)
        ret |= 0x4 | 0x8;

    return ret;
}

/* Init tests */

extern void
main(void)
{
    log("\r\n[ Testing libk.o ]\r\n");
    #define UNIT_TEST(x) \
    { \
        u32 result = x(); \
        for (u8 i = 0; i < sizeof(x##_str) / sizeof(char *); i++) \
        { \
            log("  "); \
            log(x##_str[i]); \
            log("(): "); \
            log(!(result & (1 << i)) ? "Pass" : "Failed"); \
            log("\r\n"); \
        } \
        log("\r\n"); \
    }

    UNIT_TEST(test_log)
    UNIT_TEST(test_clock)
    UNIT_TEST(test_io)
    UNIT_TEST(test_alloc)
    UNIT_TEST(test_memory)
    UNIT_TEST(test_string)
    UNIT_TEST(test_state)
    UNIT_TEST(test_fork)
    UNIT_TEST(test_generator)
    UNIT_TEST(test_thread)
    UNIT_TEST(test_sync)
    UNIT_TEST(test_channel)
}
