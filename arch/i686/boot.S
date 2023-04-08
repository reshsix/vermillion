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
.set MAGIC,    0x1BADB002
.set FLAGS,    0x1
.set CHECKSUM, -(MAGIC + FLAGS)

.long MAGIC
.long FLAGS
.long CHECKSUM

.global _start
_start:
    // Clearing bss with zeros
    movl $__bss_s, %eax
    movl $__bss_e, %ebx
1:
    movl $0, (%eax)
    addl $4, %eax
    cmp %ebx, %eax
    jl 1b

    // Disable interrupts
    cli

    // Set stack and jump to C code
    movl $__stack, %esp
    call __init
1:
    hlt
    jmp 1b
.size _start, . - _start
