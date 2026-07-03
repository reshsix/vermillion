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

struct fat32
{
    uint8_t disk;
    struct fat32br br;

    uint8_t  cache[0x200];
    uint32_t cache_blk;
    bool     cached;
};

/* Helper macros */

#define DIV_ROUND(x, y) (((x) + (y) - 1) / (y))

/* Disk functions */

static bool
disk_flush(struct fat32 *f)
{
    bool ret = true;

    if (f->cached)
    {
        ret = vrm_disk_write(f->disk, f->cache, f->cache_blk, 0);
        if (ret)
            f->cached = false;
    }

    return ret;
}

static bool
disk_cache(struct fat32 *f, uint32_t block)
{
    bool ret = (f->cached && f->cache_blk == block);

    if (!ret)
    {
        ret = (f->cached) ? disk_flush(f) : true;
        if (ret)
            ret = vrm_disk_read(f->disk, f->cache, block, 0);

        if (ret)
        {
            f->cache_blk = block;
            f->cached    = true;
        }
    }

    return ret;
}

/* Cluster functions */

enum
{
    CLUSTER_FREE = 0x0,
    CLUSTER_EOF  = 0x0FFFFFFF,
    CLUSTER_MASK = 0x0FFFFFFF
};

static bool
cluster_eof(uint32_t cluster)
{
    return (cluster & CLUSTER_MASK) >= 0x0FFFFFF8;
}

static bool
cluster_set(struct fat32 *f, uint32_t cluster, uint32_t value)
{
    bool ret = true;

    uint32_t *table = f->cache;

    uint32_t idx = f->br.reservedsects + (cluster / 0x80);
    for (uint32_t i = 0; ret && i < f->br.tablecount; i++)
    {
        if (disk_cache(f, idx))
        {
            uint32_t org    = table[cluster % 0x80];
            uint32_t value2 = (org & ~CLUSTER_MASK) | (value & CLUSTER_MASK);
            table[cluster % 0x80] = value2;
        }
        else
            ret = false;

        idx += f->br.sectspertable;
    }

    return ret && disk_flush(f);
}

static uint32_t
cluster_get(struct fat32 *f, uint32_t cluster)
{
    uint32_t ret = CLUSTER_EOF;

    uint32_t *table = f->cache;

    uint32_t idx = f->br.reservedsects + (cluster / 0x80);
    if (disk_cache(f, idx))
        ret = table[cluster % 0x80] & CLUSTER_MASK;

    return ret;
}

