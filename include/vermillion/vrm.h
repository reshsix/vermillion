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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef VERMILLION_INTERNALS
typedef struct fs_file vrm_disk_f;
typedef struct dict    vrm_dict;
#else
typedef struct vrm_disk_f vrm_disk_f;
typedef struct vrm_dict   vrm_dict;
#endif

struct vrm
{
    /* General functions */

    struct
    {
        void * (*new)(size_t size);
        void * (*renew)(void *mem, size_t size);
        void * (*del)(void *mem);
        int (*comp)(const void *mem, const void *mem2, size_t length);
        void * (*find)(const void *mem, uint8_t c, size_t length);
        void (*fill)(void *mem, uint8_t c, size_t length);
        void (*copy)(void *dest, const void *src, size_t length);
    } mem;

    struct
    {
        size_t (*length)(const char *str);
        int  (*comp)(const char *str, const char *str2, size_t length);
        size_t (*span)(const char *str, const char *chars, bool complement);
        char * (*find_l)(const char *str, char c);
        char * (*find_r)(const char *str, char c);
        char * (*find_m)(const char *str, const char *chars);
        char * (*find_s)(const char *str, const char *str2);
        char * (*token)(char *str, const char *chars, char **saveptr);
        void (*copy)(char *dest, const char *src, size_t length);
        void (*concat)(char *dest, const char *src, size_t length);
        char * (*dupl)(const char *str, size_t length);
    } str;

    struct
    {
        vrm_dict * (*new)(size_t type);
        vrm_dict * (*del)(vrm_dict *d);
        bool (*get)(vrm_dict *d, const char *id, void *data);
        bool (*set)(vrm_dict *d, const char *id, void *data);
    } dict;

    struct
    {
        bool (*validate)(const char *path);
        void (*cleanup)(char *path);
        void (*dirname)(char *path);
        void (*filename)(char *path);
    } path;

    /* System functions */

    struct
    {
        char (*read0)(void);
        char (*read1)(void);
        void (*write0)(char c);
        void (*write1)(char c);
    } comm;

    struct
    {
        vrm_disk_f * (*open)(const char *path);
        vrm_disk_f * (*close)(vrm_disk_f *f);

        bool (*stat)(vrm_disk_f *f,
                     bool *dir, char **name, uint32_t *size);
        bool (*walk)(vrm_disk_f *f, uint32_t index,
                     bool *dir, char **name, uint32_t *size);

        bool (*seek)(vrm_disk_f *f, uint32_t  pos);
        bool (*tell)(vrm_disk_f *f, uint32_t *pos);

        uint32_t (*read)(vrm_disk_f *f, void *buffer, uint32_t bytes);
        uint32_t (*write)(vrm_disk_f *f, void *buffer, uint32_t bytes);
        bool (*flush)(vrm_disk_f *f);

        bool (*resize)(vrm_disk_f *f, uint32_t size);

        bool (*create)(const char *path, bool dir);
        bool (*remove)(const char *path);
    } disk;

    struct
    {
        bool (*event)(void (*handler)(void *), void *arg, uint8_t jiffies);
        void (*sleep)(uint8_t jiffies);
        uint64_t (*clock)(void);
    } time;

    struct
    {
        void * (*get)(const char *id);
        bool (*set)(const char *id, void *data);
    } vars;

    /* Main-adjacent functions */

    struct
    {
        uint8_t * (*prog)(const char *path, uint32_t *entry);
    } loader;

    struct
    {
        void (*char_)(const char c);
        void (*string)(const char *s);
        void (*bool_)(const bool n);
        void (*unsigned_)(const uint64_t n);
        void (*signed_)(int64_t n);
    } syslog;
};
