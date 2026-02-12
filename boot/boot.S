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

.section ".text.boot"

.global _start
_start:
    ldr pc, #(__ivt + 0x0)
    ldr pc, #(__ivt + 0x4)
    ldr pc, #(__ivt + 0x8)
    ldr pc, #(__ivt + 0xC)
    ldr pc, #(__ivt + 0x10)
    ldr pc, #(__ivt + 0x14)
    ldr pc, #(__ivt + 0x18)
    ldr pc, #(__ivt + 0x1C)

.global __ivt
__ivt:
    .word 1f
    .word 0x0
    .word 0x0
    .word 0x0
    .word 0x0
    .word 0x0
    .word 0x0
    .word 0x0

1:
    // Single core
    mrc p15, 0, r5, c0, c0, 5
    and r5, r5, #3
    cmp r5, #0
    bne 1f

    // Clearing bss with zeros
    ldr r4, =__bss_s
    ldr r5, =__bss_e
    eor r6, r6
    eor r7, r7
    eor r8, r8
    eor r9, r9
    b 2f
1:
    stmia r4!, {r6-r9}
2:
    cmp r4, r5
    blo 1b

    // Set interrupt vector table
    ldr r5, =_start
    mcr p15, 0, r5, c12, c0, 0

    // Set stack and jump to C code
    ldr sp, =_start
    bl main

1:
    wfe
    b 1b
.size _start, . - _start
