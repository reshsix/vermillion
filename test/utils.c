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
#include <core/mem.h>

#include <core/gpio.h>
#include <core/timer.h>

#include <debug/exit.h>
#include <debug/assert.h>

static void *test_ctx = (void *)0xABCD;

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

static void
test_clock(void)
{
    drv_timer clk = {.config.get = clk_getcfg, .write = clk_write};
    dev_timer tmr = {.context = test_ctx, .driver = &clk};

    u32 rate = clock(&tmr);
    assert (rate == 10000000);

    csleep(&tmr, 1234);
    assert (elapsed == 1234);
    elapsed = 0;

    usleep(&tmr, 12);
    assert (elapsed == 120);
    elapsed = 0;

    msleep(&tmr, 12);
    assert (elapsed == 120000);
    elapsed = 0;

    sleep(&tmr, 12);
    assert (elapsed == 120000000);
    elapsed = 0;
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

static void
test_io(void)
{
    drv_gpio drv = {.config.get = io_getcfg, .read = io_read,
                                             .write = io_write};
    dev_gpio gpio = {.context = test_ctx, .driver = &drv};
    io_ports[3] = 0xFFFFFFFF;

    assert (pin_set(&gpio, 123, false));
    assert (io_ports[3] == (0xFFFFFFFF & ~(1 << (123 % 32))));
    assert (pin_set(&gpio, 123, true));
    assert (io_ports[3] == 0xFFFFFFFF);

    bool data = true;
    assert (pin_get(&gpio, 64, &data));
    assert (!data);
    assert (pin_get(&gpio, 99, &data));
    assert (data);

    assert (pin_cfg(&gpio, 30, DRIVER_GPIO_OUT, DRIVER_GPIO_PULLDOWN));
}

extern void
main(void)
{
    test_clock();
    test_io();

    exit_qemu(assert_failed);
}
