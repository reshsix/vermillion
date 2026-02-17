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

#define FG_WHITE "\033[37m"
#define FG_RED   "\033[31m"
#define FG_CYAN  "\033[36m"

static uint8_t buffer[1024] = {0};

extern bool
vrm_prog(struct vrm *v, const char **args, int count)
{
    bool ret = false;

    if (count == 3)
    {
        vrm_disk_f *f = v->disk.open(args[1]);

        if (f)
        {
            bool dir = false;
            uint32_t size = 0;
            if (v->disk.stat(f, &dir, NULL, &size))
            {
                if (!dir)
                    ret = true;
                else
                    v->syslog.string("ERROR: File is a directory\r\n");
            }
            else
                v->syslog.string("ERROR: File stat failed\r\n");

            vrm_disk_f *f2 = NULL;
            if (ret)
            {
                ret = false;

                f2 = v->disk.open(args[2]);
                if (f2)
                    ret = true;
                else
                {
                    if (v->disk.create(args[2], false))
                    {
                        f2 = v->disk.open(args[2]);
                        if (f2)
                            ret = true;
                        else
                            v->syslog.string("ERROR: Failed to open file2\r\n");
                    }
                    else
                        v->syslog.string("ERROR: File2 creation failed\r\n");
                }
            }

            if (ret)
            {
                ret = v->disk.resize(f2, size);
                if (!ret)
                    v->syslog.string("ERROR: Failed to resize file2\r\n");
            }

            if (ret)
            {
                while (ret)
                {
                    uint32_t read = v->disk.read(f, buffer, sizeof(buffer));
                    if (!read)
                        break;

                    if (v->disk.write(f2, buffer, read) != read)
                    {
                        v->syslog.string("ERROR: File2 writing failed\r\n");
                        ret = false;
                    }
                }

                if (!v->disk.flush(f2))
                {
                    v->syslog.string("ERROR: File2 flushing failed\r\n");
                    ret = false;
                }

                if (ret)
                {
                    ret = false;

                    uint32_t size = 0, size2 = 0;
                    if (v->disk.stat(f,  NULL, NULL, &size)  &&
                        v->disk.stat(f2, NULL, NULL, &size2))
                    {
                        ret = (size == size2);
                        if (!ret)
                            v->syslog.string("ERROR: File sizes differ\r\n");
                    }
                    else
                        v->syslog.string("ERROR: Failed to confirm sizes\r\n");
                }
            }
        }
        else
            v->syslog.string("ERROR: File not found\r\n");

        v->disk.close(f);
    }
    else
        v->syslog.string("USAGE: copy file file2\r\n");

    return ret;
}
