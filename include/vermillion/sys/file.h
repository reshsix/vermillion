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

#include <general/types.h>

#ifdef VERMILLION_INTERNALS
typedef struct
{
    void *init, (*clean)(void *);
    void * (*root)(void *ctx);
    void * (*walk)(void *ctx, void *parent, void *entry,
                   bool *dir, char *name, uint32_t *size);
    bool   (*read)  (void *ctx, void *entry,  uint8_t *data, uint32_t block);
    bool   (*write) (void *ctx, void *entry,  uint8_t *data, uint32_t block);
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
typedef struct vrm_file vrm_file;

void file_setup(dev_fs *list, uint8_t count);

vrm_file *file_open(uint8_t id, const char *path);
vrm_file *file_close(vrm_file *f);

bool file_stat(vrm_file *f, bool *dir, uint32_t *size);
void *file_walk(vrm_file *f, void *state, bool *dir, char *name, uint32_t *size);

bool file_seek(vrm_file *f, uint32_t pos);
bool file_tell(vrm_file *f, uint32_t *pos);

uint32_t file_read (vrm_file *f, void *buffer, uint32_t bytes);
uint32_t file_write(vrm_file *f, void *buffer, uint32_t bytes);
bool file_flush(vrm_file *f);

bool file_resize(vrm_file *f, uint32_t size);

bool file_create(uint8_t id, const char *path, bool dir);
bool file_remove(uint8_t id, const char *path);
#endif

typedef struct vrm_file vrm_file;

struct vrm_file_v1
{
    vrm_file * (*open)(uint8_t volume, const char *path);
    vrm_file * (*close)(vrm_file *f);

    bool   (*stat)(vrm_file *f, bool *dir, uint32_t *size);
    void * (*walk)(vrm_file *f, void *state,
                   bool *dir, char *name, uint32_t *size);

    bool (*seek)(vrm_file *f, uint32_t pos);
    bool (*tell)(vrm_file *f, uint32_t *pos);

    uint32_t (*read) (vrm_file *f, void *buffer, uint32_t bytes);
    uint32_t (*write)(vrm_file *f, void *buffer, uint32_t bytes);
    bool (*flush)(vrm_file *f);

    bool (*resize)(vrm_file *f, uint32_t size);

    bool (*create)(uint8_t id, const char *path, bool dir);
    bool (*remove)(uint8_t id, const char *path);
};

enum
{
    VRM_FILE_V1 = 0
};

void *file_system(uint8_t version);