static uint32_t
cluster_find(struct fat32 *f, uint32_t value)
{
    uint32_t ret = CLUSTER_EOF;

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

static uint32_t
cluster_next(struct fat32 *f, uint32_t cluster)
{
    uint32_t ret = CLUSTER_EOF;

    uint32_t length = (f->br.sectsvolume) ? f->br.sectsvolume  :
                                            f->br.sectsvolume32;
    if (cluster < length)
        ret = cluster_get(f, cluster);

    return ret;
}

static uint32_t
cluster_alloc(struct fat32 *f, uint32_t cluster, uint32_t count)
{
    uint32_t ret = CLUSTER_EOF;

    bool     chain = (cluster);
    uint32_t first =  cluster;
    for (uint32_t i = 0; i < count; i++)
    {
        if (cluster)
        {
            ret = cluster_next(f, cluster);
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
        }
        else
        {
            ret = cluster_find(f, CLUSTER_FREE);
            if (ret)
            {
                if (cluster_set(f, ret, CLUSTER_EOF))
                {
                    cluster = ret;
                    first   = ret;
                }
                else
                    ret = CLUSTER_EOF;
            }
        }
    }

    if (chain)
        first = cluster_next(f, cluster);

    return (cluster_eof(ret)) ? ret : first;
}

static bool
cluster_free(struct fat32 *f, uint32_t cluster, uint32_t depth)
{
    bool ret = true;

    for (uint32_t i = 0; ret && !cluster_eof(cluster); i++)
    {
        uint32_t prev = cluster;
        cluster = cluster_next(f, cluster);

        if ((i + 1) == depth)
            ret = cluster_set(f, prev, CLUSTER_EOF);
        else if ((i + 1) > depth)
            ret = cluster_set(f, prev, CLUSTER_FREE);
    }

    return ret;
}

static uint32_t
cluster_block(struct fat32 *f, uint32_t cluster)
{
    return f->br.reservedsects + (f->br.tablecount * f->br.sectspertable) +
                                 ((cluster - 2)    * f->br.sectspercluster);
}

/* Sector functions */

static bool
sector_cache(struct fat32 *f, uint32_t fcluster, uint32_t idx)
{
    bool ret = false;

    uint32_t c = idx / f->br.sectspercluster;
    uint32_t s = idx % f->br.sectspercluster;

    uint32_t cluster = fcluster;
    for (uint32_t i = 0; i < c; i++)
        cluster = cluster_next(f, cluster);

    if (!cluster_eof(cluster))
        ret = disk_cache(f, cluster_block(f, cluster) + s);

    return ret;
}

/* Entry functions */

static uint8_t *
entry_cache(struct fat32 *f, uint32_t fcluster, uint32_t idx)
{
    uint8_t *ret = NULL;

    uint32_t s = idx / (0x200 / 32);
    uint32_t e = idx % (0x200 / 32);

    if (sector_cache(f, fcluster, s))
       ret = &(f->cache[e * 32]);

    return ret;
}

static int64_t
entry_alloc(struct fat32 *f, uint32_t fcluster, uint8_t count)
{
    int64_t ret = -1;

    bool     alloc = false;
    uint32_t found = 0;

    uint32_t i = 0;
    while (true)
    {
        uint8_t *entry = entry_cache(f, fcluster, i);
        if (!entry)
            break;

        if (entry[0] == 0x00)
        {
            alloc  = true;
            ret    = i;
            break;
        }

        if (entry[0] == 0xE5)
        {
            if (found == 0)
                ret = i;

            found++;
            if (found >= count)
                break;
        }
        else
        {
            found =  0;
            ret   = -1;
        }

        i++;
    }

    if (alloc)
    {
        uint32_t per_sector  = 0x200 / 32;
        uint32_t per_cluster = per_sector * f->br.sectspercluster;

        uint32_t left = per_cluster - (i % per_cluster);
        if (left < count)
        {
            uint32_t clusters = DIV_ROUND(count, per_cluster);
            if (cluster_eof(cluster_alloc(f, fcluster, clusters)))
                ret = -1;
        }

        if (ret > 0)
        {
            uint8_t *last = entry_cache(f, fcluster, ret + count);
            last[0] = 0x0;
        }
    }

    return ret;
}

static void
entry_metadata(uint8_t *entry, bool *dir, uint32_t *size, uint32_t *location)
{
    if (dir)
        *dir = (entry[11] & 0x10);
    if (size)
        vrm_mem_copy(size, &(entry[28]), 4);
    if (location)
    {
        uint16_t cluster_l, cluster_h;
        vrm_mem_copy(&cluster_l, &(entry[26]), 2);
        vrm_mem_copy(&cluster_h, &(entry[20]), 2);
        *location = cluster_l | (cluster_h << 16);
    }
}

/* Encode/decode functions */

static uint32_t
utf8_to_unicode(const char **buf)
{
    uint32_t ret = 0;

    if ((*buf)[0] < 0x80)
    {
        ret  =   (*buf)[0];
        *buf = &((*buf)[1]);
    }
    else if ((*buf)[0] >= 0xC0)
    {
        if ((*buf)[0] < 0xE0)
        {
            ret  =  ((*buf)[0] & ~0xF0) | ((*buf)[1] & ~0xC0);
            *buf = &((*buf)[2]);
        }
        else if ((*buf)[0] < 0xF0)
        {
            ret  =  ((*buf)[0] & ~0xF0) | ((*buf)[1] & ~0xC0) |
                    ((*buf)[2] & ~0xC0);
            *buf = &((*buf)[3]);
        }
        else
        {
            ret   = ((*buf)[0] & ~0xF0) | ((*buf)[1] & ~0xC0) |
                    ((*buf)[2] & ~0xC0) | ((*buf)[3] & ~0xC0);
            *buf = &((*buf)[4]);
        }
    }

    return ret;
}

static uint32_t
utf16_to_unicode(const uint16_t **buf)
{
    uint32_t ret = (*buf)[0];

    if ((ret >> 10) == 0x36)
    {
        ret  = (ret & 0x3ff) << 10;

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
unicode_to_utf8(uint32_t code, char *buf)
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
        buf[ret++] = 0x80 | ((code >>  6) & 0x3F);
        buf[ret++] = 0x80 | (code & 0x3F);
    }
    else
    {
        buf[ret++] = 0xF0 | ((code >> 18) & 0x07);
        buf[ret++] = 0x80 | ((code >> 12) & 0x3F);
        buf[ret++] = 0x80 | ((code >>  6) & 0x3F);
        buf[ret++] = 0x80 | (code & 0x3F);
    }

    return ret;
}

static size_t
unicode_to_utf16(uint32_t code, uint16_t *buf)
{
    size_t ret = 0;

    if (code < 0x10000)
        buf[ret++] = code;
    else
    {
        code -= 0x10000;
        buf[ret++] = 0xD800 + (code >> 10);
        buf[ret++] = 0xDC00 + (code & 0x3FF);
    }

    return ret;
}

static size_t
utf8to16_length(const char *str)
{
    size_t ret = 0;

    while (str[0] != '\0')
    {
        uint32_t code = utf8_to_unicode(&str);
        if (code < 0x10000)
            ret += 2;
        else
            ret += 4;
    }

    return ret;
}

static size_t
id83_to_utf8(uint8_t *code, char *buf)
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
entry_to_id83(uint32_t entry, char *buf)
{
    vrm_str_copy(buf, "LFN_0000   ", 12);
    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t n = (entry >> ((3 - i) * 8)) & 0xF;
        if (n < 0xA)
            buf[4 + i] = '0' + n;
        else
            buf[4 + i] = 'A' + n - 0xA;
    }
}

