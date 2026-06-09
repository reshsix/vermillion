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

#define VERMILLION_INTERNALS
#include <vermillion/hal/disk.h>
#include <vermillion/sys/file.h>
#include <vermillion/util/mem.h>
#include <vermillion/util/str.h>
#include <vermillion/util/types.h>

struct __attribute__((packed)) fat32br
{
    uint8_t  jmp[3];
    char     oem[8];
    uint16_t bytepersect;
    uint8_t  sectspercluster;
    uint16_t reservedsects;
    uint8_t  tablecount;
    uint16_t rootentries;
    uint16_t sectsvolume;
    uint8_t  mediatype;
    uint16_t _unused;
    uint16_t sectspertrack;
    uint16_t headcount;
    uint32_t hiddensects;
    uint32_t sectsvolume32;
    uint32_t sectspertable;
    uint16_t flags;
    uint16_t version;
    uint32_t rootcluster;
    uint16_t fsinfosect;
    uint16_t backupsect;
    uint8_t _reserved[12];
    uint8_t  drivenum;
    uint8_t  flagsnt;
    uint8_t  signature;
    uint32_t volumeid;
    char     label[11];
    char     system[8];
};

struct fat32e
{
    uint8_t attributes;

    uint32_t location;

    uint32_t cluster;
    uint32_t size;

    uint32_t sect;
    uint32_t idx;
};

struct fat32
{
    uint8_t disk;

    struct fat32br br;
    uint8_t *buffer;
    struct fat32e root;

    struct fat32e *current;
};

static uint32_t fat32_buf[0x80];
int64_t fat32_buf_n = -1;
static uint8_t fat32_buf2[0x200];

/* Helper functions */

