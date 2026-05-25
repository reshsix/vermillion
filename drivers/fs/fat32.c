/*
 *  This file is part of vermillion.
 *
 *  Vermillion is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3.
 *
 *  Vermillion is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vermillion. If not, see <https://www.gnu.org/licenses/>.
*/

#include <general/types.h>
#include <general/mem.h>
#include <general/str.h>
#include <general/path.h>

#define VERMILLION_INTERNALS
#include <vermillion/hal/disk.h>
#include <vermillion/sys/file.h>

struct __attribute__((packed)) fat32br
{
    u8 jmp[3];
    char oem[8];
    u16 bytepersect;
    u8 sectspercluster;
    u16 reservedsects;
    u8 tablecount;
    u16 rootentries;
    u16 sectsvolume;
    u8 mediatype;
    u16 _unused;
    u16 sectspertrack;
    u16 headcount;
    u32 hiddensects;
    u32 sectsvolume32;
    u32 sectspertable;
    u16 flags;
    u16 version;
    u32 rootcluster;
    u16 fsinfosect;
    u16 backupsect;
    u8 _reserved[12];
    u8 drivenum;
    u8 flagsnt;
    u8 signature;
    u32 volumeid;
    char label[11];
    char system[8];
};

struct fat32e
{
    u8 attributes;

    u32 location;

    u32 cluster;
    size_t size;

    u32 sect;
    u32 idx;
};

struct fat32
{
    u8 disk;

    struct fat32br br;
    u8 *buffer;
    struct fat32e root;

    struct fat32e *current;
};

static u32 fat32_buf[0x80];
s64 fat32_buf_n = -1;
static u8 fat32_buf2[0x200];

/* Helper functions */

static u32
utf8_to_unicode(u8 **buf)
{
    u32 ret = 0;

    if ((*buf)[0] < 0x80)
    {
        ret = (*buf)[0];
        *buf = &((*buf)[1]);
    }
    else if ((*buf)[0] >= 0xC0)
    {
        if ((*buf)[0] < 0xE0)
        {
            ret = ((*buf)[0] & ~0xF0) | ((*buf)[1] & ~0xC0);
            *buf = &((*buf)[2]);
        }
        else if ((*buf)[0] < 0xF0)
        {
            ret = ((*buf)[0] & ~0xF0) | ((*buf)[1] & ~0xC0) |
                  ((*buf)[2] & ~0xC0);
            *buf = &((*buf)[3]);
        }
        else
        {
            ret = ((*buf)[0] & ~0xF0) | ((*buf)[1] & ~0xC0) |
                  ((*buf)[2] & ~0xC0) | ((*buf)[3] & ~0xC0);
            *buf = &((*buf)[4]);
        }
    }

    return ret;
}

static u32
utf16_to_unicode(u16 **buf)
{
    u32 ret = (*buf)[0];

    if ((ret >> 10) == 0x36)
    {
        ret = (ret & 0x3ff) << 10;
        *buf = &((*buf)[1]);
        if (((*buf)[0] >> 10) == 0x37)
        {
            ret |= (*buf)[0] & 0x3ff;
            ret += 0x10000;
        }
        else
            ret = '?';
    }

    return ret;
}

static size_t
unicode_to_utf8(u32 code, char* buf)
{
    size_t ret = 0;

    if (code < 0x80)
    {
        buf[ret++] = code;
    }
    else if (code < 0x800)
    {
        buf[ret++] = 0xC0 | ((code >> 6) & 0x1F);
        buf[ret++] = 0x80 | (code & 0x3F);
    }
    else if (code < 0x10000)
    {
        buf[ret++] = 0xE0 | ((code >> 12) & 0x0F);
        buf[ret++] = 0x80 | ((code >> 6) & 0x3F);
        buf[ret++] = 0x80 | (code & 0x3F);
    }
    else
    {
        buf[ret++] = 0xF0 | ((code >> 18) & 0x07);
        buf[ret++] = 0x80 | ((code >> 12) & 0x3F);
        buf[ret++] = 0x80 | ((code >> 6) & 0x3F);
        buf[ret++] = 0x80 | (code & 0x3F);
    }

    return ret;
}

