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

#include <vermillion/types.h>
#include <vermillion/utils.h>
#include <vermillion/drivers.h>

struct fat32br
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
} __attribute__((packed));

struct fat32e
{
    char *name;
    u8 attributes;

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
    struct device *storage;

    struct fat32br br;
    u8 *buffer;
    u32 *table;
    struct fat32e root;
};

static u8 fat32_buf[0x200] __attribute__((aligned(32)));

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
fat32e_init(struct fat32e *fe, char *name, u8 *entry)
{
    bool ret = true;

    fe->name = mem_new(str_length(name) + 1);
    if (!(fe->name))
        ret = fat32e_clean(fe);

    if (ret)
    {
        str_copy(fe->name, name, 0);

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

/* Parsing functions */

static u32
fat32_next(struct fat32 *f, u32 cluster)
{
    u32 ret = 0xFFFFFFFF;

    u32 length = (f->br.sectsvolume) ? f->br.sectsvolume : f->br.sectsvolume32;
    if (cluster < length)
        mem_copy(&ret, &(f->table[cluster]), 4);

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

    cluster &= 0xFFFFFFF;
    u32 firstsect = fat32_fsector(f, cluster);
    for (u8 i = 0; ret && i < f->br.sectspercluster; i++)
        ret = BLOCK_R(*(f->storage), 0, &(f->buffer[0x200 * i]), firstsect + i);

    bool finished = false;
    static char name[(255 * 4) + 1] = {0};
    static u16 lfn[(255 * 4) + 1] = {0};
    u16 *lfn_p = lfn;
    for (size_t i = 0; ret && i < (0x200 * f->br.sectspercluster); i += 32)
    {
        if (f->buffer[i] == 0x00)
        {
            finished = true;
            break;
        }

        if (f->buffer[i] == 0x05 || f->buffer[i] == 0xE5)
            continue;

        if (f->buffer[i + 11] == 0xF)
        {
            mem_copy(lfn_p, &(f->buffer[i + 1]), 10);
            lfn_p = &(lfn_p[5]);
            mem_copy(lfn_p, &(f->buffer[i + 14]), 12);
            lfn_p = &(lfn_p[6]);
            mem_copy(lfn_p, &(f->buffer[i + 28]), 4);
            lfn_p = &(lfn_p[2]);
            continue;
        }

        size_t j = 0;
        for (u16 *lfn16 = lfn; lfn16 < lfn_p; lfn16 = &(lfn16[1]))
        {
            if (lfn16[0] == 0xFFFF)
                continue;

            j += unicode_to_utf8(utf16_to_unicode(&lfn16), &(name[j]));
        }
        lfn_p = lfn;

        j += id83_to_string(&(f->buffer[i]), &(name[j]));
        if (str_comp(name, ".", 0) != 0 && str_comp(name, "..", 0) != 0)
        {
            if (out->files.count >= out->files.length)
            {
                void *new = mem_renew(out->files.data,
                                      out->files.length * 2);
                if (new != NULL)
                {
                    out->files.data = new;
                    mem_init(&(out->files.data[out->files.length]), 0,
                             out->files.length);
                    out->files.length *= 2;
                }
                else
                    ret = false;
            }
            struct fat32e *fe = &(out->files.data[out->files.count++]);
            fat32e_init(fe, name, &(f->buffer[i]));
        }
    }

    if (ret && !finished)
    {
        u32 next = fat32_next(f, cluster);
        if (next < 0xFFFFFFF8)
            ret = fat32_directory(f, next, out);
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
        mem_del(f->buffer);
        mem_del(f);
    }

    return NULL;
}

static struct fat32 *
fat32_new(struct device *storage)
{
    struct fat32 *ret = mem_new(sizeof(struct fat32));

    if (ret)
    {
        ret->storage = storage;

        if (!BLOCK_R(*(ret->storage), 0, fat32_buf, 0))
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
            success = BLOCK_R(*(ret->storage), 0,
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
        if (!(fat32_transverse(ret, ret->br.rootcluster, &(ret->root))))
            ret = fat32_del(ret);
    }

    return ret;
}

static struct fat32e *
fat32_find(struct fat32 *f, char *path)
{
    bool found = true;
    char *state = NULL;
    struct fat32e *cur = &(f->root);
    for (char *token = str_token(path, "/", &state); token;
               token = str_token(NULL, "/", &state))
    {
        found = false;
        for (size_t i = 0; i < cur->files.count; i++)
        {
            if (str_comp(cur->files.data[i].name, token, 0) == 0)
            {
                found = true;
                cur = &(cur->files.data[i]);
                break;
            }
        }
    }

    return (found) ? cur : NULL;
}

static bool
fat32_read(struct fat32 *f, struct fat32e *fe, u32 sector, u8 *out)
{
    bool ret = false;

    u32 cluster_n = sector / f->br.sectspercluster;
    u32 sector_n = sector % f->br.sectspercluster;

    u32 cluster = fe->cluster;
    for (u32 i = 0; i < cluster_n && cluster < 0xFFFFFFF8; i++)
        cluster = fat32_next(f, cluster);

    if (cluster < 0xFFFFFFF8)
    {
        cluster &= 0xFFFFFFF;
        u32 firstsect = fat32_fsector(f, cluster);
        ret = BLOCK_R(*(f->storage), 0, out, firstsect + sector_n);
    }

    return ret;
}

struct file
{
    struct fat32 *fat32;
    struct fat32e *fat32e;
};

static void
init(void **ctx, struct device *storage)
{
    if (storage)
        *ctx = fat32_new(storage);
}

static void
clean(void *ctx)
{
    fat32_del(ctx);
}

static struct file *
fs_close(struct file *f)
{
    return mem_del(f);
}

static struct file *
fs_open(void *ctx, char *path)
{
    struct file *ret = mem_new(sizeof(struct file));

    if (ret)
    {
        ret->fat32 = ctx;
        ret->fat32e = fat32_find(ret->fat32, path);
        if (!(ret->fat32e))
            ret = fs_close(ret);
    }

    return ret;
}

static void
fs_info(struct file *f, size_t *size, s32 *files)
{
    if (f && size)
        *size = f->fat32e->size;
    if (f && files)
        *files = (f->fat32e->attributes & 0x10) ?
                  (s32)f->fat32e->files.count : -1;
}

static struct file *
fs_index(struct file *f, u32 index)
{
    struct file *ret = NULL;

    if (f && f->fat32e->files.count > index)
        ret = mem_new(sizeof(struct file));

    if (ret)
    {
        ret->fat32 = f->fat32;
        ret->fat32e = &(f->fat32e->files.data[index]);
    }

    return ret;
}

static bool
fs_read(struct file *f, u32 sector, u8 *buffer)
{
    bool ret = false;

    if (f)
        ret = fat32_read(f->fat32, f->fat32e, sector, buffer);

    return ret;
}

DECLARE_DRIVER(fat32)
{
    .init = init, .clean = clean,
    .api = DRIVER_API_FS,
    .type = DRIVER_TYPE_FS,
    .interface.fs.open  = fs_open,
    .interface.fs.close = fs_close,
    .interface.fs.info  = fs_info,
    .interface.fs.index = fs_index,
    .interface.fs.read  = fs_read
};
