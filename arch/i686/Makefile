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

# -------------------------------- Parameters -------------------------------- #

ARCH = $(shell echo $(CONFIG_ARCH))
TARGET = $(shell echo $(CONFIG_TARGET))

# --------------------------------- Recipes  --------------------------------- #

# Helper recipes
.PHONY: $(.PHONY) image debug
image: build/os.img
kdebug: build/kernel.elf scripts/debug.gdb
	@printf "  QEMU    $<\n"
	@qemu-system-i386 -s -S -kernel $< &
	@gdb-multiarch --command=scripts/debug.gdb
debug: build/os.img scripts/debug.gdb
	@printf "  QEMU    $<\n"
	@qemu-system-i386 -s -S -cdrom $< &
	@gdb-multiarch --command=scripts/debug.gdb

# Specific recipes
build/boot.o: arch/$(ARCH)/boot.S deps/.$(TARGET)-gcc | build
	@printf "  AS      $@\n"
	@$(CC) $(CFLAGS) -c $< -o $@
build/os.img: build/kernel.elf arch/$(ARCH)/grub.cfg | build/mount
	@printf "  BUILD   $@\n"
	@mkdir -p build/mount/boot/grub
	@cp $< build/mount/boot/
	@cp arch/$(ARCH)/grub.cfg build/mount/boot/grub/
	@chronic grub-mkrescue -o $@ build/mount
