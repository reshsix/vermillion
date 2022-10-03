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

ENTRY(_start)

SECTIONS
{
    . = 0x40008000;

    .text   : { KEEP(*(.text.boot)) *(.text) }
    . = ALIGN(0x1000);

    .rodata : { *(.rodata) }
    . = ALIGN(0x1000);

    .data   : { *(.data) }
    . = ALIGN(0x1000);

    _bss_s = .;
    .bss    : { *(.bss COMMON) }
    . = ALIGN(0x1000);
    _bss_e = .;
}
