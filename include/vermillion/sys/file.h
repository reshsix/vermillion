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

#pragma once

#include <vermillion/util/types.h>

#ifdef VERMILLION_INTERNALS
typedef struct
{
    void *init, (*clean)(void *);
    uint32_t (*root)  (void *ctx);
    bool (*walk)  (void *ctx, uint32_t parent, uint32_t *idx,
                   bool *dir, char *name, uint32_t *size, uint32_t *location);
    bool (*read)  (void *ctx, uint32_t location,
                   uint8_t *data, uint32_t block);
    bool (*write) (void *ctx, uint32_t location,
                   const uint8_t *data, uint32_t block);
    bool (*create)(void *ctx, uint32_t parent, const char *name, bool dir,
                   uint32_t location, uint32_t size);
    bool (*remove)(void *ctx, uint32_t parent, uint32_t idx, bool data);
    bool (*resize)(void *ctx, uint32_t parent, uint32_t idx, uint32_t size,
                   uint32_t *location);
} drv_fs;

typedef struct
{
    const drv_fs *driver;
    void *context;
} dev_fs;

struct vrm_file
{
    uint8_t dev;

    uint32_t parent, idx, location;

    bool dir;
    uint32_t size;

    uint8_t  buffer[0x200];
    uint32_t block, pos;
    bool     flush;
};

void file_setup(dev_fs *list, uint8_t count);
#endif

#define VRM_FILE_PATH_S 256

bool vrm_file_validate(const char *path);
void vrm_file_sanitize(char *path);
void vrm_file_dirname (char *path);
void vrm_file_basename(char *path);

typedef struct vrm_file vrm_file;

vrm_file *vrm_file_open(uint8_t id, const char *path);
vrm_file *vrm_file_close(vrm_file *f);

bool vrm_file_stat(vrm_file *f, bool *dir, uint32_t *size);
bool vrm_file_walk(vrm_file *f, uint32_t *idx,
                   bool *dir, char *name, uint32_t *size);

bool vrm_file_seek(vrm_file *f, uint32_t  pos);
bool vrm_file_tell(vrm_file *f, uint32_t *pos);

uint32_t vrm_file_read (vrm_file *f,       void *buffer, uint32_t bytes);
uint32_t vrm_file_write(vrm_file *f, const void *buffer, uint32_t bytes);
bool vrm_file_flush(vrm_file *f);

bool vrm_file_create(uint8_t id, const char *path, bool dir);
bool vrm_file_remove(uint8_t id, const char *path);

bool vrm_file_resize(vrm_file *f, uint32_t size);
bool vrm_file_move(vrm_file *f, const char *path);
