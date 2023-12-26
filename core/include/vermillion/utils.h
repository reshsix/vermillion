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

#ifndef VERMILLION_UTILS_H
#define VERMILLION_UTILS_H

#include <vermillion/types.h>
#include <vermillion/drivers.h>

struct device *logger(struct device *log);
void log_c(const char c);
void log_s(const char *s);
void log_h(const u32 n);
void log_u(const u32 n);
void panic(const char *s);

u32 clock(struct device *tmr);
void csleep(struct device *tmr, const u32 n);
void usleep(struct device *tmr, const u32 n);
void msleep(struct device *tmr, const u32 n);
void sleep(struct device *tmr, const u32 n);

bool pin_cfg(struct device *gpio, u16 pin, u8 role, u8 pull);
bool pin_set(struct device *gpio, u16 pin, bool data);
bool pin_get(struct device *gpio, u16 pin, bool *data);

void *mem_new(size_t size);
void *mem_renew(void *mem, size_t size);
void *mem_del(void *mem);

int mem_comp(const void *mem, const void *mem2, size_t length);
void *mem_find(const void *mem, u8 c, size_t length);
void mem_init(void *mem, u8 c, size_t length);
void mem_copy(void *dest, const void *src, size_t length);

size_t str_length(const char *str);
int str_comp(const char *str, const char *str2, size_t length);
size_t str_span(const char *str, const char *chars, bool complement);
char *str_find_l(const char *str, char c);
char *str_find_r(const char *str, char c);
char *str_find_m(const char *str, const char *chars);
char *str_find_s(const char *str, const char *str2);
char *str_token(char *str, const char *chars, char **saveptr);
void str_copy(char *dest, char *src, size_t length);
void str_concat(char *dest, char *src, size_t length);
char *str_dupl(char *str, size_t length);

bool intr_config(u16 n, void (*f)(void), bool enable, u8 priority);
void intr_wait(void);

void memcpy(void *dest, const void *src, size_t length);
void memmove(void *dest, const void *src, size_t length);
void memset(void *mem, int c, size_t length);
int memcmp(const void *mem, const void *mem2, size_t length);

struct state;
struct state *state_new(void);
struct state *state_del(struct state *st);
void *state_save(struct state *st);
noreturn state_load(struct state *st, void *ret);

struct fork;
struct fork *fork_new(void (*f)(void *), void *arg);
struct fork *fork_del(struct fork *fk);
void fork_run(struct fork *fk);

struct generator;
struct generator *generator_new(void (*f)(struct generator *), void *arg);
struct generator *generator_del(struct generator *g);
bool generator_next(struct generator *g);
void generator_rewind(struct generator *g);
void *generator_arg(struct generator *g);
void generator_yield(struct generator *g);
noreturn generator_finish(struct generator *g);

#define THREAD(id) \
    noreturn id(__attribute__((unused)) struct generator *___)
struct thread;
struct thread *thread_new(THREAD(f), void *arg, bool persistent, u8 priority);
struct thread *thread_del(struct thread *t);
size_t thread_sync(struct thread *t, size_t step);
size_t thread_wait(struct thread *t);
bool thread_rewind(struct thread *t);
void *thread_arg(void);
void thread_block(bool state);
void thread_yield(void);
noreturn thread_loop(void);
noreturn thread_finish(void);

#define __CONCAT(a, b) a##b
#define _CONCAT(a, b) __CONCAT(a, b)
#define _UNIQUE(x) _CONCAT(x, __LINE__)

#define SEMAPHORE(count) \
    static int _UNIQUE(_semaphore_s) = count; \
    bool _UNIQUE(_semaphore) = true; \
    for (semaphore_wait(&_UNIQUE(_semaphore_s)); _UNIQUE(_semaphore); \
         semaphore_signal(&_UNIQUE(_semaphore_s)), \
                          _UNIQUE(_semaphore) = false)
void semaphore_wait(int *s);
void semaphore_signal(int *s);
#define MUTEX(...) \
    void *_UNIQUE(_mutex_p) = NULL; \
    __VA_OPT__(_UNIQUE(_mutex_p) = (void *)__VA_ARGS__;) \
    static void * _UNIQUE(_mutex_m) = NULL; \
    bool _UNIQUE(_mutex) = true; \
    for (mutex_lock(&_UNIQUE(_mutex_m), _UNIQUE(_mutex_p)); _UNIQUE(_mutex); \
         mutex_unlock(&_UNIQUE(_mutex_m), _UNIQUE(_mutex_p)), \
        _UNIQUE(_mutex) = false)
void mutex_lock(void **m, void *param);
void mutex_unlock(void **m, void *param);
#define CRITICAL \
    bool _UNIQUE(_critical) = true; \
    for (critical_lock(); _UNIQUE(_critical); critical_unlock(), \
         _UNIQUE(_critical) = false)
void critical_lock(void);
void critical_unlock(void);

struct channel;
struct channel *channel_new(size_t type, size_t size);
struct channel *channel_del(struct channel *ch);
void channel_read(struct channel *ch, void *data);
void channel_write(struct channel *ch, void *data);

void _utils_init(void);
void _utils_clean(void);

#endif
