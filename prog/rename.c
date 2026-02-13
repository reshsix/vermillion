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

extern bool
vrm_entry(struct vrm *v, const char **args, int count)
{
    bool ret = false;

    if (count == 3)
    {
        vrm_disk_f *f = v->disk.open(args[1]);
        if (f)
        {
            if (!v->disk.rename(f, args[2]))
                v->syslog.string("ERROR: Renaming failed\r\n");
        }
        else
            v->syslog.string("ERROR: Entry not found\r\n");
    }
    else
        v->syslog.string("USAGE: rename [org] [new]\r\n");

    return ret;
}
