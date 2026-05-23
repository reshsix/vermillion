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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum vrm_driver
{
    VRM_UART = 0,
    VRM_GPIO,
    VRM_EINT,
    VRM_SPI,
    VRM_I2C,

    VRM_DISK,

    VRM_PIC,
    VRM_TIMER,
    VRM_POWER,

    VRM_FS,
};

struct vrm
{
    void * (*driver)(enum vrm_driver id, uint8_t version);

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
    } str;

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
        bool   (*load)(const char *name, void *pointer, void *memory);
        bool   (*unload)(const char *name);
        void * (*pointer)(const char *name);
    } libs;

    /* Main-adjacent functions */

    struct
    {
        uint8_t * (*fdpic)(uint8_t id, const char *path, uint32_t *entry);
    } loader;

    struct
    {
        void (*char_)(const char c);
        void (*string)(const char *s);
        void (*unsigned_)(uint64_t n);
        void (*signed_)(int64_t n);
    } syslog;
};
