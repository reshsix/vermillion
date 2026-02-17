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

#include <vermillion/prog.h>

extern bool
vrm_prog(struct vrm *v, const char **args, int count)
{
    bool ret = false;

    if (count == 2)
    {
        vrm_disk_f *f = v->disk.open(args[1]);

        if (f)
        {
            bool dir = false;
            if (v->disk.stat(f, &dir, NULL, NULL) && !dir)
            {
                ret = true;

                while (ret)
                {
                    char c = '\0';

                    uint32_t read = v->disk.read(f, &c, 1);
                    if (!read)
                        break;

                    v->comm.write0(c);
                }
            }
            else
                v->syslog.string("ERROR: Not a regular file\r\n");
        }
        else
            v->syslog.string("ERROR: File not found\r\n");

        v->disk.close(f);
    }
    else
        v->syslog.string("USAGE: show file\r\n");

    return ret;
}
