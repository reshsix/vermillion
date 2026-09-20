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

struct vrm_task_list
{
    vrm_task *head, *tail;
};

/* Main functions */

vrm_task * vrm_task_create   (void (*f)(void *), void *arg, uint8_t priority);
vrm_task * vrm_task_remove   (vrm_task *t);
void       vrm_task_block    (vrm_task *t, struct vrm_task_list *list);
void       vrm_task_unblock  (vrm_task *t, struct vrm_task_list *list);
void       vrm_task_priority (vrm_task *t, uint8_t priority);
void       vrm_task_yield    (void);
void       vrm_task_scheduler(uint8_t timer, uint32_t us, uint32_t flags);

/* Tick counter */

uint64_t vrm_task_ticks(void);

#define VRM_TASK_RUNNING \
    if (vrm_task_ticks())

/* Critical sections */

void vrm_task_crit_in (void);
void vrm_task_crit_out(void);

#define VRM_TASK_CRITICAL \
    for (int _once##__LINE__ = (vrm_task_crit_in(), 1); \
             _once##__LINE__;                               \
         (vrm_task_crit_out(), _once##__LINE__ = 0))
#define VRM_TASK_NONCRITICAL \
    for (int _once##__LINE__ = (vrm_task_crit_out(), 1); \
             _once##__LINE__;                               \
         (vrm_task_crit_in(), _once##__LINE__ = 0))

/* Semaphores */

struct vrm_task_sem
{
    size_t count;
};
bool vrm_task_sem_take(struct vrm_task_sem *s, uint32_t timeout);
void vrm_task_sem_give(struct vrm_task_sem *s);

#define VRM_TASK_SEMAPHORE(s, timeout) \
    if (vrm_task_sem_take(s, timeout)) \
        for (bool _once##__LINE__ = 1; _once##__LINE__; \
            (vrm_task_sem_give(s), _once##__LINE__ = 0))

/* Mutexes */

struct vrm_task_mut
{
    vrm_task *owner;
};
bool vrm_task_mut_lock  (struct vrm_task_mut *m, uint32_t timeout);
void vrm_task_mut_unlock(struct vrm_task_mut *m);

#define VRM_TASK_MUTEX(m, timeout) \
    if (vrm_task_mut_lock(m, timeout)) \
        for (bool _once##__LINE__ = 1; _once##__LINE__; \
            (vrm_task_mut_unlock(m), _once##__LINE__ = 0))

/* Delay */

void vrm_task_delay(uint32_t ticks);
