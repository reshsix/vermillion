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

#ifdef CONFIG_EXTRA_DIAGNOSIS

#include <h3/ports.h>
#include <signal.h>
#include <utils.h>

static enum pin gpio[40] = {999,  999,
                            PA12, 999,
                            PA11, 999,
                            PA6,  PA13,
                            999,  PA14,
                            PA1,  PD14,
                            PA0,  999,
                            PA3,  PC4,
                            999,  PC7,
                            PC0,  999,
                            PC1,  PA2,
                            PC2,  PC3,
                            999,  PA21,
                            PA19, PA18,
                            PA7,  999,
                            PA8,  PG8,
                            PA9,  999,
                            PA10, PG9,
                            PA20, PG6,
                            999,  PG7};

static void
print_pin(u8 n, char *s)
{
    if ((n % 2) == 0)
    {
        print("| ");
        print(s);
        print(" ");
    }
    else
    {
        print(s);
        print(" |  ");
    }
}

#define GET_PIN \
    u8 n = (i * 2) + j; \
    enum pin pin = gpio[n]; \
    if (pin == 999) \
    { \
        print_pin(n, "-"); \
        continue; \
    }

extern void
check_gpio(void)
{
    u8 input_pd = 0;
    u8 input_pu = 0;
    u8 output = 0;

    print("\r\n|-----------------------------|");
    print("\r\n|          GPIO Check         |");
    print("\r\n|-----------------------------|");
    print("\r\n|  InputPD  InputPU  Output   |\r\n");
    for (u8 i = 0; i < 20; i++)
    {
        print("|  ");
        for (u8 j = 0; j < 2; j++)
        {
            GET_PIN;

            pin_config(pin, PIN_CFG_IN);
            pin_pull(pin, PIN_PULLDOWN);
            usleep(10);

            bool f = pin_read(pin);
            if (!f)
            {
                print_pin(n, "o");
                input_pd++;
            }
            else
                print_pin(n, "x");
        }
        for (u8 j = 0; j < 2; j++)
        {
            GET_PIN;

            pin_config(pin, PIN_CFG_IN);
            pin_pull(pin, PIN_PULLUP);
            usleep(10);

            bool t = pin_read(pin);
            if (t)
            {
                print_pin(n, "o");
                input_pu++;
            }
            else
                print_pin(n, "x");
        }
        for (u8 j = 0; j < 2; j++)
        {
            GET_PIN;

            pin_config(pin, PIN_CFG_OUT);
            usleep(10);

            pin_write(pin, true);
            usleep(10);
            bool t = pin_read(pin);

            pin_write(pin, false);
            usleep(10);
            bool f = pin_read(pin);

            if (t && !f)
            {
                print_pin(n, "o");
                output++;
            }
            else
                print_pin(n, "x");
        }
        print("|\r\n");
    }

    print("|  ");
    print((input_pd < 10) ? "   " : "  ");
    print_uint(input_pd);
    print("/28  ");
    print((input_pu < 10) ? "   " : "  ");
    print_uint(input_pu);
    print("/28  ");
    print((output < 10) ? "   " : "  ");
    print_uint(output);
    print("/28  ");
    print("|");

    print("\r\n|-----------------------------|\r\n");
}

#endif
