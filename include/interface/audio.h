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
#include <drivers/dummy.h>
#include <drivers/buzzer.h>

struct audio;
struct audio *audio_new(void);
struct audio *audio_del(struct audio *a);
void audio_note(struct audio *a, u16 freq, u16 duration);
void audio_sample(struct audio *a, u16 freq, u8 *data, size_t size);

#endif
