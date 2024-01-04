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

QEMU_MACHINE = $(shell echo $(CONFIG_QEMU_MACHINE))

# --------------------------------- Recipes  --------------------------------- #

# Helper recipes
image: $(BUILD)/vermillion.img
debug: $(BUILD)/vermillion.img scripts/debug.gdb
	@printf '%s\n' "  QEMU    $(<:$(BUILD)/%=%)"
	@qemu-system-arm -s -S -M $(QEMU_MACHINE) -drive file=$<,format=raw &
	@gdb-multiarch --command=scripts/debug.gdb
test: $(BUILD)/vermillion.img
	@printf '%s\n' "  TEST    $(<:$(BUILD)/%=%)"
	@chronic qemu-system-arm -M $(QEMU_MACHINE) \
             -semihosting -nographic -drive file=$<,format=raw

# Specific recipes
$(BUILD)/boot.o: arch/$(ARCH)/boot.S deps/.$(TARGET)-gcc | $(BUILD)
	@printf '%s\n' "  AS      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/boot.scr: $(BUILD)/arch/u-boot.cmd | $(BUILD)
	@printf '%s\n' "  MKIMAGE core/$(@:$(BUILD)/%=%)"
	@chronic mkimage -C none -A $(ARCH) -T script -d $< $@
$(BUILD)/vermillion.img: deps/u-boot.bin $(BUILD)/kernel.bin \
                      $(BUILD)/boot.scr | $(BUILD)/mount
	@printf '%s\n' "  BUILD   $(@:$(BUILD)/%=%)"
	@dd if=/dev/zero of=$@ bs=1M count=$(CONFIG_DISK_SIZE) status=none
	@sudo losetup /dev/loop0 $@
	@printf 'start=2048, type=83, bootable\n' \
     | sudo chronic sfdisk -q /dev/loop0
	@sudo partx -a /dev/loop0
	@sudo chronic mkfs.fat -F32 "/dev/loop0p1"
	@sudo mount /dev/loop0p1 $(BUILD)/mount
	@sudo mkdir -p $(BUILD)/mount/boot/
	@sudo cp $(BUILD)/boot.scr $(BUILD)/mount/
	@sudo cp $(BUILD)/kernel.bin $(BUILD)/mount/boot/
	@sudo umount $(BUILD)/mount
	@sudo dd if=deps/u-boot.bin of=/dev/loop0 bs=1024 seek=8 status=none
	@sleep 1
	@sudo partx -d /dev/loop0
	@sudo losetup -d /dev/loop0

# --------------------------------- Objects  ---------------------------------

OBJS += boot.o
