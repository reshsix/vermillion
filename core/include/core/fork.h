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

#ifndef CORE_FORK_H
#define CORE_FORK_H

typedef struct fork
{
    void *stack;
    void (*f)(void*), *arg;

    #if defined(CONFIG_ARCH_ARM)
    void *fp, *sp;
    #elif defined(CONFIG_ARCH_I686)
    void *ebp, *esp;
    #endif

    struct fork *previous;
} fork;

fork *fork_new(void (*f)(void *), void *arg);
fork *fork_del(fork *fk);
void fork_run(fork *fk);

#endif
