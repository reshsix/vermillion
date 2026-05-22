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

enum pic_index
{
    PIC_STATE = 0 + 1,
    PIC_CONFIG_IRQ,
    PIC_CONFIG_SWI,
    PIC_WAIT,
    PIC_SYSCALL
};

typedef struct
{
    void *init, (*clean)(void *);
    bool (*stat) (void *ctx, u32 idx, u32 *width, u32 *length);
    bool (*read) (void *ctx, u32 idx, void *buffer, u32 block);
    bool (*write)(void *ctx, u32 idx, void *buffer, u32 block);
} drv_pic;

typedef struct
{
    const drv_pic *driver;
    void *context;
} dev_pic;

enum pic_level
{
    PIC_EDGE_H, PIC_EDGE_L, PIC_LEVEL_H, PIC_LEVEL_L, PIC_DOUBLE
};

struct pic_irq
{
    bool enabled;
    void (*handler)(void *), *arg;
    enum pic_level level;
};

struct pic_swi
{
    bool enabled;
    void (*handler)(void *, void *), *arg;
};

bool pic_state(dev_pic *dp, bool enabled);
bool pic_info(dev_pic *dp, u16 n, bool *enabled, void (**handler)(void *),
              void **arg, enum pic_level *level);
bool pic_config(dev_pic *dp, u16 n, bool enabled, void (*handler)(void *),
                void *arg, enum pic_level level);
bool pic_check(dev_pic *dp, u16 n, bool *enabled,
               void (**handler)(void *, void *), void **arg);
bool pic_setup(dev_pic *dp, u16 n, bool enabled,
               void (*handler)(void *, void *), void *arg);
bool pic_wait(dev_pic *dp);
bool pic_syscall(dev_pic *dp, u8 id, void *data);
