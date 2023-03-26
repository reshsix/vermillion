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

#ifndef INTERFACE_FS_H
#define INTERFACE_FS_H

#include <_types.h>

struct file;

bool _fs_init(void);
void _fs_clean(void);

struct file *fs_open(char *path);
struct file *fs_close(struct file *f);
void fs_info(struct file *f, size_t *size, s32 *files);
struct file *fs_index(struct file *f, u32 index);
bool fs_read(struct file *f, u32 sector, u8 *buffer);

#endif
