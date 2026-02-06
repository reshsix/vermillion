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
#include <system/loader.h>

void devtree_init();
void devtree_clean();

static void
log(const char *s)
{
    for (size_t i = 0; s[i] != '\0'; i++)
        comm_write0(s[i]);
}

extern void
main(void)
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

        u32 entry = 0;
        u8 *mem = loader_prog("init.elf", &entry);
        if (mem)
        {
            int (*init)(void) = (void *)&(mem[entry]);
            log(!init() ? "Init success" : "Init failure");
        }
        else
            log("init.elf missing\r\n");
        mem_del(mem);
    }
    else
        log("NOTICE missing\r\n");
    disk_close(f);

    devtree_clean();
    _mem_clean();
}
