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

bool comm_gpio_dir(uint8_t pin, bool output);
bool comm_gpio_get(uint8_t pin, bool *state);
bool comm_gpio_set(uint8_t pin, bool state);

bool comm_uart_info(bool slave, uint32_t *rate, uint8_t *bits,
                    uint8_t *parity, uint8_t *stop);
bool comm_uart_config(bool slave, uint32_t rate, uint8_t bits,
                      uint8_t parity, uint8_t stop);
bool comm_uart_read(bool slave, char *c);
bool comm_uart_write(bool slave, char c);
bool comm_uart_read_nb(bool slave, char *c);
bool comm_uart_write_nb(bool slave, char c);

bool comm_spi_info(uint32_t *rate, uint8_t *mode, bool *lsb);
bool comm_spi_config(uint32_t rate, uint8_t mode, bool lsb);
bool comm_spi_state(bool cs);
bool comm_spi_transfer(uint8_t *data, size_t count);
bool comm_spi_limit(size_t *count);
bool comm_spi_transfer_nb(uint8_t *data, size_t count);
bool comm_spi_poll(void);