/* LFN functions */

static uint8_t
lfn_entries(const char *name)
{
    uint8_t ret = 0;

    uint32_t length = utf8to16_length(name);
    if (length <= 255)
        ret = (length + 25) / 26;

    return ret;
}

static uint8_t
lfn_checksum(const char *buf)
{
    uint8_t ret = 0;

    for (uint8_t i = 11; i != 0; i--)
        ret = ((ret & 1) << 7) + (ret >> 1) + buf[11 - i];

    return ret;
}

static void
lfn_read(const uint8_t *entry, uint16_t *buf)
{
    uint16_t idx = entry[0] & 0x1F;
    if (idx && idx < 20)
    {
        idx = (idx - 1) * 13;
        vrm_mem_copy(&(buf[idx + 0]),  &(entry[1]),  5 * 2);
        vrm_mem_copy(&(buf[idx + 5]),  &(entry[14]), 6 * 2);
        vrm_mem_copy(&(buf[idx + 11]), &(entry[28]), 2 * 2);
    }
}

static void
lfn_write(const uint16_t *buf, uint8_t *entry,
          uint8_t checksum, uint8_t n, bool last)
{
    entry[0]  = n + 1 + ((last) ? 0x40 : 0x0);
    entry[11] = 0xF;
    entry[13] = checksum;

    vrm_mem_copy(&(entry[1]),  &(buf[0]),  5 * 2);
    vrm_mem_copy(&(entry[14]), &(buf[5]),  6 * 2);
    vrm_mem_copy(&(entry[28]), &(buf[11]), 2 * 2);
}

static void
lfn_decode(const uint16_t *lfn, char *buf)
{
    size_t i = 0;
    for (; i < 255 && lfn[0]; lfn = &(lfn[1]))
        i += unicode_to_utf8(utf16_to_unicode(&lfn), &(buf[i]));
    buf[i] = '\0';
}

/* Driver definition */

static uint32_t
root(void *ctx)
{
    (void)ctx;
    return 2;
}

static bool
walk(void *ctx, uint32_t parent, uint32_t *idx,
     bool *dir, char *name, uint32_t *size, uint32_t *location)
{
    bool ret = false;

    struct fat32 *f = ctx;

    static char name2[256] = {0};
    if (!name)
        name = name2;

    uint16_t lfn[260] = {0};
    for (uint32_t i = *idx; true; i++)
    {
        uint8_t *entry = entry_cache(f, parent, i);
        if (!entry)
            break;

        if (entry[0]  == 0x00)
            break;
        if (entry[0]  == 0x05 || entry[0] == 0xE5)
            continue;
        if (entry[11] == 0x0F)
        {
            lfn_read(entry, lfn);
            continue;
        }
        lfn_decode(lfn, name);

        if (!name[0])
            id83_to_utf8(entry, name);

        entry_metadata(entry, dir, size, location);
        *idx = i + 1;
        ret  = true;
        break;
    }

    return ret;
}

