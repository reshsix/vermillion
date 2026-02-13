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
#include <general/str.h>
#include <general/dict.h>
#include <general/path.h>

#include <system/comm.h>
#include <system/disk.h>
#include <system/time.h>
#include <system/vars.h>

#include <loader.h>
#include <syslog.h>
#include <devtree.h>

#define VERMILLION_INTERNALS
#include <vermillion/vrm.h>
#include <vermillion/entry.h>

extern void
main(void)
{
    devtree_init();

    struct vrm v = {.mem.new       = mem_new,
                    .mem.renew     = mem_renew,
                    .mem.del       = mem_del,
                    .mem.comp      = mem_comp,
                    .mem.find      = mem_find,
                    .mem.fill      = mem_fill,
                    .mem.copy      = mem_copy,
                    .str.length    = str_length,
                    .str.comp      = str_comp,
                    .str.span      = str_span,
                    .str.find_l    = str_find_l,
                    .str.find_r    = str_find_r,
                    .str.find_m    = str_find_m,
                    .str.find_s    = str_find_s,
                    .str.token     = str_token,
                    .str.copy      = str_copy,
                    .str.concat    = str_concat,
                    .str.dupl      = str_dupl,
                    .dict.new      = dict_new,
                    .dict.del      = dict_del,
                    .dict.get      = dict_get,
                    .dict.set      = dict_set,
                    .path.cleanup  = path_cleanup,
                    .path.dirname  = path_dirname,
                    .path.filename = path_filename,

                    .comm.read0  = comm_read0,
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
                    .disk.create = disk_create,
                    .disk.remove = disk_remove,
                    .time.event0 = time_event0,
                    .time.event1 = time_event1,
                    .time.sleep0 = time_sleep0,
                    .time.sleep1 = time_sleep1,
                    .time.clock0 = time_clock0,
                    .time.clock1 = time_clock1,
                    .vars.get    = vars_get,
                    .vars.set    = vars_set,

                    .loader.prog      = loader_prog,
                    .syslog.char_     = syslog_char,
                    .syslog.string    = syslog_string,
                    .syslog.unsigned_ = syslog_unsigned,
                    .syslog.signed_   = syslog_signed};
    syslog_string("\033[2J\033[H");

    disk_f *f = disk_open("/NOTICE");
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

        const char *path = "/prog/init.elf";

        u32 entry = 0;
        u8 *mem = loader_prog(path, &entry);
        if (mem)
        {
            vrm_entry_t f = (void *)&(mem[entry]);
            syslog_string(f(&v, &path, 1) ? "Success" : "Failure");
        }
        else
            syslog_string("init.elf missing\r\n");
        mem_del(mem);
    }
    else
        syslog_string("NOTICE missing\r\n");
    disk_close(f);

    devtree_clean();
}
