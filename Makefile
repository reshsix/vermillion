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

ARCH = arm
ARMVER = armv7-a
FLOAT = hard
FPU = vfpv4
HOST = $(shell printf '%s\n' "$$MACHTYPE" | sed 's/-[^-]*/-cross/')
TARGET = $(ARCH)-none-eabihf

BINUTILS = binutils-2.39
GCC = gcc-12.2.0

CC = $(TARGET)-gcc
CFLAGS += -Og -ggdb3
CFLAGS += -I. -std=gnu99 -fpic -nostdlib -ffreestanding -mcpu=cortex-a7
CFLAGS += -Wall -Wextra

DISK_SIZE = 4
UBOOT_CONFIG = orangepi_one_defconfig
UBOOT_IMAGE = u-boot-sunxi-with-spl.bin
QEMU_MACHINE = orangepi-pc

.PHONY: all clean depclean test debug ktest kdebug uart

all: build/os.img

clean:
	rm -rf build
depclean:
	rm -rf deps

test: build/os.img
	qemu-system-arm -M $(QEMU_MACHINE) -drive file=$<,format=raw
debug: build/os.img scripts/debug.gdb
	qemu-system-arm -s -S -M $(QEMU_MACHINE) -drive file=$<,format=raw &
	gdb-multiarch --command=scripts/debug.gdb
ktest: build/kernel.elf
	qemu-system-arm -M $(QEMU_MACHINE) -kernel $<
kdebug: build/kernel.elf scripts/kdebug.gdb
	qemu-system-arm -s -S -M $(QEMU_MACHINE) -kernel $< &
	gdb-multiarch --command=scripts/kdebug.gdb

uart:
	sudo stty -F /dev/ttyUSB0 115200 cs8 -parenb -cstopb -crtscts
	sudo screen /dev/ttyUSB0 115200

build/boot.o: boot.S deps/.gcc | build
	$(CC) $(CFLAGS) -c $< -o $@
build/main.o: main.c deps/.gcc | build
	$(CC) $(CFLAGS) -c $< -o $@
build/kernel.elf: scripts/linker.ld build/boot.o build/main.o | build
	$(CC) $(CFLAGS) -T $^ -o $@ -lgcc
build/kernel.bin: build/kernel.elf | build
	$(TARGET)-objcopy $< -O binary $@

build/boot.scr: scripts/u-boot.cmd | build
	chronic mkimage -C none -A arm -T script -d $< $@
build/os.img: deps/u-boot.bin build/kernel.bin \
              build/boot.scr | build/mount
	dd if=/dev/zero of=$@ bs=1M count=$(DISK_SIZE) status=none
	sudo losetup /dev/loop0 $@
	printf 'start=2048, type=83, bootable\n' | sudo chronic sfdisk -q /dev/loop0
	sudo partx -a /dev/loop0
	sudo mkfs.ext2 "/dev/loop0p1"
	sudo mount /dev/loop0p1 build/mount
	sudo mkdir -p build/mount/boot/
	sudo cp build/boot.scr build/mount/
	sudo cp build/kernel.bin build/mount/boot/
	sudo umount build/mount
	sudo dd if=deps/u-boot.bin of=/dev/loop0 bs=1024 seek=8 status=none
	@sleep 1
	sudo partx -d /dev/loop0
	sudo losetup -d /dev/loop0

build:
	mkdir -p $@
build/mount: build
	mkdir -p $@

deps:
	mkdir -p $@
deps/tools: | deps
	mkdir -p $@

deps/.binutils: deps/.binutils-step4
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
deps/.binutils-step2: deps/.binutils-step1 | \
                            deps/binutils-build
	cd $| && make -j "$$(nproc)" configure-host
	touch $@
deps/.binutils-step3: deps/.binutils-step2 | \
                            deps/binutils-build
	cd $| && make -j "$$(nproc)"
	touch $@
deps/.binutils-step4: deps/.binutils-step3 | \
                            deps/binutils-build
	cd $| && make -j "$$(nproc)" install
	touch $@

deps/.gcc: deps/.binutils deps/.gcc-step4
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

deps/u-boot.bin: deps/.gcc deps/.u-boot-step4
deps/u-boot: | deps
	cd $| && git clone https://gitlab.denx.de/u-boot/u-boot.git
deps/.u-boot-step1: | deps/u-boot
	cd $| && git checkout tags/v2020.04
	touch $@
deps/.u-boot-step2: deps/.u-boot-step1 | deps/u-boot
	cd $| && make ARCH=$(ARCH) CROSS_COMPILE=$(TARGET)- $(UBOOT_CONFIG)
	touch $@
deps/.u-boot-step3: deps/.u-boot-step2 | deps/u-boot
	cd $| && make ARCH=$(ARCH) CROSS_COMPILE=$(TARGET)-
	touch $@
deps/.u-boot-step4: deps/.u-boot-step3 | deps/u-boot
	cp $|/$(UBOOT_IMAGE) deps/u-boot.bin
	touch $@
