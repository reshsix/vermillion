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

#include <stdio.h>
#include <_types.h>

extern void
print(const char *s)
{
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            fputc(s[0], stdout);
    }
}

static void
print_h8(const u8 n)
{
    for (u8 i = 1; i <= 1; i--)
    {
        u8 x = (n >> (i * 4)) & 0xF;
        if (x < 10)
            fputc(x + '0', stdout);
        else
            fputc(x - 10 + 'A', stdout);
    }
}

extern void
print_hex(const u32 n)
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
print_uint(const u32 n)
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
            fputc(d + '0', stdout);
            a -= i * d;
        }

        if (i == 1)
            break;
    }

    if (!start)
        fputc('0', stdout);
}

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
