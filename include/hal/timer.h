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

typedef struct
{
    void *init, (*clean)(void *);
    bool (*alarm)(void *ctx, u32 us, bool repeat,
                  void (*handler)(void *), void *arg);
    void (*wait)(void *ctx);
} drv_timer;

typedef struct
{
    const drv_timer *driver;
    void *context;
} dev_timer;

/* For devtree usage */

void timer_setup(dev_timer *list, u8 count);

/* For external usage */

bool timer_alarm(u8 id, u32 us, bool repeat,
                 void (*handler)(void *), void *arg);
