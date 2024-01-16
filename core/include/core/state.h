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

#ifndef CORE_STATE_H
#define CORE_STATE_H

#include <core/state.h>

typedef struct [[packed, aligned(4)]] state
{
    #if defined(CONFIG_ARCH_ARM)
    void *gpr[13];
    #elif defined(CONFIG_ARCH_I686)
    void *gpr[6], *retaddr;
    #endif
} state;

void *state_save(state *st);
[[noreturn]] void state_load(state *st, void *ret);

#endif
