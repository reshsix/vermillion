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
#include <system/load.h>
#include <system/time.h>

#define VERMILLION_INTERNALS
#include <vermillion/vrm.h>

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

    struct vrm v = {.comm.read0  = comm_read0,
                    .comm.read1  = comm_read1,
                    .comm.write0 = comm_write0,
                    .comm.write1 = comm_write1,
                    .disk.open   = disk_open,
                    .disk.close  = disk_close,
                    .disk.stat   = disk_stat,
                    .disk.walk   = disk_walk,
                    .disk.seek   = disk_seek,
                    .disk.tell   = disk_tell,
                    .disk.read   = disk_read,
                    .disk.write  = disk_write,
                    .disk.flush  = disk_flush,
                    .disk.rename = disk_rename,
                    .disk.resize = disk_resize,
                    .disk.mkfile = disk_mkfile,
                    .disk.mkdir  = disk_mkdir,
                    .disk.remove = disk_remove,
                    .load.prog   = load_prog,
                    .time.event0 = time_event0,
                    .time.event1 = time_event1,
                    .time.sleep0 = time_sleep0,
                    .time.sleep1 = time_sleep1,
                    .time.clock0 = time_clock0,
                    .time.clock1 = time_clock1};

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
        u8 *mem = load_prog("init.elf", &entry);
        if (mem)
        {
            int (*init)(struct vrm *) = (void *)&(mem[entry]);
            log(!init(&v) ? "Init success" : "Init failure");
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
