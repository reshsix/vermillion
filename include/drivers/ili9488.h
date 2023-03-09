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

#ifndef DRIVERS_ILI9488_H
#define DRIVERS_ILI9488_H

struct ili9488
{
    enum pin dcrs;
    enum pin leds;

    void (*write)(u8);
};

struct ili9488 *ili9488_new(enum pin dcrs, enum pin leds, void (*write)(u8));
struct ili9488 *ili9488_del(struct ili9488 *ili);
void ili9488_start(struct ili9488 *ili, u8 *splash,
                   u16 splash_w, u16 splash_h);
void ili9488_update(struct ili9488 *ili, u8* buffer,
                    u16 x, u16 y, u16 w, u16 h);
void ili9488_clear(struct ili9488 *ili);

#endif
