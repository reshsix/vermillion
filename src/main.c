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

#include <interface/loader.h>

extern int
kernel_main(void)
{
    int ret = 0;

    u32 entry = 0;
    u8 *prog = loader_prog("/bin/init", &entry);
    if (prog)
        ret = ((int (*)(void))&(prog[entry]))();

    return ret;
}
