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

CC      = arm-none-eabi-gcc
CFLAGS  = --std=c99 -Iinclude -I/usr/local/include -Wall -Wextra
CFLAGS += -shared -fPIE -fPIC -ffreestanding -nostdlib -Wl,-z,defs

.PHONY: all clean

all: $(PROJECT).tar
clean:
	@rm -rf build
	@rm -f $(PROJECT).tar

FOLDERS = build build/prog build/lib
$(FOLDERS):
	@mkdir -p $@

PROGS := $(addprefix build/prog/, $(PROGS))
build/prog/%.elf: prog/%.c | $(FOLDERS)
	@printf '%s\n' "  PROG    $(@:build/prog/%=%)"
	@$(CC) $(CFLAGS) -Wl,-evrm_prog $< -o $@

LIBS := $(addprefix build/lib/, $(LIBS))
build/lib/%.elf: lib/%.c | $(FOLDERS)
	@printf '%s\n' "  LIB     $(@:build/lib/%=%)"
	@$(CC) $(CFLAGS) -Wl,-evrm_lib $< -o $@

$(PROJECT).tar: $(PROGS) $(LIBS) | $(FOLDERS)
	@printf '%s\n' "  PACKAGE $@"
	@cd build && tar -cf ../$@ *
