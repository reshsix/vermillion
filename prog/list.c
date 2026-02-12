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

#define FG_WHITE "\033[37m"
#define FG_RED   "\033[31m"
#define FG_CYAN  "\033[36m"

extern bool
vrm_entry(struct vrm *v, const char **args, int count)
{
    bool ret = false;

    if (count == 2)
    {
        vrm_disk_f *f = v->disk.open(args[1]);
        if (f)
        {
            bool dir = false;
            if (v->disk.stat(f, &dir, NULL, NULL) && dir)
            {
                char *name = NULL;
                uint32_t size = 0;

                size_t count = 0;
                while (v->disk.walk(f, count, &dir, &name, &size))
                {
                    v->syslog.string((dir) ? FG_CYAN : "");
                    v->syslog.string(name);
                    v->syslog.string(FG_WHITE "\t\t");
                    v->syslog.signed_(size);
                    v->syslog.string("\r\n");
                    count++;
                }

                v->syslog.signed_(count);
                v->syslog.string(" files\r\n\r\n");
                ret = true;
            }
            else
                v->syslog.string("ERROR: Not a directory\r\n");
        }
        else
            v->syslog.string("ERROR: Directory not found\r\n");
    }
    else
        v->syslog.string("USAGE: list [directory]\r\n");

    return ret;
}
