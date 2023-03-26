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

#ifndef INTERFACE_GIC_H
#define INTERFACE_GIC_H

#include <_types.h>
#include <drivers/gic.h>

bool _gic_init(void);
void _gic_clean(void);

void gic_config(u16 n, void (*f)(void), bool enable, u8 priority);
void gic_wait(void);

#endif
