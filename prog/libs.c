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

#include <vermillion/lib.h>
#include <vermillion/vrm.h>
#include <vermillion/prog.h>

static char buffer[256] = {0};

struct test
{
    int (*test)(struct vrm *v);
};

extern bool
vrm_prog(struct vrm *v, const char **args, int count)
{
    bool ret = false;

    if (count == 3)
    {
        if (v->str.comp(args[1], "load", 0) == 0)
        {
            v->mem.fill(buffer, '\0', sizeof(buffer));
            v->str.copy(buffer, "/lib/", 6);
            v->str.concat(buffer, args[2], v->str.length(args[2]));
            v->str.concat(buffer, ".elf", 4);

            uint32_t entry = 0;
            uint8_t *mem = v->loader.fdpic(buffer, &entry);
            if (mem)
            {
                vrm_lib_t f = (void *)&(mem[entry]);
                void *address = f();
                if (address)
                    ret = v->libs.load(args[2], address, mem);

                if (!ret)
                {
                    v->syslog.string("ERROR: Initialization failed\r\n");
                    v->mem.del(mem);
                }
            }
            else
                v->syslog.string("ERROR: Library not found\r\n");
        }
        else if (v->str.comp(args[1], "unload", 0) == 0)
        {
            if (v->str.comp(args[2], "test", 0) == 0)
            {
                struct test *t = v->libs.pointer(args[2]);
                v->syslog.unsigned_(t->test(v));
                v->syslog.string("\r\n");
            }

            ret = v->libs.unload(args[2]);
            if (!ret)
                v->syslog.string("ERROR: Cleaning up failed\r\n");
        }
        else
            v->syslog.string("USAGE: libs load/unload name\r\n");
    }
    else
        v->syslog.string("USAGE: libs load/unload name\r\n");

    return ret;
}
