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

#include <general/mem.h>

#include <system/comm.h>
#include <system/disk.h>
#include <system/wheel.h>

void devtree_init();
void devtree_clean();

void main(void)
{
    _mem_init();
    devtree_init();

    disk_f *f = disk_open("NOTICE");
    if (f)
    {
        u8 buf[128] = {0};
        for (;;)
        {
            u32 read = disk_read(f, buf, sizeof(buf));
            if (read == 0)
                break;

            for (size_t i = 0; i < read; i++)
                comm_write0(buf[i]);
        }
    }
    else
    {
        const char *msg = "NOTICE missing\r\n";
        for (size_t i = 0; msg[i] != '\0'; i++)
            comm_write0(msg[i]);
    }
    disk_close(f);

    devtree_clean();
    _mem_clean();
}
