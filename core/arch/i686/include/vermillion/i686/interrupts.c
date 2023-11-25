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

#include <vermillion/types.h>
#include <vermillion/utils.h>

static void
init_intr(void)
{
    return;
}

static void
clean_intr(void)
{
    return;
}

extern bool
intr_config(u16 n, void (*f)(void), bool enable, u8 priority)
{
    (void)n, (void)f, (void)enable, (void)priority;
    return false;
}

extern void
intr_wait(void)
{
    asm volatile ("hlt");
}