static uint32_t
utf8_to_unicode(uint8_t **buf)
{
    uint32_t ret = 0;

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

static uint32_t
utf16_to_unicode(uint16_t **buf)
{
    uint32_t ret = (*buf)[0];

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
unicode_to_utf8(uint32_t code, char* buf)
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
unicode_to_utf16(uint32_t code, char *buf)
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
        uint16_t high = 0xD800 + (code >> 10);
        uint16_t low = 0xDC00 + (code & 0x3FF);
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

    uint8_t *buf = (uint8_t*)str;
    while (buf[0] != '\0')
    {
        uint32_t code = utf8_to_unicode(&buf);
        if (code < 0x10000)
            ret += 2;
        else
            ret += 4;
    }

    return ret;
}

static size_t
id83_to_string(uint8_t *code, char* buf)
{
    size_t ret = 0;

    char id83[12] = {0};
    char name8[9] = {0};
    char ext3[4] = {0};

    vrm_mem_copy(name8, code, 8);
    for (int k = 7; k >= 0 && name8[k] == ' '; k--)
        name8[k] = '\0';

    vrm_mem_copy(ext3, &(code[8]), 3);
    for (int k = 2; k >= 0 && ext3[k] == ' '; k--)
        ext3[k] = '\0';

    vrm_str_copy(id83, name8, 0);
    if (ext3[0] != '\0')
        vrm_str_concat(id83, ".", 0);
    vrm_str_concat(id83, ext3, 0);

    ret = vrm_str_length(id83) + 1;
    vrm_mem_copy(buf, id83, ret);

    return ret;
}

static void
string_to_id83(const char *str, uint8_t *buf)
{
    uint32_t len = vrm_str_length(str);
    if (vrm_str_length(str) <= 11)
    {
        vrm_mem_fill(buf, 0x20, 11);
        vrm_mem_copy(buf, str, len);
    }
    else
        vrm_mem_copy(buf, str, 11);
}

static uint8_t
lfn_checksum(uint8_t *buf)
{
    uint8_t ret = 0;

    for (uint8_t i = 11; i != 0; i--)
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
cluster_eof(uint32_t cluster)
{
    return (cluster & 0x0FFFFFFF) >= 0x0FFFFFF8;
}

static bool
cluster_set(struct fat32 *f, uint32_t cluster, uint32_t value)
{
    bool ret = true;

    uint32_t idx = f->br.reservedsects + (cluster / 0x80);
    for (uint32_t i = 0; ret && i < f->br.tablecount; i++)
    {
        if ((fat32_buf_n == idx) ||
            vrm_disk_read(f->disk, (uint8_t*)fat32_buf, idx, 0))
        {
            uint32_t org = fat32_buf[cluster % 0x80];
            uint32_t value2 = (org & ~CLUSTER_MASK) | (value & CLUSTER_MASK);
            fat32_buf[cluster % 0x80] = value2;

            ret = vrm_disk_write(f->disk, (uint8_t*)fat32_buf, idx, 0);
            fat32_buf_n = idx;
        }
        else
            ret = false;

        idx += f->br.sectspertable;
    }

    return ret;
}

static uint32_t
cluster_get(struct fat32 *f, uint32_t cluster)
{
    uint32_t ret = CLUSTER_EOF;

    uint32_t idx = f->br.reservedsects + (cluster / 0x80);
    if ((fat32_buf_n == idx) ||
        vrm_disk_read(f->disk, (uint8_t *)fat32_buf, idx, 0))
    {
        ret = fat32_buf[cluster % 0x80] & CLUSTER_MASK;
        fat32_buf_n = idx;
    }

    return ret;
}

static uint32_t
cluster_find(struct fat32 *f, uint32_t value)
{
    uint32_t ret = 0;

    for (uint32_t i = 2; i < 0x200 * f->br.sectspertable / 4; i++)
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

static uint32_t
fat32_next(struct fat32 *f, uint32_t cluster)
{
    uint32_t ret = 0xFFFFFFFF;

    uint32_t length = (f->br.sectsvolume) ? f->br.sectsvolume  :
                                            f->br.sectsvolume32;
    if (cluster < length)
        ret = cluster_get(f, cluster);

    return ret;
}

static uint32_t
fat32_fsector(struct fat32 *f, uint32_t cluster)
{
    uint32_t datasect = f->br.reservedsects +
                                      (f->br.tablecount * f->br.sectspertable);
    return datasect + ((cluster - 2) * f->br.sectspercluster);
}

static bool
fat32_walk(struct fat32 *f, uint32_t cluster, uint32_t sect, uint32_t idx,
           struct fat32e *out, bool *dir, char *name, uint32_t *size)
{
    bool ret = true;

    static char name2[255] = {0};
    if (!name)
        name = name2;

    static uint16_t lfn[255] = {0};
    static uint8_t lfn_last = 0;
    vrm_mem_fill(lfn, 0xFF, sizeof(lfn));

    bool found = false;
    uint32_t firstsect = 0;
    for (uint32_t i = 0x200 * f->br.sectspercluster; ret; i += 32)
    {
        if (i >= (0x200 * f->br.sectspercluster))
        {
            if (!cluster_eof(cluster))
            {
                firstsect = fat32_fsector(f, cluster);
                if (firstsect >= sect)
                {
                    for (uint8_t k = 0; ret && k < f->br.sectspercluster; k++)
                        ret = vrm_disk_read(f->disk, &(f->buffer[0x200 * k]),
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
            uint16_t idx = f->buffer[i] & 0x1F;
            if (idx > 0 && idx < 256)
            {
                idx = (idx - 1) * 13;
                vrm_mem_copy(&(lfn[idx]),      &(f->buffer[i + 1]),  5 * 2);
                vrm_mem_copy(&(lfn[idx + 5]),  &(f->buffer[i + 14]), 6 * 2);
                vrm_mem_copy(&(lfn[idx + 11]), &(f->buffer[i + 28]), 2 * 2);

                uint8_t last = idx + 13;
                if (last > lfn_last)
                    lfn_last = last;
            }
            continue;
        }

        size_t j = 0;
        for (uint16_t *lfn_p = lfn; lfn_p <= &(lfn[lfn_last]);
                                    lfn_p  = &(lfn_p[1]))
        {
            if (*lfn_p == 0xFFFF)
                continue;

            j += unicode_to_utf8(utf16_to_unicode(&lfn_p), &(name[j]));
        }
        vrm_mem_fill(lfn, 0xFF, sizeof(lfn));

        if (j > 0)
            name[j] = '\0';
        else
            id83_to_string(&(f->buffer[i]), name);

        uint8_t *entry = &(f->buffer[i]);

        out->sect = firstsect;
        out->idx  = i;
        out->location = (firstsect * 0x200) + i;
        out->attributes = entry[11];
        vrm_mem_copy(&(out->size), &(entry[28]), 4);

        uint16_t cluster_l, cluster_h;
        vrm_mem_copy(&cluster_l, &(entry[26]), 2);
        vrm_mem_copy(&cluster_h, &(entry[20]), 2);
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

static uint32_t
fat32_alloc(struct fat32 *f)
{
    uint32_t ret = cluster_find(f, CLUSTER_FREE);

    if (ret)
        if (!cluster_set(f, ret, CLUSTER_EOF))
            ret = CLUSTER_EOF;

    return ret;
}

static uint32_t
fat32_alloc2(struct fat32 *f, uint32_t cluster)
{
    uint32_t ret = fat32_next(f, cluster);

    if (cluster_eof(ret))
    {
        uint32_t cluster2 = cluster_find(f, CLUSTER_FREE);
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
fat32_free(struct fat32 *f, uint32_t cluster, uint32_t depth)
{
    bool ret = true;

    for (uint32_t i = 0; ret && !cluster_eof(cluster); i++)
    {
        uint32_t prev = cluster;
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
        vrm_mem_del(f->buffer);
        vrm_mem_del(f);
    }

    return NULL;
}

static struct fat32 *
fat32_new(uint8_t disk)
{
    struct fat32 *ret = vrm_mem_new(sizeof(struct fat32));

    if (ret)
    {
        vrm_mem_fill(ret, 0, sizeof(struct fat32));

        ret->disk = disk;
        if (vrm_disk_read(ret->disk, (uint8_t*)fat32_buf, 0, 0))
            vrm_mem_copy(&(ret->br), fat32_buf, sizeof(struct fat32br));
        else
            ret = fat32_del(ret);
    }

    if (ret)
    {
        ret->buffer = vrm_mem_new(0x200 * ret->br.sectspercluster);
        if (ret->buffer)
            vrm_mem_fill(ret->buffer, 0, 0x200 * ret->br.sectspercluster);
        else
            ret = fat32_del(ret);
    }

    if (ret)
    {
        if (vrm_disk_read(ret->disk, (uint8_t *)fat32_buf,
                          ret->br.fsinfosect, 0))
        {
            /* Invalidate free cluster cache, it will not be used */
            vrm_mem_fill(&(fat32_buf[0x7A]), 0xFF, 8);
            if (!vrm_disk_write(ret->disk, (uint8_t *)fat32_buf,
                                ret->br.fsinfosect, 0))
                ret = fat32_del(ret);
        }
        else
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
fat32_rw(struct fat32 *f, struct fat32e *fe,
         uint32_t sector, uint8_t *data, bool write)
{
    bool ret = !(fe->attributes & 0x10);

    if (ret)
    {
        uint32_t cluster_n = sector / f->br.sectspercluster;
        uint32_t sector_n = sector % f->br.sectspercluster;

        uint32_t cluster = fe->cluster;
        for (uint32_t i = 0; i < cluster_n && !cluster_eof(cluster); i++)
            cluster = fat32_next(f, cluster);

        if (!cluster_eof(cluster))
        {
            cluster &= 0xFFFFFFF;
            uint32_t firstsect = fat32_fsector(f, cluster);
            ret = ((write) ? vrm_disk_write : vrm_disk_read)
                  (f->disk, data, firstsect + sector_n, 0);
        }
    }

    return ret;
}

static bool
entry_update(struct fat32 *f, struct fat32e *fe, bool unused)
{
    bool ret = true;

    uint32_t sector = fe->location / 0x200;
    uint32_t index = fe->location % 0x200;
    ret = vrm_disk_read(f->disk, f->buffer, sector, 0);

    if (ret)
    {
        if (unused)
        {
            for (int64_t i = index; true; i -= 32)
            {
                if (i != index && f->buffer[i + 11] != 0x0F)
                    break;

                vrm_mem_fill(&(f->buffer[i + 0]), 0xE5, 1);
                vrm_mem_fill(&(f->buffer[i + 1]), 0x00, 31);

                if (i == 0)
                {
                    ret = vrm_disk_write(f->disk, f->buffer, sector--, 0) &&
                          vrm_disk_read (f->disk, f->buffer, sector,   0);
                    i = 0x200;
                }
            }
        }
        else
        {
            uint32_t i = index;
            vrm_mem_copy(&(f->buffer[i + 28]), &(fe->size), sizeof(uint32_t));
            f->buffer[i + 26] = (fe->cluster >> 0)  & 0xFF;
            f->buffer[i + 27] = (fe->cluster >> 8)  & 0xFF;
            f->buffer[i + 20] = (fe->cluster >> 16) & 0xFF;
            f->buffer[i + 21] = (fe->cluster >> 24) & 0xFF;
        }

        if (ret)
            ret = vrm_disk_write(f->disk, f->buffer, sector, 0);
    }

    return ret;
}

static bool
fat32_resize(struct fat32 *f, struct fat32e *fe, uint32_t size)
{
    bool ret = true;

    uint32_t cluster_n = 0;
    if (fe->cluster && !cluster_eof(fe->cluster))
    {
        for (uint32_t c = fe->cluster; !cluster_eof(c); c = fat32_next(f, c))
            cluster_n++;
    }
    else if (size)
    {
        fe->cluster = fat32_alloc(f);
        cluster_n   = 1;
        ret = (!cluster_eof(fe->cluster));
    }

    if (ret)
    {
        uint32_t cluster_n2 = (size / 0x200) / f->br.sectspercluster;
        if (cluster_n != cluster_n2)
        {
            if (cluster_n2 < cluster_n)
            {
                ret = fat32_free(f, fe->cluster, cluster_n2 + 1);
                fe->size = size;
            }
            else if (cluster_n2 > cluster_n)
            {
                uint32_t cluster = fe->cluster;
                for (uint32_t i = 0; ret && i <= cluster_n2; i++)
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
    }

    if (ret)
        ret = entry_update(f, fe, false);

    return ret;
}

static void
fill_entry(uint8_t *buf, const char *name,
           bool dir, uint8_t entries,
           uint32_t cluster, uint32_t size)
{
    vrm_mem_fill(buf, 0x0, 0x200);

    uint8_t id83[11] = {0};
    string_to_id83(name, id83);

    uint8_t *name2 = (uint8_t*)name;
    uint8_t lfn_c = entries - 1;
    uint8_t checksum = lfn_checksum(id83);
    for (uint32_t n = 0; n < lfn_c; n++)
    {
        uint32_t i = ((lfn_c - n - 1) * 32);

        buf[i] = n + 1 + ((n == (uint32_t)(lfn_c - 1)) ? 0x40 : 0x0);
        buf[i + 11] = 0xF;
        buf[i + 13] = checksum;

        bool last = true;
        uint8_t start[] = {1, 14, 28}, end[] = {11, 26, 32};
        for (uint8_t j = 0; j < 3; j++)
        {
            uint8_t *cur = &(buf[i + start[j]]);
            while (cur < &(buf[i + end[j]]))
            {
                if (name2[0])
                {
                    uint32_t code = utf8_to_unicode(&name2);
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

    uint32_t i = lfn_c * 32;
    vrm_mem_copy(&(buf[i]), id83, 11);
    buf[i + 11] = 0x20 | ((dir) ? 0x10 : 0x0);
    buf[i + 26] = (cluster >> 0)  & 0xFF;
    buf[i + 27] = (cluster >> 8)  & 0xFF;
    buf[i + 20] = (cluster >> 16) & 0xFF;
    buf[i + 21] = (cluster >> 24) & 0xFF;

    /* Unused date/time slots */
    buf[i + 16] = 0x21;
    buf[i + 18] = 0x21;
    buf[i + 24] = 0x21;

    /* Filesize in case it's being moved */
    vrm_mem_copy(&(buf[i + 28]), &size, sizeof(uint32_t));
}

static bool
chain_write(struct fat32 *f, uint8_t *buf, uint32_t cluster,
            uint32_t sector_idx, uint32_t pos, uint32_t bytes)
{
    bool ret = true;

    uint32_t idx = 0;
    while (ret && bytes)
    {
        uint32_t sector = fat32_fsector(f, cluster) + sector_idx;
        ret = vrm_disk_read(f->disk, f->buffer, sector, 0);

        if (ret)
        {
            uint32_t write = (bytes < (0x200 - pos)) ? bytes : (0x200 - pos);
            if (pos + bytes <= 0x200)
            {
                vrm_mem_copy(&(f->buffer[pos]), &(buf[idx]), write);
                idx += write;
                bytes -= write;
            }
            else
                vrm_mem_fill(&(f->buffer[pos]), 0xE5, write);
            ret = vrm_disk_write(f->disk, f->buffer, sector, 0);
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
directory_dots(struct fat32 *f, uint32_t cluster, uint32_t fpcluster)
{
    bool ret = false;

    uint32_t sector = fat32_fsector(f, cluster);
    ret = vrm_disk_read(f->disk, f->buffer, sector, 0);
    if (ret)
    {
        uint8_t *buf = f->buffer;
        vrm_mem_fill(buf, 0x0, 0x200 * f->br.sectspercluster);

        uint32_t i = 0;
        vrm_mem_fill(&(buf[i]), 0x20, 11);
        buf[i +  0] = '.';
        buf[i + 11] = 0x30;
        buf[i + 26] = (cluster >> 0)  & 0xFF;
        buf[i + 27] = (cluster >> 8)  & 0xFF;
        buf[i + 20] = (cluster >> 16) & 0xFF;
        buf[i + 21] = (cluster >> 24) & 0xFF;

        if (fpcluster == 2)
            fpcluster =  0;

        i += 32;
        vrm_mem_fill(&(buf[i]), 0x20, 11);
        buf[i +  0] = '.';
        buf[i +  1] = '.';
        buf[i + 11] = 0x10;
        buf[i + 26] = (fpcluster >> 0)  & 0xFF;
        buf[i + 27] = (fpcluster >> 8)  & 0xFF;
        buf[i + 20] = (fpcluster >> 16) & 0xFF;
        buf[i + 21] = (fpcluster >> 24) & 0xFF;

        ret = vrm_disk_write(f->disk, f->buffer, sector, 0);
    }

    return ret;
}

static bool
fat32_create(struct fat32 *f, uint32_t pcluster,
             bool dir, const char *name,
             uint32_t cluster, uint32_t size)
{
    bool ret = (f && name);

    uint32_t entries = 0;
    if (ret)
    {
        uint32_t length = utf8to16_length(name);
        if (length <= 255)
            entries = (length / 13) + 2;
        else
            ret = false;
    }

    if (ret)
    {
        uint16_t pos = 0;
        uint32_t fpcluster = pcluster;
        uint32_t sector    = fat32_fsector(f, pcluster), sector_idx = 0;
        uint16_t unused_st = 0, unused_c = 0;

        bool found = false;
        while (!found)
        {
            if (vrm_disk_read(f->disk, f->buffer, sector, 0))
            {
                for (uint32_t i = 0; i < 0x200; i += 32)
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
            if (ret && cluster == 0 && dir)
            {
                cluster = fat32_alloc(f);
                if (cluster)
                    ret = directory_dots(f, cluster, fpcluster);
                else
                    ret = false;
            }

            if (ret)
            {
                fill_entry(fat32_buf2, name, dir, entries, cluster, size);
                fat32_buf_n = -1;

                uint32_t bytes = (entries * 32) + ((unused_st != pos) ? 32 : 0);
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
     bool *dir, char *name, uint32_t *size)
{
    void *ret = NULL;

    struct fat32e *fd = parent;
    if (ctx && parent && (fd->attributes & 0x10))
    {
        struct fat32e *fe = entry;

        uint32_t sect = 0, idx = 0;
        if (fe)
        {
            sect = fe->sect;
            idx  = fe->idx;
        }

        struct fat32e *new = vrm_mem_new(sizeof(struct fat32e));
        if (new)
        {
            vrm_mem_fill(new, 0, sizeof(struct fat32e));
            if (fat32_walk(ctx, fd->cluster, sect, idx, new, dir, name, size))
                ret = new;
            else
                vrm_mem_del(new);
        }

        vrm_mem_del(entry);
    }

    return ret;
}

static bool
read(void *ctx, void *entry, uint8_t *data, uint32_t block)
{
    return fat32_rw(ctx, entry, block, data, false);
}

static bool
write(void *ctx, void *entry, const uint8_t *data, uint32_t block)
{
    return fat32_rw(ctx, entry, block, (uint8_t *)data, true);
}

static bool
create(void *ctx, void *parent, const char *name, bool dir)
{
    bool ret = (ctx && parent && name);

    struct fat32e *fe = parent;
    if (ret)
        ret = fat32_create(ctx, fe->cluster, dir, name, 0, 0);

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

static bool
resize(void *ctx, void *entry, uint32_t size)
{
    bool ret = (ctx && entry);

    if (ret)
        ret = fat32_resize(ctx, entry, size);

    return ret;
}

static bool
move(void *ctx, void *entry, void *parent, const char *name)
{
    bool ret = (ctx && entry && parent && name);

    struct fat32 *f = ctx;
    if (ret)
        ret = (entry != &(f->root));

    bool dir = false;
    uint32_t cluster = 0, size = 0;
    if (ret)
    {
        struct fat32e *fe = entry;
        dir     = fe->attributes & 0x10;
        cluster = fe->cluster;
        size    = fe->size;
        ret = fat32_remove(ctx, entry, false);
    }

    if (ret)
    {
        struct fat32e *fe = parent;
        ret = fat32_create(ctx, fe->cluster, dir, name, cluster, size);
    }

    return ret;
}

static const drv_fs fat32 =
{
    .root = root, .walk  = walk,
    .read = read, .write = write,
    .create = create, .remove = remove,
    .resize = resize, .move   = move
};

/* Device creation */

extern dev_fs
fat32_init(uint8_t disk)
{
    void *ret = NULL;

    uint16_t sector = 0;
    if (vrm_disk_size(disk, &sector, NULL) && sector == 0x200)
        ret = fat32_new(disk);

    return (dev_fs){.driver = &fat32, .context = ret};
}

extern void
fat32_clean(dev_fs *f)
{
    if (f)
        f->context = fat32_del(f->context);
}
