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

#ifndef DRIVERS_FAT32_H
#define DRIVERS_FAT32_H

#include <types.h>

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

struct fat32 *fat32_new(u32 lba, bool (*read)(u8*, u32, u32));
struct fat32 *fat32_del(struct fat32 *f);
struct fat32e *fat32_find(struct fat32 *f, char *path);
bool fat32_read(struct fat32 *f, struct fat32e *fe, u32 sector, u8 *out);

#endif
