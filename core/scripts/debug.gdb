# This file is part of vermillion.

# Vermillion is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 3.

# Vermillion is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with vermillion. If not, see <https://www.gnu.org/licenses/>.

shell sleep 3
target remote localhost:1234

py import os
py gdb.set_convenience_variable("elf", os.environ["BUILD"] + "/kernel.elf")
eval "symbol-file %s", $elf

tui new-layout debug {-horizontal src 1 asm 1} 2 status 0 \
                     {-horizontal cmd 2 regs 1} 1
tui layout debug
tui reg all

break _start
continue
