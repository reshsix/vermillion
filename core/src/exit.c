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

#include <core/types.h>

#include <debug/exit.h>

#if defined(CONFIG_ARCH_I686)
#include <i686/env.h>
#endif

[[noreturn]]
extern void
exit_qemu(bool failure)
{
    #if defined(CONFIG_ARCH_ARM)

    register int r0 asm("r0") = 0x18;
    register int r1 asm("r1") = (failure) ? 0x20024 : 0x20026;
    (void)r0, (void)r1;

    asm volatile ("svc #0x00123456");

    #elif defined(CONFIG_ARCH_I686)

    out8(0x501, (failure) ? 1 : (255 - 1) / 2);

    #endif

    while (true);
}
