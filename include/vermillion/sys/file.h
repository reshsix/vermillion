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
    void * (*root)(void *ctx);
    void * (*walk)(void *ctx, void *parent, void *entry,
                   bool *dir, char *name, uint32_t *size);
    bool   (*read)  (void *ctx, void *entry,
                           uint8_t *data, uint32_t block);
    bool   (*write) (void *ctx, void *entry,
                     const uint8_t *data, uint32_t block);
    bool   (*resize)(void *ctx, void *entry,  uint32_t size);
    bool   (*create)(void *ctx, void *parent, const char *name, bool dir);
    bool   (*remove)(void *ctx, void *entry);
} drv_fs;

typedef struct
{
    const drv_fs *driver;
    void *context;
} dev_fs;

struct vrm_file
{
    dev_fs *df;
    void *cache;

    uint8_t *buffer;
    uint32_t width, block;
    bool flush;

    bool dir;
    uint32_t size;

    uint32_t pos;
};

void file_setup(dev_fs *list, uint8_t count);
#endif

typedef struct vrm_file vrm_file;

vrm_file *vrm_file_open(uint8_t id, const char *path);
vrm_file *vrm_file_close(vrm_file *f);

bool vrm_file_stat(vrm_file *f, bool *dir, uint32_t *size);
void *vrm_file_walk(vrm_file *f, void *state,
                    bool *dir, char *name, uint32_t *size);

bool vrm_file_seek(vrm_file *f, uint32_t pos);
bool vrm_file_tell(vrm_file *f, uint32_t *pos);

uint32_t vrm_file_read (vrm_file *f,       void *buffer, uint32_t bytes);
uint32_t vrm_file_write(vrm_file *f, const void *buffer, uint32_t bytes);
bool vrm_file_flush(vrm_file *f);

bool vrm_file_resize(vrm_file *f, uint32_t size);

bool vrm_file_create(uint8_t id, const char *path, bool dir);
bool vrm_file_remove(uint8_t id, const char *path);
