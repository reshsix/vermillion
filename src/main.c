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

#include <_utils.h>
#include <vermillion/loader.h>

#define __STRING(x) #x
#define _STRING(x) __STRING(x)

extern int
kernel_main(void)
{
    int ret = 0;

    u32 entry = 0;

    print("Loading ");
    #ifndef CONFIG_LOADER_EMBED
    print(_STRING(CONFIG_LOADER_FILE));
    u8 *prog = loader_prog(_STRING(CONFIG_LOADER_FILE), &entry);
    #else
    extern u8 _binary_init_bin_start[];
    print_hex((u32)_binary_init_bin_start);
    u8 *prog = loader_prog(NULL, &entry);
    #endif
    print(": ");

    if (prog)
    {
        print("Success\r\n");
        ret = ((int (*)(void))&(prog[entry]))();
    }
    else
        print("Failure\r\n");

    return ret;
}
