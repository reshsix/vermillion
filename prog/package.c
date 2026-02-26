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

struct __attribute__((packed)) tar_h
{
    uint8_t path[100];
    uint8_t mode[8];
    uint8_t uid[8];
    uint8_t gid[8];
    uint8_t size[12];
    uint8_t time[12];
    uint8_t checksum[8];
    uint8_t type[1];
    uint8_t pad[355];
};

static uint8_t buffer [512] = {0};
static uint8_t buffer2[256] = {0};
static uint8_t buffer3[256] = {0};

static bool
exist_or_create(struct vrm *v, bool dir, const char *path, bool *created)
{
    bool ret = false;

    vrm_disk_f *f = v->disk.open(path);
    if (f)
    {
        bool dir2 = false;
        if (v->disk.stat(f, &dir2, NULL, NULL) && (dir == dir2))
            ret = true;
        else
            v->syslog.string(": wrong type");
    }
    else
    {
        ret = v->disk.create(path, dir);
        if (ret)
            *created = true;
    }
    v->disk.close(f);

    return ret;
}

static bool
tar_install(struct vrm *v, vrm_disk_f *f, vrm_disk_f *scr,
            bool dir, const char *name, size_t size)
{
    bool ret = true;

    v->str.copy(buffer2, name, 0);
    v->path.cleanup(buffer2);
    size_t len = v->str.length(buffer2);

    char *state = NULL;
    for (char *token = v->str.token(buffer2, "/", &state); ret && token;
               token = v->str.token(NULL,    "/", &state))
    {
        bool dir2 = true;

        size_t len2 = v->str.length(buffer2);
        if (len == len2)
            dir2 = dir;

        bool created = false;
        ret = exist_or_create(v, dir2, buffer2, &created);
        if (!ret)
        {
            v->syslog.string("Failed to open or create ");
            v->syslog.string(buffer2);
            v->syslog.string("\r\n");
        }

        if (ret && created)
        {
            char z = '\0';
            v->disk.write(scr, &z, 1);
            v->disk.write(scr, buffer2, len2);
        }

        state[-1] = '/';
    }
    v->str.copy(buffer2, name, 0);
    v->path.cleanup(buffer2);

    if (ret && !dir)
    {
        vrm_disk_f *f2 = v->disk.open(buffer2);
        if (f2 && v->disk.resize(f2, size))
        {
            while (ret && size)
            {
                size_t frac = 512;
                if (size < frac)
                    frac = size;

                ret = ( v->disk.read(f,  buffer,  512) ==  512) &&
                      (v->disk.write(f2, buffer, frac) == frac);
                if (!ret)
                    v->syslog.string("Failed to unpack data\r\n");

                size -= frac;
            }
        }
        else
            v->syslog.string("Failed to open file\r\n");
        v->disk.close(f2);
    }

    return ret;
}

static bool
tar_read(struct vrm *v, vrm_disk_f *f, vrm_disk_f *scr)
{
    bool ret = true;

    while (ret && v->disk.read(f, buffer, sizeof(buffer)))
    {
        struct tar_h *hdr = buffer;
        if (hdr->path[0] == '\0')
            continue;

        size_t len = v->str.length(hdr->path);
        if (len < 100 && (hdr->type[0] == '0' || hdr->type[0] == 0 ||
                          hdr->type[0] == '5'))
        {
            bool dir = false;
            size_t size = 0;

            if (hdr->path[len - 1] == '/')
                dir = true;

            if (!dir)
            {
                for (size_t i = 0; i < 12 && hdr->size[i]; i++)
                {
                    size *= 8;
                    size += hdr->size[i] - '0';
                }
            }

            v->syslog.string(hdr->path);
            v->syslog.string("\r\n");

            if (scr)
                ret = tar_install(v, f, scr, dir, hdr->path, size);
            else if (!dir)
            {
                uint32_t pos = 0;
                if (v->disk.tell(f, &pos))
                {
                    pos = (pos + size + 511) & ~511;
                    if (!v->disk.seek(f, pos))
                    {
                        v->syslog.string("ERROR: File seek failed\r\n");
                        ret = false;
                        break;
                    }
                }
                else
                {
                    v->syslog.string("ERROR: File tell failed\r\n");
                    ret = false;
                    break;
                }
            }
        }
        else
        {
            v->syslog.string("ERROR: Package contain unsupported files\r\n");
            ret = false;
            break;
        }
    }

    return ret;
}

