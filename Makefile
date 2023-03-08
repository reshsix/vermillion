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

export PATH += :$(shell pwd)/deps/tools/bin

# -------------------------------- Parameters -------------------------------- #

# Compilation parameters
CC = arm-none-eabihf-gcc
CFLAGS += -O0 -ggdb3
CFLAGS += -Iinclude -std=gnu99 -fpic -nostdlib -ffreestanding -mcpu=cortex-a7
CFLAGS += -Wall -Wextra -Wno-attributes -Wl,--no-warn-rwx-segment
DISK_SIZE = 2

# Extra variables
VERSION = 0.0a $$(git rev-parse --short HEAD)
COMPILATION = $$(LC_ALL=C date "+%b %d %Y - %H:%M:%S %z")
CONFIG_FLAGS = $(shell [ -f .config ] && cat .config)
CFLAGS += -D__VERMILLION__="\"$(VERSION)\""
CFLAGS += -D__COMPILATION__="\"$(COMPILATION)\""
CFLAGS += $(CONFIG_FLAGS)

# Helper parameters
QEMU_MACHINE = orangepi-pc
UART_DEVICE = /dev/ttyUSB0
FLASH_DEVICE = /dev/sdf

# Dependency versions
BINUTILS = binutils-2.39
GCC = gcc-12.2.0
UBOOT = v2020.04

# Dependency parameters
ARCH = arm
ARMVER = armv7-a
FLOAT = hard
FPU = vfpv4
TARGET = $(ARCH)-none-eabihf
HOST = $(shell printf '%s\n' "$$MACHTYPE" | sed 's/-[^-]*/-cross/')
UBOOT_CONFIG = orangepi_one_defconfig
UBOOT_IMAGE = u-boot-sunxi-with-spl.bin

# -------------------------------- Functions --------------------------------- #

# Config functions
define check
	RESULT="$$(echo $(1) | gcc -E -P -xc $(CONFIG_FLAGS) -)" && \
    if [ "$$RESULT" = '1' ]; then printf 'on'; else printf 'off'; fi
endef
define flag
	$(1) $(2) $$($(call check, $(1)))
endef

# --------------------------------- Recipes  --------------------------------- #

# Helper recipes
.PHONY: all clean depclean test debug ktest kdebug uart flash config
all: build/os.img
clean:
	@printf "  RM      build\n"
	@rm -rf build
depclean:
	@printf "  RM      deps\n"
	@rm -rf deps
test: build/os.img
	@printf "  QEMU    $<\n"
	@qemu-system-arm -M $(QEMU_MACHINE) -drive file=$<,format=raw
debug: build/os.img scripts/debug.gdb
	@printf "  QEMU    $<\n"
	@qemu-system-arm -s -S -M $(QEMU_MACHINE) -drive file=$<,format=raw &
	@gdb-multiarch --command=scripts/debug.gdb
ktest: build/kernel.elf
	@printf "  QEMU    $<\n"
	@qemu-system-arm -M $(QEMU_MACHINE) -kernel $<
kdebug: build/kernel.elf scripts/kdebug.gdb
	@printf "  QEMU    $<\n"
	@qemu-system-arm -s -S -M $(QEMU_MACHINE) -kernel $< &
	@gdb-multiarch --command=scripts/kdebug.gdb
uart: $(UART_DEVICE)
	@printf "  SCREEN  $<\n"
	@sudo stty -F $< 115200 cs8 -parenb -cstopb -crtscts
	@sudo screen $< 115200
flash: build/os.img $(FLASH_DEVICE)
	@printf "  FLASH   $(FLASH_DEVICE)\n"
	@sudo dd if=$< of=$(FLASH_DEVICE) status=none
	@sudo head -c "$$(($(DISK_SIZE) * 1024 * 1024))" /dev/sdf \
     | cmp - build/os.img
config:
	@TERM=linux \
    DIALOGRC=scripts/dialog.conf \
    dialog --no-cancel --output-separator ' -D' \
           --backtitle "Vermillion configuration" \
           --title "Compilation flags" \
           --checklist "Deactivating an object makes it be compiled empty" \
           0 0 0 \
           $(call flag, CONFIG_EXTRA_BITBANG,   'Bitbang Library') \
           $(call flag, CONFIG_EXTRA_DIAGNOSIS, 'Diagnosis Library') \
           2> .config_tmp && mv .config_tmp .config
	@rm -f .config_tmp

# Folder creation
deps deps/tools build build/libc build/mount:
	@mkdir -p $@

# Generic recipes
build/%.o: src/%.c deps/.gcc | build build/libc
	@$(CC) $(CFLAGS) -c $< -o $@
	@[ -n "$$(nm -P $@ | awk '$$2 == "T"')" ] && \
    printf '%s\n' "  CC      $@"; true
build/%.a:
	@printf "  AR      $@\n"
	@chronic ar ruv $@ $^
	@printf "  RANLIB  $@\n"
	@ranlib $@

# Library definitions
build/libc.a: build/libc/assert.o build/libc/bitbang.o \
              build/libc/ctype.o build/libc/diagnosis.o \
              build/libc/signal.o build/libc/stdlib.o \
              build/libc/string.o build/libc/utils.o | build build/libc

