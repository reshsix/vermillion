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

#include <types.h>

#include <h3/uart.h>
#include <h3/ports.h>
#include <h3/timers.h>
#include <h3/interrupts.h>

extern void __attribute__((noreturn))
halt(void)
{
    gic_disable();

    pin_config(PA15, PIN_CFG_OUT);
    pin_write(PA15, true);
    pin_config(PL10, PIN_CFG_OUT);
    pin_write(PL10, false);

    for (;;)
        arm_wait_interrupts();
}

extern void
print(char *s)
{
    for (; s[0] != '\0'; s = &(s[1]))
    {
        if (s[0] != '\0')
            uart_write(UART0, s[0]);
    }
}

static void
print_h8(u8 n)
{
    for (u8 i = 1; i <= 1; i--)
    {
        u8 x = (n >> (i * 4)) & 0xF;
        if (x < 10)
            uart_write(UART0, x + '0');
        else
            uart_write(UART0, x - 10 + 'A');
    }
}

extern void
print_hex(u32 n)
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
csleep(int n)
{
    timer_enable(TIMER0);
    timer_interval_set(TIMER0, n);
    timer_config(TIMER0, true, TIMER_CLK_24MHZ);
    timer_reload(TIMER0);
    timer_start(TIMER0);

    while (timer_current_get(TIMER0) != 0)
        arm_wait_interrupts();

    timer_stop(TIMER0);
    timer_disable(TIMER0);
}

extern void
usleep(int n)
{
    for (s64 a = n * 24; a > 0; a -= UINT32_MAX)
        csleep((a < UINT32_MAX) ? a : UINT32_MAX);
}

extern void
msleep(int n)
{
    for (s64 a = n * 1000; a > 0; a -= UINT32_MAX)
        usleep((a < UINT32_MAX) ? a : UINT32_MAX);
}

extern void
sleep(int n)
{
    for (s64 a = n * 1000; a > 0; a -= UINT32_MAX)
        msleep((a < UINT32_MAX) ? a : UINT32_MAX);
}