static bool
read(void *ctx, uint32_t location, uint8_t *data, uint32_t block)
{
    bool ret = sector_cache(ctx, location, block);

    struct fat32 *f = ctx;
    if (ret)
        vrm_mem_copy(data, f->cache, 0x200);

    return ret;
}

static bool
write(void *ctx, uint32_t location, const uint8_t *data, uint32_t block)
{
    bool ret = sector_cache(ctx, location, block);

    struct fat32 *f = ctx;
    if (ret)
        vrm_mem_copy(f->cache, data, 0x200);

    return ret && disk_flush(f);
}

static bool
create(void *ctx, uint32_t parent, const char *name, bool dir,
       uint32_t location, uint32_t size)
{
    bool ret = true;

    struct fat32 *f = ctx;

    uint8_t entries = lfn_entries(name);
    int64_t idx     = -1;
    if (entries)
        idx = entry_alloc(f, parent, entries + 1);

    if (idx > 0)
    {
        if (location == 0 && dir)
        {
            location = cluster_alloc(f, 0, 1);
            if (location)
            {
                for (uint8_t i = 0; i < f->br.sectspercluster; i++)
                {
                    ret = sector_cache(f, location, i);
                    if (ret)
                        vrm_mem_fill(f->cache, 0x0, 0x200);
                    else
                        break;
                }
            }
            else
                ret = false;

            if (ret)
            {
                uint8_t *dot = entry_cache(f, location, 0);
                vrm_mem_fill(dot, 0x20, 11);
                dot[ 0] = '.';
                dot[11] = 0x10;
                dot[26] = (location >> 0)  & 0xFF;
                dot[27] = (location >> 8)  & 0xFF;
                dot[20] = (location >> 16) & 0xFF;
                dot[21] = (location >> 24) & 0xFF;

                uint32_t fpcluster = (parent == 2) ? 0 : parent;

                dot = entry_cache(f, location, 1);
                vrm_mem_fill(dot, 0x20, 11);
                dot[ 0] = '.';
                dot[ 1] = '.';
                dot[11] = 0x10;
                dot[26] = (fpcluster >> 0)  & 0xFF;
                dot[27] = (fpcluster >> 8)  & 0xFF;
                dot[20] = (fpcluster >> 16) & 0xFF;
                dot[21] = (fpcluster >> 24) & 0xFF;

                dot = entry_cache(f, location, 2);
                dot[0] = 0x00;
            }
        }
    }
    else
        ret = false;

    if (ret)
    {
        const char *ptr = name;
        uint16_t name2[260] = {0};

        bool end = false;
        for (size_t i = 0; i < 260;)
        {
            if (ptr[0])
                i += unicode_to_utf16(utf8_to_unicode(&ptr), &(name2[i]));
            else
            {
                name2[i++] = (end) ? 0xFFFF : 0x0000;
                end = true;
            }
        }

        char id83[12] = {0};
        entry_to_id83(idx + entries, id83);

        uint8_t checksum = lfn_checksum(id83);
        for (uint8_t i = 0; i < entries; i++)
        {
            uint8_t *lfn = entry_cache(f, parent, idx + i);
            vrm_mem_fill(lfn, 0x0, 32);
            lfn_write(&(name2[13 * i]), lfn,
                      checksum, i, ((i + 1) == entries));
        }

        uint8_t *sfn = entry_cache(f, parent, idx + entries);
        vrm_mem_fill(sfn, 0x00, 32);
        vrm_mem_copy(sfn, id83, 11);

        sfn[11] = 0x20 | ((dir) ? 0x10 : 0x0);
        sfn[26] = (location >> 0)  & 0xFF;
        sfn[27] = (location >> 8)  & 0xFF;
        sfn[20] = (location >> 16) & 0xFF;
        sfn[21] = (location >> 24) & 0xFF;

        /* Unused date/time slots */
        sfn[16] = 0x21;
        sfn[18] = 0x21;
        sfn[24] = 0x21;

        /* Filesize in case it's being moved */
        vrm_mem_copy(&(sfn[28]), &size, sizeof(uint32_t));
    }

    return ret && disk_flush(f);
}