# Specific recipes
build/boot.o: boot.S deps/.gcc | build
	@printf "  AS      $@\n"
	@$(CC) $(CFLAGS) -c $< -o $@
build/kernel.elf: scripts/linker.ld build/libc.a \
                  build/main.o build/boot.o | build
	@printf "  LD      $@\n"
	@$(CC) $(CFLAGS) -T $< build/boot.o build/main.o -o $@ -Lbuild -lc -lgcc
build/kernel.bin: build/kernel.elf | build
	@printf "  OBJCOPY $@\n"
	@$(TARGET)-objcopy $< -O binary $@
build/boot.scr: scripts/u-boot.cmd | build
	@printf "  MKIMAGE $@\n"
	@chronic mkimage -C none -A arm -T script -d $< $@
build/os.img: deps/u-boot.bin build/kernel.bin \
              build/boot.scr | build/mount
	@printf "  BUILD   $@\n"
	@dd if=/dev/zero of=$@ bs=1M count=$(DISK_SIZE) status=none
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

# Binutils compilation
deps/.binutils: deps/.binutils-step4
	touch $@
deps/binutils.tar.xz: | deps
	cd $| && wget https://ftp.gnu.org/gnu/binutils/$(BINUTILS).tar.xz
	mv $|/$(BINUTILS).tar.xz $@
deps/binutils: deps/binutils.tar.xz | deps
	cd $| && tar -xf binutils.tar.xz
	mv $|/$(BINUTILS) $@
deps/binutils-build: | deps/tools deps/binutils
	mkdir -p $@
deps/.binutils-step1: | deps/binutils-build
	cd $| && ../binutils/configure \
                        --prefix="$(abspath deps/tools)" \
                        --target="$(TARGET)" \
                        --with-sysroot="$(abspath deps/tools/$(TARGET))" \
                        --disable-nls --disable-multilib
	touch $@
deps/.binutils-step2: deps/.binutils-step1 | deps/binutils-build
	cd $| && make -j "$$(nproc)" configure-host
	touch $@
deps/.binutils-step3: deps/.binutils-step2 | deps/binutils-build
	cd $| && make -j "$$(nproc)"
	touch $@
deps/.binutils-step4: deps/.binutils-step3 | deps/binutils-build
	cd $| && make -j "$$(nproc)" install
	touch $@

# Gcc compilation
deps/.gcc: deps/.binutils deps/.gcc-step4
	touch $@
deps/gcc.tar.xz: | deps
	cd $| && wget https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC).tar.xz
	mv $|/$(GCC).tar.xz $@
deps/gcc: deps/gcc.tar.xz | deps
	cd $| && tar -xf gcc.tar.xz
	mv $|/$(GCC) $@
deps/gcc-build: | deps/tools deps/gcc
	mkdir -p $@
deps/.gcc-step1: | deps/gcc
	cd $| && ./contrib/download_prerequisites
	touch $@
deps/.gcc-step2: deps/.gcc-step1 | deps/gcc-build
	cd $| && ../gcc/configure --prefix="$(abspath deps/tools)" \
                     --build="$(HOST)" --host="$(HOST)" --target="$(TARGET)" \
                     --with-sysroot="$(abspath deps/tools/$(TARGET))" \
                     --disable-nls --disable-shared \
                     --without-headers --with-newlib --disable-decimal-float \
                     --disable-libgomp --disable-libmudflap \
                     --disable-libssp --disable-libatomic \
                     --disable-libquadmath --disable-threads \
                     --enable-languages=c --disable-multilib \
                     --with-arch="$(ARMVER)" \
                     --with-float="$(FLOAT)" --with-fpu="$(FPU)"
	touch $@
deps/.gcc-step3: deps/.gcc-step2 | deps/gcc-build
	cd $| && make -j "$$(nproc)" all-gcc all-target-libgcc
	touch $@
deps/.gcc-step4: deps/.gcc-step3 | deps/gcc-build
	cd $| && make -j "$$(nproc)" install-gcc install-target-libgcc
	touch $@

# U-boot compilation
deps/u-boot.bin: deps/.gcc deps/.u-boot-step4
deps/u-boot: | deps
	cd $| && git clone https://gitlab.denx.de/u-boot/u-boot.git
deps/.u-boot-step1: | deps/u-boot
	cd $| && git checkout tags/$(UBOOT)
	touch $@
deps/.u-boot-step2: deps/.u-boot-step1 | deps/u-boot
	cd $| && make ARCH=$(ARCH) CROSS_COMPILE=$(TARGET)- $(UBOOT_CONFIG)
	[ -f scripts/u-boot.patch ] && patch deps/u-boot/.config < \
                                         scripts/u-boot.patch
	touch $@
deps/.u-boot-step3: deps/.u-boot-step2 | deps/u-boot
	cd $| && make ARCH=$(ARCH) CROSS_COMPILE=$(TARGET)-
	touch $@
deps/.u-boot-step4: deps/.u-boot-step3 | deps/u-boot
	cp $|/$(UBOOT_IMAGE) deps/u-boot.bin
	touch $@