static bool
script_read(struct vrm *v, vrm_disk_f *f, bool remove)
{
    bool ret = true;

    bool dir = false;
    uint32_t size = 0;
    if (v->disk.stat(f, &dir, NULL, &size))
    {
        if (!dir)
        {
            while (ret && size)
            {
                size_t len = 0;
                for (size_t i = 0; ret && i < 255; i++)
                {
                    ret = (v->disk.seek(f, size - 1) &&
                           v->disk.read(f, &(buffer[i]), 1));
                    size--;

                    if (ret && buffer[i] == '\0')
                        break;
                    len++;
                }
                if (!len)
                    continue;

                for (size_t i = 0; i < len; i++)
                    buffer3[i] = buffer[len - i - 1];
                buffer3[len] = '\0';

                v->syslog.string(buffer3);
                v->syslog.string("\r\n");

                if (!ret)
                {
                    if (size == 0)
                        ret = true;
                    else
                        v->syslog.string("ERROR: Failed to read next file\r\n");
                }

                if (ret && remove && len > 2)
                {
                    ret = v->disk.remove(buffer3);
                    if (!ret)
                        v->syslog.string("ERROR: Failed to remove file\r\n");
                }
            }
        }
        else
            v->syslog.string("ERROR: File is somehow a directory\r\n");
    }
    else
        v->syslog.string("ERROR: File stat failed\r\n");

    return ret;
}

extern bool
vrm_prog(struct vrm *v, const char **args, int count)
{
    bool ret = false;

    if (count == 3)
    {
        uint8_t op = 0;
        if (v->str.comp(args[1], "prev", 0) == 0)
            op = 1;
        else if (v->str.comp(args[1], "install", 0) == 0)
            op = 2;
        else if (v->str.comp(args[1], "show", 0) == 0)
            op = 3;
        else if (v->str.comp(args[1], "remove", 0) == 0)
            op = 4;

        if (op && op < 5)
        {
            v->mem.fill(buffer, '\0', sizeof(buffer));
            v->str.copy(buffer, "/pkg/", 6);
            v->str.concat(buffer, args[2], v->str.length(args[2]));
            v->str.concat(buffer, ".tar", 0);

            v->mem.fill(buffer2, '\0', sizeof(buffer2));
            v->str.copy(buffer2, "/pkg/", 6);
            v->str.concat(buffer2, args[2], v->str.length(args[2]));
            v->str.concat(buffer2, ".script", 0);

            vrm_disk_f *f = v->disk.open((op < 3) ? buffer : buffer2);
            if (f)
            {
                bool dir = false;
                uint32_t size = 0;
                if (v->disk.stat(f, &dir, NULL, &size))
                {
                    if (!dir)
                    {
                        switch (op)
                        {
                            case 1:
                                ret = tar_read(v, f, NULL);
                                break;
                            case 2:
                                if (v->disk.create(buffer2, false))
                                {
                                    vrm_disk_f *f2 = v->disk.open(buffer2);
                                    if (f2)
                                        ret = tar_read(v, f, f2);
                                    else
                                        v->syslog.string(
                                            "ERROR: Failed to open file\r\n");
                                    v->disk.close(f2);
                                }
                                else
                                    v->syslog.string(
                                        "ERROR: Package already installed\r\n");
                                break;
                            case 3:
                                ret = script_read(v, f, false);
                                break;
                            case 4:
                                ret = script_read(v, f, true);
                                if (ret)
                                {
                                    v->disk.close(f);
                                    f = NULL;

                                    ret = v->disk.remove(buffer2);
                                    if (!ret)
                                        v->syslog.string(
                                            "ERROR: Failed to remove file\r\n");
                                }
                                break;
                        }
                    }
                    else
                        v->syslog.string("ERROR: File is a directory\r\n");
                }
                else
                    v->syslog.string("ERROR: File stat failed\r\n");
            }
            else
                v->syslog.string("ERROR: File not found\r\n");
            v->disk.close(f);
        }
        else
            v->syslog.string(
                "USAGE: package prev/install/show/remove file\r\n");
    }
    else
        v->syslog.string("USAGE: package prev/install/show/remove file\r\n");

    return ret;
}
