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

#include <vermillion/entry.h>

static char line[1024] = {0};
static size_t line_c = 0;

static char *list[256] = {0};
static size_t list_c = 0;

static char buffer[2048] = {0};

#define FG_WHITE "\033[37m"
#define FG_RED   "\033[31m"
#define FG_GREEN "\033[32m"

extern bool
vrm_entry(struct vrm *v, const char **args, int count)
{
    bool ret = false;

    (void)args;
    (void)count;

    v->comm.write0('>');
    v->comm.write0(' ');

    while (true)
    {
        char c = v->comm.read0();
        if (c == '\r')
        {
            line[line_c++] = '\0';
            v->comm.write0('\r');
            v->comm.write0('\n');

            /* Parse program */
            char *saveptr = NULL;
            list[0] = v->str.token(line, " ", &saveptr);
            if (list[0])
                list_c++;

            /* Parse args */
            for (size_t i = 0; list[i] && i < 255; i++)
            {
                list[i + 1] = v->str.token(NULL, " ", &saveptr);
                if (list[i + 1])
                    list_c++;
            }

            /* Try built-ins */
            if (v->str.comp(list[0], "exit", 0) == 0)
            {
                ret = true;
                break;
            }
            else if (v->str.comp(list[0], "clock", 0) == 0)
            {
                if (list_c == 1)
                {
                    uint64_t ms = v->time.clock0();
                    uint64_t secs = v->time.clock1();
                    uint64_t dec = ms - (secs * 100);

                    v->syslog.signed_(secs);
                    v->syslog.string(".");
                    v->syslog.signed_(dec);
                    v->syslog.string(" seconds since boot\r\n");
                }
                else
                    v->syslog.string("USAGE: clock\r\n");
            }
            else
            {
                /* Make path */
                v->mem.fill(buffer, '\0', sizeof(buffer));
                v->str.copy(buffer, "/prog/", 6);
                v->str.concat(buffer, list[0], v->str.length(list[0]));
                v->str.concat(buffer, ".elf", 4);

                /* Run program */
                if (list_c)
                {
                    uint32_t entry = 0;
                    uint8_t *mem = v->loader.prog(buffer, &entry);
                    if (mem)
                    {
                        vrm_entry_t f = (void *)&(mem[entry]);
                        v->syslog.string(f(v, list, list_c) ?
                                         FG_GREEN "Success\r\n" :
                                         FG_RED   "Failure\r\n");
                        v->syslog.string(FG_WHITE);
                    }
                    else
                        v->syslog.string("ERROR: Program not found\r\n");
                    v->mem.del(mem);
                }
            }

            /* Cleanup */
            for (size_t i = 0; i < list_c; i++)
                list[i] = NULL;
            line_c = 0;
            list_c = 0;

            /* Prompt */
            v->comm.write0('>');
            v->comm.write0(' ');
        }
        else if (c == '\b' || c == 0x7F)
        {
            if (line_c > 0)
            {
                v->comm.write0('\b');
                v->comm.write0(' ');
                v->comm.write0('\b');
                line[--line_c] = '\0';
            }
        }
        else
        {
            if (line_c < sizeof(line))
            {
                v->comm.write0(c);
                line[line_c++] = c;
            }
            else
            {
                v->syslog.string("NOTICE: user is typing too much\r\n");
                break;
            }
        }
    }

    return ret;
}
