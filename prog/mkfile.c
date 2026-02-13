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

    if (count == 2)
    {
        if (v->disk.create(args[1], false))
            ret = true;
        else
            v->syslog.string("ERROR: Creation failed\r\n");
    }
    else
        v->syslog.string("USAGE: mkfile [file]\r\n");

    return ret;
}
