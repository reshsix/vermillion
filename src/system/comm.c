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

#include <general/types.h>

#include <hal/stream.h>
#include <hal/classes/spi.h>
#include <hal/classes/gpio.h>
#include <hal/classes/uart.h>

#include <system/comm.h>

static dev_stream *u0 = NULL;
static dev_stream *u1 = NULL;
static dev_gpio   *g0 = NULL;
static dev_spi    *s0 = NULL;

static uint16_t pa[32] = {0};
static uint8_t  pc     = 0;

/* For devtree usage */

extern void
comm_setup(dev_uart *uart0, dev_uart *uart1,
           dev_gpio *gpio, uint16_t *pins, uint8_t pinc,
           dev_spi *spi)
{
    u0 = uart0;
    u1 = uart1;
    g0 = gpio;
    s0 = spi;

    if (pinc > 32)
        pinc = 32;

    pc = pinc;
    for (uint8_t i = 0; i < pinc; i++)
        pa[i] = pins[i];
}

/* GPIO functions */

extern bool
comm_gpio_dir(uint8_t pin, bool output)
{
    bool ret = false;

    if (pin < pc)
        ret = gpio_config(g0, pa[pin],
                          (output) ? GPIO_OUT : GPIO_IN, GPIO_PULLOFF);

    return ret;
}

extern bool
comm_gpio_get(uint8_t pin, bool *state)
{
    bool ret = false;

    if (pin < pc)
        ret = gpio_get(g0, pa[pin], state);

    return ret;
}

extern bool
comm_gpio_set(uint8_t pin, bool state)
{
    bool ret = false;

    if (pin < pc)
        ret = gpio_set(g0, pa[pin], state);

    return ret;
}

/* UART functions */

extern bool
comm_uart_info(bool slave, uint32_t *rate, uint8_t *bits,
               uint8_t *parity, uint8_t *stop)
{
    bool ret = false;

    enum uart_bits   bits2;
    enum uart_parity parity2;
    enum uart_stop   stop2;

    ret = uart_info((slave) ? u1 : u0, rate, &bits2, &parity2, &stop2);
    if (ret)
    {
        if (bits)
            *bits = bits2;
        if (parity)
            *parity = parity2;
        *stop = stop2;
    }

    return ret;
}
extern bool
comm_uart_config(bool slave, uint32_t rate, uint8_t bits,
                 uint8_t parity, uint8_t stop)
{
    return uart_config((slave) ? u1 : u0, rate, bits, parity, stop);
}

extern bool
comm_uart_read(bool slave, char *c)
{
    while (!uart_read((slave) ? u1 : u0, c));
    return true;
}

extern bool
comm_uart_write(bool slave, char c)
{
    while (!uart_write((slave) ? u1 : u0, c));
    return true;
}

extern bool
comm_uart_read_nb(bool slave, char *c)
{
    return uart_read((slave) ? u1 : u0, c);
}

extern bool
comm_uart_write_nb(bool slave, char c)
{
    return uart_write((slave) ? u1 : u0, c);
}

/* SPI functions */

extern bool
comm_spi_info(uint32_t *rate, uint8_t *mode, bool *lsb)
{
    bool ret = false;

    enum spi_mode mode2;
    ret = spi_info(s0, rate, &mode2, lsb);
    if (ret)
    {
        if (mode)
            *mode = mode2;
    }

    return ret;
}

extern bool
comm_spi_config(uint32_t rate, uint8_t mode, bool lsb)
{
    return spi_config(s0, rate, mode, lsb);
}

extern bool
comm_spi_state(bool cs)
{
    return spi_state(s0, cs);
}

extern bool
comm_spi_transfer(uint8_t *data, size_t count)
{
    bool ret = false;

    size_t limit = 0;
    ret = spi_limit(s0, &limit);
    if (ret)
    {
        for (size_t i = 0; i < count; i += limit)
        {
            size_t remain = count - i;
            size_t size = (remain > limit) ? limit : remain;
            while (!spi_transfer(s0, &(data[i]), size));
            while (!spi_poll(s0));
        }
    }

    return ret;
}

extern bool
comm_spi_limit(size_t *count)
{
    return spi_limit(s0, count);
}

extern bool
comm_spi_transfer_nb(uint8_t *data, size_t count)
{
    return spi_transfer(s0, data, count);
}

extern bool
comm_spi_poll(void)
{
    return spi_poll(s0);
}
