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

#ifndef INTERFACE_STORAGE_H
#define INTERFACE_STORAGE_H

#include <types.h>
#include <drivers/fat32.h>

struct storage;
struct file;
struct storage *storage_new(void);
struct storage *storage_del(struct storage *st);
struct file *storage_open(struct storage *st, char *path);
struct file *storage_close(struct file *f);
void storage_info(struct file *f, size_t *size, s32 *files);
struct file *storage_index(struct file *f, u32 index);
bool storage_read(struct file *f, u32 sector, u8 *buffer);

#endif
