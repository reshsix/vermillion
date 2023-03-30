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

#include <_types.h>
#include <string.h>
#include <stdlib.h>
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
    u32 lba;
    struct fat32br br;
    u8 *buffer;
    u32 *table;
    struct fat32e root;

    bool (*read)(u8*, u32, u32);
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

    memcpy(name8, code, 8);
    for (int k = 7; k >= 0 && name8[k] == ' '; k--)
        name8[k] = '\0';

    memcpy(ext3, &(code[8]), 3);
    for (int k = 2; k >= 0 && ext3[k] == ' '; k--)
        ext3[k] = '\0';

    strcpy(id83, name8);
    if (ext3[0] != '\0')
        strcat(id83, ".");
    strcat(id83, ext3);

    ret = strlen(id83) + 1;
    memcpy(buf, id83, ret);

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

        free(fe->name);
        free(fe->files.data);
    }

    return false;
}

static bool
fat32e_init(struct fat32e *fe, char *name, u8 *entry)
{
    bool ret = true;

    fe->name = calloc(1, strlen(name) + 1);
    if (!(fe->name))
        ret = fat32e_clean(fe);

    if (ret)
    {
        strcpy(fe->name, name);

        fe->attributes = entry[11];

        u16 time, date;
        memcpy(&time, &(entry[14]), 2);
        memcpy(&date, &(entry[16]), 2);
        fe->created = fat32t_to_unix(time, date);
        memcpy(&date, &(entry[18]), 2);
        fe->accessed = fat32t_to_unix(0, date);
        memcpy(&time, &(entry[22]), 2);
        memcpy(&date, &(entry[24]), 2);
        fe->modified = fat32t_to_unix(time, date);

        memcpy(&(fe->size), &(entry[28]), 4);

        u16 cluster_l, cluster_h;
        memcpy(&cluster_l, &(entry[26]), 2);
        memcpy(&cluster_h, &(entry[20]), 2);
        fe->cluster = cluster_l | (cluster_h << 16);

        if (fe->attributes & 0x10)
        {
            fe->files.length = 16;
            fe->files.data = calloc(fe->files.length, sizeof(struct fat32e));
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
        memcpy(&ret, &(f->table[cluster]), 4);

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
    ret = f->read(f->buffer, f->lba + firstsect, f->br.sectspercluster);

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
            memcpy(lfn_p, &(f->buffer[i + 1]), 10);
            lfn_p = &(lfn_p[5]);
            memcpy(lfn_p, &(f->buffer[i + 14]), 12);
            lfn_p = &(lfn_p[6]);
            memcpy(lfn_p, &(f->buffer[i + 28]), 4);
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
        if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
        {
            if (out->files.count >= out->files.length)
            {
                out->files.data = realloc(out->files.data,
                                          out->files.length * 2);
                memset(&(out->files.data[out->files.length]), 0,
                       out->files.length);
                out->files.length *= 2;
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
        free(f->buffer);
        free(f);
    }

    return NULL;
}

static struct fat32 *
fat32_new(u32 lba, bool (*read)(u8*, u32, u32))
{
    struct fat32 *ret = calloc(1, sizeof(struct fat32));

    if (ret)
    {
        ret->lba = lba;
        ret->read = read;

        if (!ret->read(fat32_buf, ret->lba, 1))
            ret = fat32_del(ret);
    }

    if (ret)
    {
        memcpy(&(ret->br), fat32_buf, sizeof(struct fat32br));

        ret->buffer = malloc(0x200 * ret->br.sectspercluster);
        ret->table = malloc(0x200 * ret->br.sectspertable);
        if (!(ret->buffer && ret->table))
            ret = fat32_del(ret);
    }

    if (ret)
    {
        u32 lba = ret->lba + ret->br.reservedsects;
        if (!ret->read((u8*)ret->table, lba, ret->br.sectspertable))
            ret = fat32_del(ret);
    }

    if (ret)
    {
        ret->root.files.length = 16;
        ret->root.files.data = calloc(ret->root.files.length,
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
    struct fat32e *cur = &(f->root);
    for (char *token = strtok(path, "/"); token; token = strtok(NULL, "/"))
    {
        found = false;
        for (size_t i = 0; i < cur->files.count; i++)
        {
            if (strcmp(cur->files.data[i].name, token) == 0)
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
        ret = f->read(out, f->lba + firstsect + sector_n, 1);
    }

    return ret;
}

struct fs
{
    struct fat32 *fat32;
};

static struct fs fs;

struct file
{
    struct fat32 *fat32;
    struct fat32e *fat32e;
};

static void
clean(void)
{
    fat32_del(fs.fat32);
}

static bool
init(void)
{
    bool ret = false;

    const struct driver *st0 = driver_find(DRIVER_TYPE_STORAGE, 0);
    if (st0->routines.storage.read(fat32_buf, 0, 1))
    {
        u32 lba = ((u32*)&(fat32_buf[0x1BE]))[2];
        fs.fat32 = fat32_new(lba, st0->routines.storage.read);
        if (fs.fat32)
            ret = true;
    }

    return ret;
}

static struct file *
fs_close(struct file *f)
{
    free(f);
    return NULL;
}

static struct file *
fs_open(char *path)
{
    struct file *ret = malloc(sizeof(struct file));

    if (ret)
    {
        ret->fat32 = fs.fat32;
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
        ret = malloc(sizeof(struct file));

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

static const struct driver fat32 =
{
    .name = "FAT32/MBR",
    .init = init, .clean = clean,
    .type = DRIVER_TYPE_FS,
    .routines.fs.open  = fs_open,
    .routines.fs.close = fs_close,
    .routines.fs.info  = fs_info,
    .routines.fs.index = fs_index,
    .routines.fs.read  = fs_read
};
driver_register(fat32);
