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

.set MAGIC, 0xE85250D6
.set ARCH,  0x0

.section ".text.boot"

.align 8
header:
.long MAGIC                                   // MAGIC
.long ARCH                                    // ARCH
.long header_end - header                     // LENGTH
.long -(MAGIC + ARCH + (header_end - header)) // CHECKSUM

.word 5  // TYPE
.word 0  // FLAGS
.word 20 // SIZE
.long 0  // WIDTH
.long 0  // HEIGHT
.long 32 // DEPTH
header_end:

.global multiboot_addr
multiboot_addr:
.long 0

.section ".text"
.global _start
_start:
    // Saving multiboot2 info address
    movl %ebx, multiboot_addr

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
    call thread_scheduler
1:
    hlt
    jmp 1b
.size _start, . - _start
