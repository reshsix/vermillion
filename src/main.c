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

#include <general/mem.h>
#include <general/str.h>
#include <general/path.h>

#include <hal/fs.h>
#include <hal/timer.h>

#include <system/libs.h>

#include <loader.h>
#include <syslog.h>
#include <devtree.h>

#define VERMILLION_INTERNALS
#include <vermillion/vrm.h>
#include <vermillion/prog.h>

#include <vermillion/hal/spi.h>
#include <vermillion/hal/gpio.h>
#include <vermillion/hal/uart.h>

static void *
driver(u8 driver, u8 version)
{
    void *ret = NULL;

    switch (driver)
    {
        case VRM_UART:
            ret = uart_driver(version);
            break;
        case VRM_GPIO:
            ret = gpio_driver(version);
            break;
        case VRM_SPI:
            ret = spi_driver(version);
            break;
    }

    return ret;
}

extern void
main(void)
{
    devtree_init();

    struct vrm v = {.driver = driver,

                    .mem.new       = mem_new,
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
                    .path.validate = path_validate,
                    .path.cleanup  = path_cleanup,
                    .path.dirname  = path_dirname,
                    .path.filename = path_filename,

                    .timer.alarm  = timer_alarm,
                    .fs.open      = fs_open,
                    .fs.close     = fs_close,
                    .fs.stat      = fs_stat,
                    .fs.walk      = fs_walk,
                    .fs.seek      = fs_seek,
                    .fs.tell      = fs_tell,
                    .fs.read      = fs_read,
                    .fs.write     = fs_write,
                    .fs.flush     = fs_flush,
                    .fs.resize    = fs_resize,
                    .fs.create    = fs_create,
                    .fs.remove    = fs_remove,
                    .libs.load    = libs_load,
                    .libs.unload  = libs_unload,
                    .libs.pointer = libs_pointer,

                    .loader.fdpic     = loader_fdpic,
                    .syslog.char_     = syslog_char,
                    .syslog.string    = syslog_string,
                    .syslog.unsigned_ = syslog_unsigned,
                    .syslog.signed_   = syslog_signed};
    syslog_string("\033[2J\033[H");

    struct fs_file *f = fs_open(0, "/NOTICE");
    if (f)
    {
        u8 buf[128] = {0};
        for (;;)
        {
            u32 read = fs_read(f, buf, sizeof(buf));
            if (read == 0)
                break;

            for (size_t i = 0; i < read; i++)
                syslog_char(buf[i]);
        }

        const char *path  = "/prog/init.elf";
        const char *path2 = "/prog/shell.elf";

        u32 entry = 0;
        u8 *mem = loader_fdpic(0, path, &entry);
        if (mem)
        {
            vrm_prog_t f = (void *)&(mem[entry]);
            syslog_string(f(&v, &path, 1) ? "Success" : "Failure");
        }
        else
        {
            syslog_string("init.elf missing, running shell\r\n");
            u8 *mem = loader_fdpic(0, path2, &entry);
            if (mem)
            {
                vrm_prog_t f = (void *)&(mem[entry]);
                syslog_string(f(&v, &path, 1) ? "Success" : "Failure");
            }
            else
                syslog_string("shell.elf missing\r\n");
        }
        mem_del(mem);
    }
    else
        syslog_string("NOTICE missing\r\n");
    fs_close(f);

    devtree_clean();
}
