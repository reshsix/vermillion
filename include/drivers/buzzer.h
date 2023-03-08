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

#ifndef DRIVERS_BUZZER_H
#define DRIVERS_BUZZER_H

#include <types.h>

struct buzzer
{
    void (*write)(bool);
};

struct buzzer *buzzer_new(void (*write)(bool));
struct buzzer *buzzer_del(struct buzzer *bz);
void buzzer_note(struct buzzer *bz, u16 freq, u16 duration);
void buzzer_sample(struct buzzer *bz, u16 freq, u8 *data, size_t size);

#endif
