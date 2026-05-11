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

#include <hal/classes/gpio.h>

/* For devtree usage */

void comm_setup(dev_gpio *gpio, uint16_t *pins, uint8_t pinc);

/* For external usage */

bool comm_gpio_dir(uint8_t pin, bool output);
bool comm_gpio_get(uint8_t pin, bool *state);
bool comm_gpio_set(uint8_t pin, bool state);
