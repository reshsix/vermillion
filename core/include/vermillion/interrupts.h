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

#ifndef VERMILLION_INTERRUPTS_H
#define VERMILLION_INTERRUPTS_H

#include <vermillion/types.h>

bool _interrupts_init(void);
void _interrupts_clean(void);

bool intr_config(u16 n, void (*f)(void), bool enable, u8 priority);
void intr_wait(void);

#endif
