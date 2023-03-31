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
UBOOT_IMAGE = $(shell echo $(CONFIG_UBOOT_IMAGE))
UBOOT_CONFIG = $(shell echo $(CONFIG_UBOOT_CONFIG))
QEMU_MACHINE = $(shell echo $(CONFIG_QEMU_MACHINE))

# Dependency versions
UBOOT = v2020.04

# --------------------------------- Recipes  --------------------------------- #

# Helper recipes
.PHONY: $(.PHONY) image debug
image: build/os.img
debug: build/os.img scripts/debug.gdb
	@printf "  QEMU    $<\n"
	@qemu-system-arm -s -S -M $(QEMU_MACHINE) -drive file=$<,format=raw &
	@gdb-multiarch --command=scripts/debug.gdb

# Specific recipes
build/boot.o: arch/$(ARCH)/boot.S deps/.$(TARGET)-gcc | build
	@printf "  AS      $@\n"
	@$(CC) $(CFLAGS) -c $< -o $@
build/boot.scr: build/arch/u-boot.cmd | build
	@printf "  MKIMAGE $@\n"
	@chronic mkimage -C none -A $(ARCH) -T script -d $< $@
build/os.img: deps/u-boot.bin build/kernel.bin \
              build/boot.scr | build/mount
	@printf "  BUILD   $@\n"
	@dd if=/dev/zero of=$@ bs=1M count=$(CONFIG_DISK_SIZE) status=none
	@sudo losetup /dev/loop0 $@
	@printf 'start=2048, type=83, bootable\n' \
     | sudo chronic sfdisk -q /dev/loop0
	@sudo partx -a /dev/loop0
	@sudo chronic mkfs.fat -F32 "/dev/loop0p1"
	@sudo mount /dev/loop0p1 build/mount
	@sudo mkdir -p build/mount/boot/
	@sudo cp build/boot.scr build/mount/
	@sudo cp build/kernel.bin build/mount/boot/
	@sudo umount build/mount
	@sudo dd if=deps/u-boot.bin of=/dev/loop0 bs=1024 seek=8 status=none
	@sleep 1
	@sudo partx -d /dev/loop0
	@sudo losetup -d /dev/loop0

# ------------------------------- Dependencies ------------------------------- #

# U-boot compilation
deps/u-boot.bin: deps/.$(TARGET)-gcc deps/.u-boot-step4
deps/u-boot: | deps
	cd $| && git clone https://gitlab.denx.de/u-boot/u-boot.git
deps/.u-boot-step1: | deps/u-boot
	cd $| && git checkout tags/$(UBOOT)
	touch $@
deps/.u-boot-step2: deps/.u-boot-step1 | deps/u-boot
	cd $| && make ARCH=$(ARCH) CROSS_COMPILE=$(TARGET)- $(UBOOT_CONFIG)
	[ -f arch/$(ARCH)/u-boot.patch ] && patch deps/u-boot/.config < \
                                              arch/$(ARCH)/u-boot.patch
	touch $@
deps/.u-boot-step3: deps/.u-boot-step2 | deps/u-boot
	cd $| && make ARCH=$(ARCH) CROSS_COMPILE=$(TARGET)-
	touch $@
deps/.u-boot-step4: deps/.u-boot-step3 | deps/u-boot
	cp $|/$(UBOOT_IMAGE) deps/u-boot.bin
	touch $@
