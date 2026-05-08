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

static dev_gpio   *g0 = NULL;
static dev_spi    *s0 = NULL;

static uint16_t pa[32] = {0};
static uint8_t  pc     = 0;

/* For devtree usage */

extern void
comm_setup(dev_gpio *gpio, uint16_t *pins, uint8_t pinc,
           dev_spi *spi)
{
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
