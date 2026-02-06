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

#pragma once

#include <general/types.h>

#include <hal/classes/fs.h>

/* For devtree usage */

void disk_config(dev_fs *f);

/* For external usage */

typedef struct fs_file disk_f;

disk_f *disk_open(const char *path);
disk_f *disk_close(disk_f *f);

bool disk_stat(disk_f *f, bool *dir, char **name, u32 *size);
bool disk_walk(disk_f *f, u32 index, bool *dir, char **name, u32 *size);

bool disk_seek(disk_f *f, u32  pos);
bool disk_tell(disk_f *f, u32 *pos);

u32 disk_read(disk_f *f, void *buffer, u32 bytes);
u32 disk_write(disk_f *f, void *buffer, u32 bytes);
bool disk_flush(disk_f *f);

bool disk_rename(disk_f *f, const char *name);
bool disk_resize(disk_f *f, u32 size);

bool disk_mkfile(const char *path);
bool disk_mkdir(const char *path);
bool disk_remove(const char *path);
