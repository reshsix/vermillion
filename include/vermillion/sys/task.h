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

typedef struct vrm_task vrm_task;

enum vrm_task_st
{
    VRM_TASK_NEW,
    VRM_TASK_READY,
    VRM_TASK_BLOCKED,
    VRM_TASK_DELETED
};

vrm_task * vrm_task_create   (void (*f)(void *), void *arg, uint8_t priority);
vrm_task * vrm_task_remove   (vrm_task *t);
bool       vrm_task_block    (vrm_task *t);
bool       vrm_task_unblock  (vrm_task *t);
bool       vrm_task_suspend  (vrm_task *t);
bool       vrm_task_resume   (vrm_task *t);
bool       vrm_task_priority (vrm_task *t, uint8_t priority);
void       vrm_task_yield    (void);
void       vrm_task_scheduler(uint8_t timer, uint32_t us, uint32_t flags);
