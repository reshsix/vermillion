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

#include <general/types.h>
#include <general/mem.h>
#include <general/str.h>
#include <general/path.h>

#include <hal/classes/fs.h>

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
    char *name;
    u8 attributes;

    u32 location;
    bool unused;

    u32 cluster;
    size_t size;

    u32 created;
    u32 accessed;
    u32 modified;

    struct
    {
        struct fat32e *data;
        size_t count;
        size_t length;
    } files;
};

struct fat32
{
    dev_block *storage;

    struct fat32br br;
    u8 *buffer;
    u32 *table;
    struct fat32e root;

    struct fat32e *current;
};

alignas(32) static u8 fat32_buf[0x200];

/* Helper functions */

static u32
fat32t_to_unix(u16 time, u16 date)
{
    u32 ret = 0;

    u8 year = ((date >> 9) & 0x7f) + 10;
    u8 day = (date & 0x1f) - 1;
    u8 month = ((date >> 5) & 0xf) - 1;
    ret += ((year * 36525 / 100) + (month * 36525 / 1200) + day) * 86400;

    u8 seconds = (time & 0x1f) * 2;
    u8 minutes = (time >> 5) & 0x3f;
    u8 hours = (time >> 11) & 0x1f;
    ret += seconds + (minutes * 60) + (hours * 3600);

    return ret;
}

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

static void
cluster_set(u32 *cluster, u32 value)
{
    *cluster = (*cluster & ~CLUSTER_MASK) | (value & CLUSTER_MASK);
}

static u32
cluster_get(u32 *cluster)
{
    return *cluster & CLUSTER_MASK;
}

static u32
cluster_find(struct fat32 *f, u32 value)
{
    u32 ret = 0;

    for (u32 i = 2; i < 0x200 * f->br.sectspertable / 4; i++)
    {
        if (cluster_get(&(f->table[i])) == value)
        {
            ret = i;
            break;
        }
    }

    return ret;
}

/* Entry constructor/destructor */

static bool
fat32e_clean(struct fat32e *fe)
{
    if (fe)
    {
        for (size_t i = 0; i < fe->files.count; i++)
            fat32e_clean(&(fe->files.data[i]));

        mem_del(fe->name);
        mem_del(fe->files.data);
    }

    return false;
}

static bool
fat32e_init(struct fat32e *fe, const char *name, u8 *entry, u32 location)
{
    bool ret = true;

    fe->name = str_dupl(name, 0);
    if (!(fe->name))
        ret = fat32e_clean(fe);

    if (ret)
    {
        fe->location = location;
        fe->attributes = entry[11];

        u16 time, date;
        mem_copy(&time, &(entry[14]), 2);
        mem_copy(&date, &(entry[16]), 2);
        fe->created = fat32t_to_unix(time, date);
        mem_copy(&date, &(entry[18]), 2);
        fe->accessed = fat32t_to_unix(0, date);
        mem_copy(&time, &(entry[22]), 2);
        mem_copy(&date, &(entry[24]), 2);
        fe->modified = fat32t_to_unix(time, date);

        mem_copy(&(fe->size), &(entry[28]), 4);

        u16 cluster_l, cluster_h;
        mem_copy(&cluster_l, &(entry[26]), 2);
        mem_copy(&cluster_h, &(entry[20]), 2);
        fe->cluster = cluster_l | (cluster_h << 16);

        if (fe->attributes & 0x10)
        {
            fe->files.length = 16;
            fe->files.data = mem_new(fe->files.length * sizeof(struct fat32e));
            if (!(fe->files.data))
                ret = fat32e_clean(fe);
        }
    }

    return ret;
}

