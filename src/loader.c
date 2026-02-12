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

/* Programs have to be compiled with the following flags:
    -shared -fPIE -fPIC -ffreestanding -nostdlib -Wl,-emain -Wl,-z,defs
*/

#include <general/mem.h>
#include <general/types.h>

#include <system/disk.h>

#include <syslog.h>

struct __attribute__((packed)) elf {
    u32 magic;
    u8 bits;
    u8 endianess;
    u8 hversion;
    u8 abi;
    u8 _pad[8];
    u16 type;
    u16 arch;
    u32 version;
    u32 entry;
    u32 progs;
    u32 sects;
    u32 flags;
    u16 hsize;
    u16 prog_s;
    u16 progs_n;
    u16 sect_s;
    u16 sects_n;
    u16 sects_strs;
};

struct __attribute__((packed)) elfp {
    u32 type;
    u32 offset;
    u32 vaddr;
    u32 paddr;
    u32 size_f;
    u32 size_m;
    u32 flags;
    u32 alignment;
};

static u8 *
fdpic_loader(const char *path, u32 *entry)
{
    u8 ret = true;

    disk_f *f = NULL;
    struct elf header = {0};
    if (path)
    {
        f = disk_open(path);
        if (!f || disk_read(f, &header, sizeof(struct elf)) !=
                  sizeof(struct elf))
            ret = false;
    }
    else
        ret = false;

    if (ret)
    {
        ret = header.magic == 0x464C457F && header.bits == 1     &&
              header.endianess == 1      && header.hversion == 1 &&
              header.abi == 0            && header.type == 3     &&
              header.arch == 0x28        && header.version == 1  &&
              header.hsize == sizeof(struct elf);
    }

    if (ret)
    {
        syslog_string("Loading ");
        syslog_string(path);
        syslog_string("\r\n");
    }

    size_t buffer_s = 0x400;
    u8 *buffer = mem_new(buffer_s);
    for (u16 i = 0; ret && i < header.progs_n; i++)
    {
        struct elfp pheader = {0};
        s32 offset = header.progs + (header.prog_s * i);

        ret = disk_seek(f, offset) &&
              disk_read(f, &pheader, sizeof(struct elfp)) ==
                        sizeof(struct elfp);

        if (ret && pheader.type == 1)
        {
            syslog_string("Prog ");
            syslog_unsigned(i);
            syslog_string(" [");
            if (pheader.flags & 4)
                syslog_char('R');
            if (pheader.flags & 2)
                syslog_char('W');
            if (pheader.flags & 1)
                syslog_char('X');
            syslog_string("]: ");
            syslog_unsigned((uintptr_t)&(buffer[pheader.vaddr]));
            syslog_string(" (");
            syslog_unsigned(pheader.size_f);
            syslog_string(")\r\n");

            u32 needed = pheader.vaddr + pheader.size_m;
            while (ret && buffer_s <= needed)
            {
                u8 *buffer2 = mem_renew(buffer, buffer_s *= 2);
                if (buffer2)
                    buffer = buffer2;
                else
                {
                    mem_del(buffer);
                    ret = false;
                }
            }

            if (ret)
            {
                ret = disk_seek(f, pheader.offset) &&
                      disk_read(f, &(buffer[pheader.vaddr]),
                                pheader.size_f) == pheader.size_f;
            }
        }
    }
    syslog_string("\r\n");

    if (ret)
        *entry = header.entry;
    else
        mem_del(buffer);

    if (f)
        disk_close(f);

    return ret ? buffer : NULL;
}

extern u8 *
loader_prog(const char *path, u32 *entry)
{
    u8 *ret = NULL;

    if (entry)
        ret = fdpic_loader(path, entry);

    return ret;
}
