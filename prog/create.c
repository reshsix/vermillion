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

    bool dir = false;
    if (count == 3)
    {
        ret = true;
        if (v->str.comp(args[1], "dir", 3) == 0)
            dir = true;
        else if (v->str.comp(args[1], "file", 4) != 0)
            ret = false;
    }

    if (ret)
    {
        ret = v->disk.create(args[2], dir);
        if (!ret)
            v->syslog.string("ERROR: Creation failed\r\n");
    }
    else
        v->syslog.string("USAGE: create file/dir name\r\n");

    return ret;
}
