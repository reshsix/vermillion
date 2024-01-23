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

# --------------------------------- Recipes  --------------------------------- #

# Helper recipes
debug:
	@printf '%s\n' "  QEMU    vermillion.img"
	@qemu-system-i386 -s -S \
        -drive file=build/vermillion.img,index=0,if=ide,format=raw &
	@gdb-multiarch --command=scripts/debug.gdb

# Specific recipes
$(BUILD)/boot.o: arch/$(ARCH)/boot.S deps/.$(TARGET)-gcc | $(BUILD)
	@printf '%s\n' "  AS      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/root: $(BUILD)/kernel.elf arch/$(ARCH)/grub.cfg
	@mkdir -p $@/boot/grub
	@cp $(BUILD)/kernel.elf $@/boot/
	@cp arch/$(ARCH)/grub.cfg $@/boot/grub/
image: $(BUILD)/vermillion.img
	@printf '%s\n' "  GRUB    $(<:$(BUILD)/%=%)"
	@chronic sudo grub-install --target=i386-pc \
        --root-directory=$(BUILD)/mount/ \
        --boot-directory=$(BUILD)/mount/boot/ \
        --modules="normal part_msdos multiboot2 fat search" \
        /dev/loop0
	@sudo umount $(BUILD)/mount
	@sudo losetup -d /dev/loop0

# --------------------------------- Objects  ---------------------------------

OBJS += boot.o