static size_t
unicode_to_utf16(u32 code, char *buf)
{
    size_t ret = 0;

    if (code < 0x10000)
    {
        buf[ret++] = code & 0xFF;
        buf[ret++] = code >> 8;
    }
    else
    {
        code -= 0x10000;
        u16 high = 0xD800 + (code >> 10);
        u16 low = 0xDC00 + (code & 0x3FF);
        buf[ret++] = high & 0xFF;
        buf[ret++] = high >> 8;
        buf[ret++] = low & 0xFF;
        buf[ret++] = low >> 8;
    }

    return ret;
}

static size_t
utf8to16_length(const char *str)
{
    size_t ret = 0;

    u8 *buf = (u8*)str;
    while (buf[0] != '\0')
    {
        u32 code = utf8_to_unicode(&buf);
        if (code < 0x10000)
            ret += 2;
        else
            ret += 4;
    }

    return ret;
}

static size_t
id83_to_string(u8 *code, char* buf)
{
    size_t ret = 0;

    char id83[12] = {0};
    char name8[9] = {0};
    char ext3[4] = {0};

    mem_copy(name8, code, 8);
    for (int k = 7; k >= 0 && name8[k] == ' '; k--)
        name8[k] = '\0';

    mem_copy(ext3, &(code[8]), 3);
    for (int k = 2; k >= 0 && ext3[k] == ' '; k--)
        ext3[k] = '\0';

    str_copy(id83, name8, 0);
    if (ext3[0] != '\0')
        str_concat(id83, ".", 0);
    str_concat(id83, ext3, 0);

    ret = str_length(id83) + 1;
    mem_copy(buf, id83, ret);

    return ret;
}

static void
string_to_id83(const char *str, u8 *buf)
{
    u32 len = str_length(str);
    if (str_length(str) <= 11)
    {
        mem_fill(buf, 0x20, 11);
        mem_copy(buf, str, len);
    }
    else
        mem_copy(buf, str, 11);
}

static u8
lfn_checksum(u8 *buf)
{
    u8 ret = 0;

    for (u8 i = 11; i != 0; i--)
        ret = ((ret & 1) << 7) + (ret >> 1) + buf[11 - i];

    return ret;
}

/* Cluster functions */

enum
{
    CLUSTER_FREE = 0x0,
    CLUSTER_EOF = 0x0FFFFFF8,
    CLUSTER_MASK = 0x0FFFFFFF
};

static bool
cluster_eof(u32 cluster)
{
    return (cluster & 0x0FFFFFFF) >= 0x0FFFFFF8;
}

static bool
cluster_set(struct fat32 *f, u32 cluster, u32 value)
{
    bool ret = true;

    u32 idx = f->br.reservedsects + (cluster / 0x80);
    for (u32 i = 0; ret && i < f->br.tablecount; i++)
    {
        if ((fat32_buf_n == idx) || disk_read(f->disk, (u8*)fat32_buf, idx, 0))
        {
            u32 org = fat32_buf[cluster % 0x80];
            u32 value2 = (org & ~CLUSTER_MASK) | (value & CLUSTER_MASK);
            fat32_buf[cluster % 0x80] = value2;

            ret = disk_write(f->disk, (u8*)fat32_buf, idx, 0);
            fat32_buf_n = idx;
        }
        else
            ret = false;

        idx += f->br.sectspertable;
    }

    return ret;
}

static u32
cluster_get(struct fat32 *f, u32 cluster)
{
    u32 ret = CLUSTER_EOF;

    u32 idx = f->br.reservedsects + (cluster / 0x80);
    if ((fat32_buf_n == idx) || disk_read(f->disk, (u8*)fat32_buf, idx, 0))
    {
        ret = fat32_buf[cluster % 0x80] & CLUSTER_MASK;
        fat32_buf_n = idx;
    }

    return ret;
}