static bool
fat32e_grow(struct fat32e *fe)
{
    bool ret = true;

    if ((fe->files.count + 1) >= fe->files.length)
    {
        void *new = mem_renew(fe->files.data,
                              sizeof(struct fat32e) * fe->files.length * 2);
        if (new != NULL)
        {
            fe->files.data = new;
            fe->files.length *= 2;
        }
        else
            ret = false;
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
        ret = cluster_get(&(f->table[cluster]));

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
fat32_directory(struct fat32 *f, u32 cluster, struct fat32e *out)
{
    bool ret = true;

    u32 firstsect = 0;

    static char name[(255 * 4) + 1] = {0};
    static u16 lfn[255] = {0};
    static u8 lfn_last = 0;
    mem_fill(lfn, 0xFF, sizeof(lfn));
    for (u32 i = 0x200 * f->br.sectspercluster; ret; i += 32)
    {
        if (i >= (0x200 * f->br.sectspercluster))
        {
            if (!cluster_eof(cluster))
            {
                firstsect = fat32_fsector(f, cluster);
                for (u8 k = 0; ret && k < f->br.sectspercluster; k++)
                    ret = block_read(f->storage, BLOCK_COMMON,
                                     &(f->buffer[0x200 * k]), firstsect + k);
                i = 0;
            }
            else
                break;
            cluster = fat32_next(f, cluster);
        }

        if (f->buffer[i] == 0x00)
            break;

        if (f->buffer[i] == 0x05 || f->buffer[i] == 0xE5)
            continue;

        if (f->buffer[i + 11] == 0xF)
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

        if (str_comp(name, ".", 0) != 0 && str_comp(name, "..", 0) != 0)
        {
            if (fat32e_grow(out))
            {
                struct fat32e *fe = &(out->files.data[out->files.count++]);
                ret = fat32e_init(fe, name, &(f->buffer[i]),
                                  (firstsect * 0x200) + i);
            }
        }
    }

    return ret;
}

static bool
fat32_transverse(struct fat32 *f, u32 cluster, struct fat32e *out)
{
    bool ret = true;

    ret = fat32_directory(f, cluster, out);
    for (size_t i = 0; ret && i < out->files.count; i++)
    {
        if (out->files.data[i].attributes & 0x10)
        {
            ret = fat32_transverse(f, out->files.data[i].cluster,
                                   &(out->files.data[i]));
        }
    }

    return ret;
}

/* Internal interface */

static struct fat32 *
fat32_del(struct fat32 *f)
{
    if (f)
    {
        fat32e_clean(&(f->root));
        mem_del(f->buffer);
        mem_del(f);
    }

    return NULL;
}

static struct fat32 *
fat32_new(dev_block *storage)
{
    struct fat32 *ret = mem_new(sizeof(struct fat32));

    if (ret)
    {
        ret->storage = storage;

        if (!block_read(ret->storage, BLOCK_COMMON, fat32_buf, 0))
            ret = fat32_del(ret);
    }

    if (ret)
    {
        mem_copy(&(ret->br), fat32_buf, sizeof(struct fat32br));

        ret->buffer = mem_new(0x200 * ret->br.sectspercluster);
        ret->table = mem_new(0x200 * ret->br.sectspertable);
        if (!(ret->buffer && ret->table))
            ret = fat32_del(ret);
    }

    if (ret)
    {
        bool success = true;
        for (u32 i = 0; success && i < ret->br.sectspertable; i++)
            success = block_read(ret->storage, BLOCK_COMMON,
                                 &(((u8*)(ret->table))[0x200 * i]),
                                 ret->br.reservedsects + i);

        if (!success)
            ret = fat32_del(ret);
    }

    if (ret)
    {
        ret->root.files.length = 16;
        ret->root.files.data = mem_new(ret->root.files.length *
                                       sizeof(struct fat32e));
        if (!(ret->root.files.data))
            ret = fat32_del(ret);
    }

    if (ret)
    {
        if (fat32_transverse(ret, ret->br.rootcluster, &(ret->root)))
        {
            ret->root.attributes = 0x10;
            ret->root.cluster = ret->br.rootcluster;

            ret->root.name = str_dupl("/", 0);
            if (!(ret->root.name))
                ret = fat32_del(ret);
        }
        else
            ret = fat32_del(ret);
    }

    return ret;
}

static struct fat32e *
fat32_find(struct fat32 *f, char *path)
{
    bool found = true;

    struct fat32e *cur = &(f->root);
    char *path2 = str_dupl(path, 0), *state = NULL;
    if (path2)
    {
        for (char *token = str_token(path2, "/", &state); token;
                   token = str_token(NULL, "/", &state))
        {
            found = false;
            for (size_t i = 0; i < cur->files.count; i++)
            {
                if (cur->files.data[i].unused)
                    continue;

                if (str_comp(cur->files.data[i].name, token, 0) == 0)
                {
                    found = true;
                    cur = &(cur->files.data[i]);
                    break;
                }
            }
        }
        mem_del(path2);
    }
    else
        found = false;

    return (found) ? cur : NULL;
}

static bool
fat32_rw(struct fat32 *f, struct fat32e *fe, u32 sector, u8 *data, bool write)
{
    bool ret = false;

    u32 cluster_n = sector / f->br.sectspercluster;
    u32 sector_n = sector % f->br.sectspercluster;

    u32 cluster = fe->cluster;
    for (u32 i = 0; i < cluster_n && !cluster_eof(cluster); i++)
        cluster = fat32_next(f, cluster);

    if (!cluster_eof(cluster))
    {
        cluster &= 0xFFFFFFF;
        u32 firstsect = fat32_fsector(f, cluster);
        ret = ((write) ? block_write : block_read)(f->storage, BLOCK_COMMON,
                                                   data, firstsect + sector_n);
    }

    return ret;
}

static bool
table_update(struct fat32 *f)
{
    bool ret = true;

    for (u32 i = 0; ret && i < f->br.tablecount; i++)
    {
        for (u32 j = 0; ret && j < f->br.sectspertable; j++)
            ret = block_write(f->storage, BLOCK_COMMON,
                              &(((u8*)(f->table))[0x200 * j]),
                              f->br.reservedsects + j +
                              (i * f->br.sectspertable));
    }

    return ret;
}

static bool
entry_update(struct fat32 *f, struct fat32e *fe)
{
    bool ret = true;

    u32 sector = fe->location / 0x200;
    u32 index = fe->location % 0x200;
    ret = block_read(f->storage, BLOCK_COMMON, f->buffer, sector);

    if (ret)
    {
        if (fe->unused)
        {
            for (s64 i = index; true; i -= 32)
            {
                if (i != index && f->buffer[i + 11] != 0x0F)
                    break;

                mem_fill(&(f->buffer[i]), 0xE5, 1);
                mem_fill(&(f->buffer[i + 1]), 0x00, 31);

                if (i == 0)
                {
                    ret = block_write(f->storage, BLOCK_COMMON, f->buffer,
                                      sector--) &&
                          block_read(f->storage, BLOCK_COMMON, f->buffer,
                                     sector);
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
            ret = block_write(f->storage, BLOCK_COMMON, f->buffer, sector);
    }

    return ret;
}

static bool
fat32_resize(struct fat32 *f, struct fat32e *fe, u32 size)
{
    bool ret = true;

    u32 cluster_n = (fe->size / 0x200) / f->br.sectspercluster;
    u32 cluster_n2 = (size / 0x200) / f->br.sectspercluster;
    if (cluster_n != cluster_n2)
    {
        if (size < fe->size)
        {
            u32 cluster = fe->cluster;
            for (u32 i = 0; i <= cluster_n && !cluster_eof(cluster); i++)
            {
                u32 prev = cluster;
                cluster = fat32_next(f, cluster);

                if (i == cluster_n2)
                    cluster_set(&(f->table[prev]), CLUSTER_EOF);
                else if (i > cluster_n2)
                    cluster_set(&(f->table[prev]), CLUSTER_FREE);
            }
            fe->size = size;
        }
        else if (size > fe->size)
        {
            u32 cluster = fe->cluster;
            for (u32 i = 0; i <= cluster_n2; i++)
            {
                u32 prev = cluster;
                cluster = fat32_next(f, cluster);
                if (cluster_eof(cluster))
                {
                    cluster = cluster_find(f, CLUSTER_FREE);
                    if (cluster && !cluster_eof(cluster))
                    {
                        cluster_set(&(f->table[prev]), cluster);
                        fe->size += f->br.sectspercluster * 0x200;
                    }
                    else
                    {
                        ret = false;
                        break;
                    }
                }
            }

            if (ret)
            {
                cluster_set(&(f->table[cluster]), CLUSTER_EOF);
                fe->size = size;
            }
        }
    }
    else
        fe->size = size;

    if (ret)
        ret = entry_update(f, fe) && table_update(f);

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
        ret = block_read(f->storage, BLOCK_COMMON, f->buffer, sector);

        if (ret)
        {
            u32 write = (bytes < (0x200 - pos)) ? bytes : (0x200 - pos);
            mem_copy(&(f->buffer[pos]), &(buf[idx]), write);
            idx += write;
            bytes -= write;

            ret = block_write(f->storage, BLOCK_COMMON, f->buffer, sector);
        }

        if (ret && bytes)
        {
            pos = 0;

            sector_idx++;
            if (sector_idx >= f->br.sectspercluster)
            {
                sector_idx = 0;
                cluster = fat32_next(f, cluster);
                if (cluster_eof(cluster))
                {
                    u32 new = cluster_find(f, CLUSTER_FREE);
                    cluster_set(&(f->table[cluster]), new);
                    cluster_set(&(f->table[new]), CLUSTER_EOF);
                    cluster = new;
                }
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
    ret = block_read(f->storage, BLOCK_COMMON, f->buffer, sector);
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

        ret = block_write(f->storage, BLOCK_COMMON, f->buffer, sector);
    }

    return ret;
}

static bool
fat32_create(struct fat32 *f, struct fat32e *fe, const char *name, bool dir,
             u32 cluster, u32 size)
{
    bool ret = (f && fe && fe->attributes & 0x10 && name);

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
        u32 pcluster = fe->cluster;
        u32 sector = fat32_fsector(f, pcluster), sector_idx = 0;
        u16 unused_st = 0, unused_c = 0;
        while (!found)
        {
            if (block_read(f->storage, BLOCK_COMMON, f->buffer, sector))
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
                pcluster = fat32_next(f, pcluster);
                if (!cluster_eof(pcluster))
                {
                    sector = fat32_fsector(f, pcluster);
                    sector_idx = 0;
                }
                else
                    break;
            }
        }

        if (found)
        {
            if (ret && cluster == 0)
            {
                u32 new = cluster_find(f, CLUSTER_FREE);
                cluster_set(&(f->table[new]), CLUSTER_EOF);
                cluster = new;
                if (dir)
                    ret = directory_dots(f, new, pcluster);
            }

            if (ret)
            {
                fill_entry(fat32_buf, name, dir, entries, cluster);
                u32 bytes = (entries * 32) + ((unused_st != pos) ? 32 : 0);
                ret = chain_write(f, fat32_buf, pcluster,
                                  sector_idx, pos, bytes);
            }
            if (ret)
                ret = table_update(f);

            if (ret && fat32e_grow(fe))
            {
                u32 location = ((fat32_fsector(f, pcluster) +
                                 sector_idx) * 0x200) + pos;
                struct fat32e *new = &(fe->files.data[fe->files.count++]);
                if (new)
                    ret = fat32e_init(new, name,
                                      &(fat32_buf[(entries - 1) * 32]),
                                      location);
                else
                    ret = false;

                if (ret)
                    new->size = size;
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
    {
        u32 cluster_n = (fe->size / 0x200) / f->br.sectspercluster;

        u32 cluster = fe->cluster;
        for (u32 i = 0; i <= cluster_n && !cluster_eof(cluster); i++)
        {
            u32 prev = cluster;
            cluster = fat32_next(f, cluster);
            cluster_set(&(f->table[prev]), CLUSTER_FREE);
        }
    }
    fe->size = 0;
    fe->unused = true;
    ret = entry_update(f, fe) && table_update(f);

    return ret;
}

static bool
fat32_rename(struct fat32 *f, struct fat32e *fe, char *path)
{
    bool ret = true;

    char *dir = path_dirname(path);
    char *name = path_filename(path);
    ret = (dir && name);

    struct fat32e *target = NULL;
    if (ret)
    {
        target = fat32_find(f, dir);
        ret = (target);
    }

    if (ret)
        ret = fat32_remove(f, fe, false) &&
              fat32_create(f, target, name, fe->attributes & 0x10,
                           fe->cluster, fe->size);

    return ret;
}

/* Driver definition */

static bool
stat(void *ctx, u32 idx, u32 *width, u32 *length)
{
    bool ret = false;

    struct fat32 *f = ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            ret = (f->current);
            if (ret)
            {
                if (f->current->attributes & 0x10)
                {
                    *width = 0;
                    *length = f->current->files.count;
                }
                else
                {
                    *width = 0x200;
                    *length = f->current->size / 0x200;
                }
            }
            break;
        case FS_FIND:
            *width = sizeof(char *);
            *length = 1;
            break;
        case FS_CACHE:
            *width = sizeof(void *);
            *length = 1;
            break;
        case FS_SWITCH:
            *width = sizeof(void *);
            *length = 1;
            break;
        case FS_WALK:
            *width = sizeof(u32);
            *length = 1;
            break;
        case FS_NAME:
            *width = sizeof(char *);
            *length = 1;
            break;
        case FS_SIZE:
            *width = sizeof(u32);
            *length = 1;
            break;
    }

    return ret;
}

static bool
read(void *ctx, u32 idx, void *data, u32 block)
{
    bool ret = false;

    struct fat32 *f = ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            ret = fat32_rw(f, f->current, block, data, false);
            break;
        case FS_CACHE:
            ret = true;
            mem_copy(data, &(f->current), sizeof(void *));
            break;
        case FS_TYPE:
            ret = (f->current);
            enum fs_type ft = (f->current->attributes & 0x10) ?
                              FS_DIRECTORY : FS_REGULAR;
            mem_copy(data, &ft, sizeof(enum fs_type));
            break;
        case FS_NAME:
            ret = (f->current);
            mem_copy(data, &(f->current->name), sizeof(char *));
            break;
        case FS_SIZE:
            ret = (f->current);
            if (ret)
            {
                u32 size = 0;
                if (f->current->attributes & 0x10)
                    size = f->current->files.count;
                else
                    size = f->current->size;
                mem_copy(data, &size, sizeof(u32));
            }
            break;
    }

    return ret;
}

static bool
write(void *ctx, u32 idx, void *data, u32 block)
{
    bool ret = false;

    struct fat32 *f = ctx;
    switch (idx)
    {
        case BLOCK_COMMON:
            ret = fat32_rw(f, f->current, block, data, true);
            break;
        case FS_FIND:;
            char *buf = NULL;
            mem_copy(&buf, data, sizeof(char *));
            f->current = fat32_find(f, buf);
            ret = (f->current);
            break;
        case FS_SWITCH:
            ret = true;
            mem_copy(&(f->current), data, sizeof(void *));
            break;
        case FS_WALK:
            ret = (f->current && f->current->attributes & 0x10);
            if (ret)
            {
                u32 index = 0;
                mem_copy(&index, data, sizeof(u32));
                if (f->current->files.count > index)
                    f->current = &(f->current->files.data[index]);
                else
                    ret = false;
            }
            break;
        case FS_NAME:
            ret = (f->current && f->current != &(f->root));
            if (ret)
            {
                char *name = NULL;
                mem_copy(&name, data, sizeof(char *));
                if (name)
                    ret = fat32_rename(f, f->current, name);
                if (ret)
                {
                    f->current = fat32_find(f, name);
                    ret = (f->current);
                }
            }
            break;
        case FS_SIZE:
            ret = (f->current && !(f->current->attributes & 0x10));
            if (ret)
            {
                u32 size = 0;
                mem_copy(&size, data, sizeof(u32));
                ret = fat32_resize(f, f->current, size);
            }
            break;
        case FS_MKFILE:
        case FS_MKDIR:
            char *buf2 = NULL;
            mem_copy(&buf2, data, sizeof(char *));

            struct fat32e *fe = fat32_find(f, buf2);
            if (!fe)
            {
                char *dir = path_dirname(buf2);
                char *name = path_filename(buf2);
                ret = (dir && name);

                struct fat32e *target = NULL;
                if (ret)
                {
                    target = fat32_find(f, dir);
                    ret = (target);
                }

                if (ret)
                    ret = fat32_create(f, target, name,
                                       (idx == FS_MKDIR), 0, 0);

                mem_del(dir);
                mem_del(name);
            }
            else
            {
                if (((fe->attributes & 0x10) && idx == FS_MKFILE) ||
                    (!(fe->attributes & 0x10) && idx == FS_MKDIR))
                    ret = false;
            }
            break;
        case FS_REMOVE:
            ret = true;

            char *buf3 = NULL;
            mem_copy(&buf3, data, sizeof(char *));
            struct fat32e *target = fat32_find(f, buf3);
            if (target == &(f->root))
                ret = false;

            if (ret && target)
                ret = fat32_remove(f, target, true);
            break;
    }

    return ret;
}

static const drv_fs fat32 =
{
    .stat = stat, .read = read, .write = write
};

/* Device creation */

extern dev_fs
fat32_init(dev_block *storage)
{
    void *ret = NULL;

    if (storage)
    {
        u32 width = 0;
        if (block_stat(storage, BLOCK_COMMON, &width, NULL) && width == 0x200)
            ret = fat32_new(storage);
    }

    return (dev_fs){.driver = &fat32, .context = ret};
}

extern void
fat32_clean(dev_fs *f)
{
    if (f)
        f->context = fat32_del(f->context);
}