static bool
remove(void *ctx, uint32_t parent, uint32_t idx, bool data)
{
    bool ret = false;

    struct fat32 *f = ctx;
    if (parent >= 2)
    {
        uint8_t *entry = NULL;
        for (uint8_t i = 0; true; i++)
        {
            entry = entry_cache(f, parent, idx + i);
            if (!entry)
                break;

            entry[0] = 0xE5;
            if (entry[11] != 0x0F)
            {
                ret = true;
                break;
            }
        }

        if (ret && data)
        {
            if (entry)
            {
                bool dir = false;
                uint32_t cluster = 0;
                entry_metadata(entry, &dir, NULL, &cluster);

                /* Checks for empty directory */
                if (dir)
                {
                    entry = entry_cache(f, cluster, 2);
                    if (entry[0] != 0x0)
                        ret = false;
                }

                if (ret)
                    ret = cluster_free(f, cluster, 0);
            }
            else
                ret = false;
        }
    }

    return ret && disk_flush(f);
}

static bool
resize(void *ctx, uint32_t parent, uint32_t idx, uint32_t size,
       uint32_t *location)
{
    bool ret = false;

    struct fat32 *f = ctx;

    uint8_t *entry = NULL;
    while (true)
    {
        entry = entry_cache(f, parent, idx);
        if (!entry)
            break;

        if (entry[11] != 0x0F)
        {
            ret = true;
            break;
        }

        idx++;
    }

    uint32_t cluster = 0;
    if (ret)
    {
        bool dir = false;
        uint32_t fsize = 0;
        entry_metadata(entry, &dir, &fsize, &cluster);

        if (!dir)
        {
            uint32_t per_sector  = 0x200 / 32;
            uint32_t per_cluster = per_sector * f->br.sectspercluster;
            uint32_t clusters    = DIV_ROUND(fsize, per_cluster);
            uint32_t clusters2   = DIV_ROUND(size , per_cluster);
            if (!cluster)
            {
                cluster = cluster_alloc(f, cluster, clusters2);
                ret     = !cluster_eof(cluster);
            }
            else if (clusters != clusters2)
            {
                if (clusters2 > clusters)
                {
                    cluster = cluster_alloc(f, cluster, clusters2 - clusters);
                    ret     = !cluster_eof(cluster);
                }
                else
                    ret = cluster_free(f, cluster, clusters2);
            }
        }
        else
            ret = false;
    }

    if (ret)
    {
        entry = entry_cache(f, parent, idx);
        if (entry)
        {
            vrm_mem_copy(&(entry[28]), &size, 4);

            uint16_t cluster_l = cluster &  0xFFFF;
            uint16_t cluster_h = cluster >> 16;
            vrm_mem_copy(&(entry[26]), &cluster_l, 2);
            vrm_mem_copy(&(entry[20]), &cluster_h, 2);

            if (location)
                *location = cluster;
        }
        else
            ret = false;
    }

    return ret && disk_flush(f);
}

static const drv_fs fat32 =
{
    .root = root, .walk  = walk,
    .read = read, .write = write,
    .create = create, .remove = remove, .resize = resize,
};

/* Device creation */

extern dev_fs
fat32_init(uint8_t disk)
{
    struct fat32 *ret = NULL;

    uint16_t sector = 0;
    if (vrm_disk_size(disk, &sector, NULL) && sector == 0x200)
    {
        ret = vrm_mem_new(sizeof(struct fat32));

        if (ret)
        {
            vrm_mem_fill(ret, 0, sizeof(struct fat32));

            ret->disk = disk;
            if (disk_cache(ret, 0))
                vrm_mem_copy(&(ret->br), ret->cache, sizeof(struct fat32br));
            else
                ret = vrm_mem_del(ret);
        }

        if (ret)
        {
            if (disk_cache(ret, ret->br.fsinfosect))
                vrm_mem_fill(&(ret->cache[0x1E8]), 0xFF, 8);
            else
                ret = vrm_mem_del(ret);
        }
    }

    return (dev_fs){.driver = &fat32, .context = ret};
}

extern void
fat32_clean(dev_fs *f)
{
    if (f)
    {
        struct fat32 *ff = f->context;
        disk_flush(ff);
        f->context = vrm_mem_del(f);
    }
}