static u32
cluster_find(struct fat32 *f, u32 value)
{
    u32 ret = 0;

    for (u32 i = 2; i < 0x200 * f->br.sectspertable / 4; i++)
    {
        if (cluster_get(f, i) == value)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

/* Parsing functions */

static u32
fat32_next(struct fat32 *f, u32 cluster)
{
    u32 ret = 0xFFFFFFFF;

    u32 length = (f->br.sectsvolume) ? f->br.sectsvolume : f->br.sectsvolume32;
    if (cluster < length)
        ret = cluster_get(f, cluster);

    return ret;
}

static u32
fat32_fsector(struct fat32 *f, u32 cluster)
{
    u32 datasect = f->br.reservedsects +
                                      (f->br.tablecount * f->br.sectspertable);
    return datasect + ((cluster - 2) * f->br.sectspercluster);
}

static bool
fat32_walk(struct fat32 *f, u32 cluster, u32 sect, u32 idx,
           struct fat32e *out, bool *dir, char *name, u32 *size)
{
    bool ret = true;

    static char name2[255] = {0};
    if (!name)
        name = name2;

    static u16 lfn[255] = {0};
    static u8 lfn_last = 0;
    mem_fill(lfn, 0xFF, sizeof(lfn));

    bool found = false;
    u32 firstsect = 0;
    for (u32 i = 0x200 * f->br.sectspercluster; ret; i += 32)
    {
        if (i >= (0x200 * f->br.sectspercluster))
        {
            if (!cluster_eof(cluster))
            {
                firstsect = fat32_fsector(f, cluster);
                if (firstsect >= sect)
                {
                    for (u8 k = 0; ret && k < f->br.sectspercluster; k++)
                        ret = disk_read(f->disk, &(f->buffer[0x200 * k]),
                                        firstsect + k, 0);
                }
                if (!ret)
                    break;
                i = 0;
            }
            else
                break;
            cluster = fat32_next(f, cluster);
        }

        if (firstsect < sect)
            continue;
        if (firstsect == sect && i <= idx)
            continue;

        if (f->buffer[i] == 0x00)
            break;
        if (f->buffer[i] == 0x05 || f->buffer[i] == 0xE5)
            continue;

        if (f->buffer[i + 11] == 0x0F)
        {
            u16 idx = f->buffer[i] & 0x1F;
            if (idx > 0 && idx < 256)
            {
                idx = (idx - 1) * 13;
                mem_copy(&(lfn[idx]),      &(f->buffer[i + 1]),  5 * 2);
                mem_copy(&(lfn[idx + 5]),  &(f->buffer[i + 14]), 6 * 2);
                mem_copy(&(lfn[idx + 11]), &(f->buffer[i + 28]), 2 * 2);

                u8 last = idx + 13;
                if (last > lfn_last)
                    lfn_last = last;
            }
            continue;
        }

        size_t j = 0;
        for (u16 *lfn_p = lfn; lfn_p <= &(lfn[lfn_last]); lfn_p = &(lfn_p[1]))
        {
            if (*lfn_p == 0xFFFF)
                continue;

            j += unicode_to_utf8(utf16_to_unicode(&lfn_p), &(name[j]));
        }
        mem_fill(lfn, 0xFF, sizeof(lfn));

        if (j > 0)
            name[j] = '\0';
        else
            id83_to_string(&(f->buffer[i]), name);

        u8 *entry = &(f->buffer[i]);

        out->sect = firstsect;
        out->idx  = i;
        out->location = (firstsect * 0x200) + i;
        out->attributes = entry[11];
        mem_copy(&(out->size), &(entry[28]), 4);

        u16 cluster_l, cluster_h;
        mem_copy(&cluster_l, &(entry[26]), 2);
        mem_copy(&cluster_h, &(entry[20]), 2);
        out->cluster = cluster_l | (cluster_h << 16);

        if (dir)
            *dir  = (out->attributes & 0x10);
        if (size)
            *size = out->size;

        found = true;
        ret = true;
        break;
    }

    return found;
}

/* Allocation functions */

static u32
fat32_alloc(struct fat32 *f)
{
    u32 ret = cluster_find(f, CLUSTER_FREE);

    if (ret)
        if (!cluster_set(f, ret, CLUSTER_EOF))
            ret = CLUSTER_EOF;

    return ret;
}

static u32
fat32_alloc2(struct fat32 *f, u32 cluster)
{
    u32 ret = fat32_next(f, cluster);

    if (cluster_eof(ret))
    {
        u32 cluster2 = cluster_find(f, CLUSTER_FREE);
        if (cluster2 && !cluster_eof(cluster))
        {
            if (cluster_set(f, cluster,  cluster2) &&
                cluster_set(f, cluster2, CLUSTER_EOF))
                ret = cluster2;
            else
                ret = CLUSTER_EOF;
        }
    }

    return ret;
}

static bool
fat32_free(struct fat32 *f, u32 cluster, u32 depth)
{
    bool ret = true;

    for (u32 i = 0; ret && !cluster_eof(cluster); i++)
    {
        u32 prev = cluster;
        cluster = fat32_next(f, cluster);

        if ((i + 1) == depth)
            ret = cluster_set(f, prev, CLUSTER_EOF);
        else if ((i + 1) > depth)
            ret = cluster_set(f, prev, CLUSTER_FREE);
    }

    return ret;
}

/* Internal interface */

static struct fat32 *
fat32_del(struct fat32 *f)
{
    if (f)
    {
        mem_del(f->buffer);
        mem_del(f);
    }

    return NULL;
}

static struct fat32 *
fat32_new(u8 disk)
{
    struct fat32 *ret = mem_new(sizeof(struct fat32));

    if (ret)
    {
        ret->disk = disk;
        if (!disk_read(ret->disk, (u8*)fat32_buf, 0, 0))
            ret = fat32_del(ret);
    }

    if (ret)
    {
        mem_copy(&(ret->br), fat32_buf, sizeof(struct fat32br));

        ret->buffer = mem_new(0x200 * ret->br.sectspercluster);
        if (!(ret->buffer))
            ret = fat32_del(ret);
    }

    if (ret)
    {
        ret->root.attributes = 0x10;
        ret->root.cluster = ret->br.rootcluster;
    }

    return ret;
}

static bool
fat32_rw(struct fat32 *f, struct fat32e *fe, u32 sector, u8 *data, bool write)
{
    bool ret = !(fe->attributes & 0x10);

    if (ret)
    {
        u32 cluster_n = sector / f->br.sectspercluster;
        u32 sector_n = sector % f->br.sectspercluster;

        u32 cluster = fe->cluster;
        for (u32 i = 0; i < cluster_n && !cluster_eof(cluster); i++)
            cluster = fat32_next(f, cluster);

        if (!cluster_eof(cluster))
        {
            cluster &= 0xFFFFFFF;
            u32 firstsect = fat32_fsector(f, cluster);
            ret = ((write) ? disk_write : disk_read)(f->disk, data,
                                                     firstsect + sector_n, 0);
        }
    }

    return ret;
}

static bool
entry_update(struct fat32 *f, struct fat32e *fe, bool unused)
{
    bool ret = true;

    u32 sector = fe->location / 0x200;
    u32 index = fe->location % 0x200;
    ret = disk_read(f->disk, f->buffer, sector, 0);

    if (ret)
    {
        if (unused)
        {
            for (s64 i = index; true; i -= 32)
            {
                if (i != index && f->buffer[i + 11] != 0x0F)
                    break;

                mem_fill(&(f->buffer[i]), 0xE5, 1);
                mem_fill(&(f->buffer[i + 1]), 0x00, 31);

                if (i == 0)
                {
                    ret = disk_write(f->disk, f->buffer, sector--, 0) &&
                          disk_read (f->disk, f->buffer, sector,   0);
                    i = 0x200;
                }
            }
        }
        else
        {
            u32 size = fe->size;
            mem_copy(&(f->buffer[index + 28]), &size, sizeof(u32));
        }

        if (ret)
            ret = disk_write(f->disk, f->buffer, sector, 0);
    }

    return ret;
}

static bool
fat32_resize(struct fat32 *f, struct fat32e *fe, u32 size)
{
    bool ret = true;

    u32 cluster_n = 0;
    for (u32 c = fe->cluster; !cluster_eof(c); c = fat32_next(f, c))
        cluster_n++;

    u32 cluster_n2 = (size / 0x200) / f->br.sectspercluster;
    if (cluster_n != cluster_n2)
    {
        if (cluster_n2 < cluster_n)
        {
            ret = fat32_free(f, fe->cluster, cluster_n2 + 1);
            fe->size = size;
        }
        else if (cluster_n2 > cluster_n)
        {
            u32 cluster = fe->cluster;
            for (u32 i = 0; ret && i <= cluster_n2; i++)
            {
                cluster = fat32_alloc2(f, cluster);
                if (cluster_eof(cluster))
                    ret = false;
            }

            if (ret)
                fe->size = size;
        }
    }
    else
        fe->size = size;

    if (ret)
        ret = entry_update(f, fe, false);

    return ret;
}

static void
fill_entry(u8 *buf, const char *name, bool dir, u8 entries, u32 cluster)
{
    mem_fill(buf, 0x0, 0x200);

    u8 id83[11] = {0};
    string_to_id83(name, id83);

    u8 *name2 = (u8*)name;
    u8 lfn_c = entries - 1;
    u8 checksum = lfn_checksum(id83);
    for (u32 n = 0; n < lfn_c; n++)
    {
        u32 i = ((lfn_c - n - 1) * 32);

        buf[i] = n + 1 + ((n == (u32)(lfn_c - 1)) ? 0x40 : 0x0);
        buf[i + 11] = 0xF;
        buf[i + 13] = checksum;

        bool last = true;
        u8 start[] = {1, 14, 28}, end[] = {11, 26, 32};
        for (u8 j = 0; j < 3; j++)
        {
            u8 *cur = &(buf[i + start[j]]);
            while (cur < &(buf[i + end[j]]))
            {
                if (name2[0])
                {
                    u32 code = utf8_to_unicode(&name2);
                    cur += unicode_to_utf16(code, (char *)cur);
                }
                else
                {
                    if (last)
                        last = false;
                    else
                    {
                        cur[0] = 0xFF;
                        cur[1] = 0xFF;
                    }
                    cur += 2;
                }
            }
        }
    }

    u32 i = lfn_c * 32;
    mem_copy(&(buf[i]), id83, 11);
    buf[i + 11] = 0x20 | ((dir) ? 0x10 : 0x0);
    buf[i + 26] = (cluster >> 0) & 0xFF;
    buf[i + 27] = (cluster >> 8) & 0xFF;
    buf[i + 20] = (cluster >> 16) & 0xFF;
    buf[i + 21] = (cluster >> 24) & 0xFF;

    /* Unused date/time slots */
    buf[i + 16] = 0x21;
    buf[i + 18] = 0x21;
    buf[i + 24] = 0x21;
}

static bool
chain_write(struct fat32 *f, u8 *buf, u32 cluster, u32 sector_idx,
            u32 pos, u32 bytes)
{
    bool ret = true;

    u32 idx = 0;
    while (ret && bytes)
    {
        u32 sector = fat32_fsector(f, cluster) + sector_idx;
        ret = disk_read(f->disk, f->buffer, sector, 0);

        if (ret)
        {
            u32 write = (bytes < (0x200 - pos)) ? bytes : (0x200 - pos);
            if (pos + bytes <= 0x200)
            {
                mem_copy(&(f->buffer[pos]), &(buf[idx]), write);
                idx += write;
                bytes -= write;
            }
            else
                mem_fill(&(f->buffer[pos]), 0xE5, write);
            ret = disk_write(f->disk, f->buffer, sector, 0);
        }

        if (ret && bytes)
        {
            pos = 0;

            sector_idx++;
            if (sector_idx >= f->br.sectspercluster)
            {
                sector_idx = 0;

                cluster = fat32_alloc2(f, cluster);
                if (cluster_eof(cluster))
                    ret = false;
            }
        }
    }

    return ret && !bytes;
}

static bool
directory_dots(struct fat32 *f, u32 cluster, u32 pcluster)
{
    bool ret = false;

    u32 sector = fat32_fsector(f, cluster);
    ret = disk_read(f->disk, f->buffer, sector, 0);
    if (ret)
    {
        u8 *buf = f->buffer;
        mem_fill(buf, 0x0, 32 * 3);

        u32 i = 0;
        mem_fill(&(buf[i]), 0x20, 11);
        buf[i] = '.';
        buf[i + 11] = 0x30;
        buf[i + 26] = (cluster >> 0) & 0xFF;
        buf[i + 27] = (cluster >> 8) & 0xFF;
        buf[i + 20] = (cluster >> 16) & 0xFF;
        buf[i + 21] = (cluster >> 24) & 0xFF;

        i += 32;
        mem_fill(&(buf[i]), 0x20, 11);
        buf[i] = '.';
        buf[i + 1] = '.';
        buf[i + 11] = 0x30;
        buf[i + 26] = (pcluster >> 0) & 0xFF;
        buf[i + 27] = (pcluster >> 8) & 0xFF;
        buf[i + 20] = (pcluster >> 16) & 0xFF;
        buf[i + 21] = (pcluster >> 24) & 0xFF;

        ret = disk_write(f->disk, f->buffer, sector, 0);
    }

    return ret;
}

static bool
fat32_create(struct fat32 *f, u32 pcluster,
             bool dir, const char *name, u32 cluster)
{
    bool ret = (f && name);

    u32 entries = 0;
    if (ret)
    {
        u32 length = utf8to16_length(name);
        if (length <= 255)
            entries = (length / 13) + 2;
        else
            ret = false;
    }

    if (ret)
    {
        u16 pos = 0;
        bool found = false;
        u32 sector = fat32_fsector(f, pcluster), sector_idx = 0;
        u16 unused_st = 0, unused_c = 0;
        while (!found)
        {
            if (disk_read(f->disk, f->buffer, sector, 0))
            {
                for (u32 i = 0; i < 0x200; i += 32)
                {
                    if (f->buffer[i] == 0x0)
                    {
                        pos = i;
                        found = true;
                        break;
                    }

                    if (f->buffer[i] == 0xE5)
                    {
                        if (unused_c == 0)
                            unused_st = i;
                        unused_c++;

                        if (unused_c == entries)
                        {
                            pos = unused_st;
                            found = true;
                            break;
                        }
                    }
                    else
                        unused_c = 0;
                }
            }
            if (found)
                break;

            sector_idx++;
            if (sector_idx < f->br.sectspercluster)
                sector += 1;
            else
            {
                pcluster = fat32_alloc2(f, pcluster);
                if (!cluster_eof(pcluster))
                {
                    sector = fat32_fsector(f, pcluster);
                    sector_idx = 0;
                }
                else
                {
                    ret = false;
                    break;
                }
            }
        }

        if (found)
        {
            if (ret && cluster == 0)
            {
                cluster = fat32_alloc(f);
                if (cluster)
                {
                    if (dir)
                        ret = directory_dots(f, cluster, pcluster);
                }
                else
                    ret = false;
            }

            if (ret)
            {
                fill_entry(fat32_buf2, name, dir, entries, cluster);
                fat32_buf_n = -1;

                u32 bytes = (entries * 32) + ((unused_st != pos) ? 32 : 0);
                ret = chain_write(f, fat32_buf2, pcluster,
                                  sector_idx, pos, bytes);
            }
        }
        else
            ret = false;
    }

    return ret;
}

static bool
fat32_remove(struct fat32 *f, struct fat32e *fe, bool clusters)
{
    bool ret = true;

    if (clusters)
        ret = fat32_free(f, fe->cluster, 0);

    if (ret)
    {
        fe->size = 0;
        ret = entry_update(f, fe, true);
    }

    return ret;
}

/* Driver definition */

static void *
root(void *ctx)
{
    void *ret = NULL;

    struct fat32 *f = ctx;
    if (f)
        ret = &(f->root);

    return ret;
}

static void *
walk(void *ctx, void *parent, void *entry,
     bool *dir, char *name, u32 *size)
{
    void *ret = NULL;

    struct fat32e *fd = parent;
    if (ctx && parent && (fd->attributes & 0x10))
    {
        struct fat32e *fe = entry;

        u32 sect = 0, idx = 0;
        if (fe)
        {
            sect = fe->sect;
            idx  = fe->idx;
        }

        struct fat32e *new = mem_new(sizeof(struct fat32e));
        if (fat32_walk(ctx, fd->cluster, sect, idx, new, dir, name, size))
            ret = new;
        else
            mem_del(new);

        mem_del(entry);
    }

    return ret;
}

static bool
read(void *ctx, void *entry, u8 *data, u32 block)
{
    return fat32_rw(ctx, entry, block, data, false);
}

static bool
write(void *ctx, void *entry, u8 *data, u32 block)
{
    return fat32_rw(ctx, entry, block, data, true);
}

static bool
resize(void *ctx, void *entry, u32 size)
{
    bool ret = (ctx && entry);

    if (ret)
        ret = fat32_resize(ctx, entry, size);

    return ret;
}

static bool
create(void *ctx, void *parent, const char *name, bool dir)
{
    bool ret = (ctx && parent && name);

    struct fat32e *fe = parent;
    if (ret)
        ret = fat32_create(ctx, fe->cluster, dir, name, 0);

    return ret;
}

static bool
remove(void *ctx, void *entry)
{
    bool ret = (ctx && entry);

    struct fat32 *f = ctx;
    if (ret)
        ret = (entry != &(f->root));
    if (ret)
        ret = fat32_remove(ctx, entry, true);

    return ret;
}

static const drv_fs fat32 =
{
    .root = root, .walk = walk,
    .read = read, .write = write,
    .resize = resize, .create = create, .remove = remove
};

/* Device creation */

extern dev_fs
fat32_init(u8 disk)
{
    void *ret = NULL;

    u16 sector = 0;
    if (disk_size(disk, &sector, NULL) && sector == 0x200)
        ret = fat32_new(disk);

    return (dev_fs){.driver = &fat32, .context = ret};
}

extern void
fat32_clean(dev_fs *f)
{
    if (f)
        f->context = fat32_del(f->context);
}
