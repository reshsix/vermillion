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

#pragma once

#include <general/types.h>

#include <hal/stream.h>
#include <hal/classes/spi.h>
#include <hal/classes/gpio.h>
#include <hal/classes/uart.h>

/* For devtree usage */

void comm_setup(dev_stream *uart0, dev_stream *uart1,
                dev_gpio *gpio, uint16_t *pins, uint8_t pinc,
                dev_spi *spi);

/* For external usage */

enum comm_id
{
    COMM_UART0,
    COMM_UART1,
    COMM_SPI
};

bool comm_dir(uint8_t pin, bool output);
bool comm_get(uint8_t pin, bool *state);
bool comm_set(uint8_t pin, bool state);

uint32_t comm_flags_uart(uint8_t bits, uint8_t parity, uint8_t stop);
uint32_t comm_flags_spi(uint8_t mode, bool lsb, bool csp, bool duplex);
bool comm_info(u8 id, u32 *rate, u32 *flags);
bool comm_config(u8 id, u32 rate, u32 flags);

bool comm_start(u8 id);
bool comm_stop(u8 id);
bool comm_read(u8 id, char *c);
bool comm_write(u8 id, char c);
