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

-include .config
export PATH += :$(shell pwd)/deps/tools/bin

# -------------------------------- Parameters -------------------------------- #

ARCH = $(shell echo $(CONFIG_ARCH))
TARGET = $(shell echo $(CONFIG_TARGET))

# Compilation parameters
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
CFLAGS += -O2 -ggdb3
CFLAGS += -I. -Iinclude -Iarch/$(ARCH)/include
CFLAGS += -std=gnu99 -nostdlib -ffreestanding
CFLAGS += -Wall -Wextra -Wno-attributes
CFLAGS += $(shell echo $(CONFIG_CFLAGS_ARCH))
CFLAGS += $(shell echo $(CONFIG_CFLAGS_BOARD))

# Extra variables
VERSION = 0.0a $$(git rev-parse --short HEAD)
COMPILATION = $$(LC_ALL=C date "+%b %d %Y - %H:%M:%S %z")
KCONFIG_FLAGS = $(shell [ -f .config ] && cat .config \
                  | sed '/^$$/d; /^#/d; s/^/ -D/' | tr '\n' ' ')
CFLAGS += -D__VERMILLION__="\"$(VERSION)\""
CFLAGS += -D__COMPILATION__="\"$(COMPILATION)\""
CFLAGS += $(KCONFIG_FLAGS)

# Helper parameters
UART_DEVICE ?= /dev/ttyUSB0

# Dependency versions
BINUTILS = binutils-2.39
GCC = gcc-12.2.0

# Dependency parameters
HOST = $(shell printf '%s\n' "$$MACHTYPE" | sed 's/-[^-]*/-cross/')

# --------------------------------- Recipes  --------------------------------- #

# Helper recipes
.PHONY: all image clean depclean test debug ktest kdebug uart \
        config menuconfig xconfig
all: image
clean:
	@printf "  RM      build\n"
	@rm -rf build
depclean:
	@printf "  RM      deps\n"
	@rm -rf deps
uart: $(UART_DEVICE)
	@printf "  SCREEN  $<\n"
	@sudo stty -F $< 115200 cs8 -parenb -cstopb -crtscts
	@sudo screen $< 115200
config:
	@kconfig-conf Kconfig
	@rm -rf build
menuconfig:
	@kconfig-mconf Kconfig
	@rm -rf build
xconfig:
	@kconfig-qconf Kconfig
	@rm -rf build

# Folder creation
FOLDERS = deps deps/tools build build/libc build/arch build/mount
FOLDERS += build/drivers build/drivers/base
FOLDERS += build/drivers/arm build/drivers/i686
FOLDERS += build/drivers/arm/sunxi
$(FOLDERS):
	@mkdir -p $@

# Generic recipes
build/%.o: src/%.c deps/.$(TARGET)-gcc | $(FOLDERS)
	@printf '%s\n' "  CC      $@"; true
	@$(CC) $(CFLAGS) -c $< -o $@
build/%.a: deps/.$(TARGET)-binutils
	@printf "  AR      $@\n"
	@chronic $(TARGET)-ar ruv $@ $^
	@printf "  RANLIB  $@\n"
	@$(TARGET)-ranlib $@
build/arch/%: arch/$(ARCH)/% deps/.$(TARGET)-gcc | $(FOLDERS)
	@printf "  CC      $@\n"
	@$(CC) $(CFLAGS) -xc $< -E -P | grep -v '^#' > $@
%_defconfig: config/%_defconfig
	@rm -rf build
	@cp $< .config
	@kconfig-conf --olddefconfig Kconfig

# Objects definitions
CORE = build/loader.o build/interrupts.o build/drivers.o \
       build/devtree.o build/boot.o build/main.o
include src/libc/make.list
LIBC := $(addprefix build/libc/, $(LIBC))
include src/drivers/make.list
DRIVERS := $(addprefix build/drivers/, $(DRIVERS))

# Specific recipes
build/devtree.o: $(shell echo $(CONFIG_DEVICE_TREE))
	@printf '%s\n' "  CC      $@"; true
	@$(CC) $(CFLAGS) -c $< -o $@
ifdef CONFIG_LOADER_EMBED
build/init.o: $(shell echo $(CONFIG_LOADER_FILE))
	@printf "  LD      $@\n"
	@$(LD) -r -b binary -o $@ $<
CORE += build/init.o
endif
build/kernel.elf: build/arch/linker.ld $(LIBC) $(DRIVERS) $(CORE)
	@printf "  LD      $@\n"
	@$(CC) $(CFLAGS) -T $^ -o $@ -lgcc
build/kernel.bin: build/kernel.elf | build
	@printf "  OBJCOPY $@\n"
	@$(TARGET)-objcopy $< -O binary $@

# ------------------------------- Dependencies ------------------------------- #

# Binutils compilation
deps/.$(TARGET)-binutils: deps/.$(TARGET)-binutils-step4
	touch $@
deps/binutils.tar.xz: | deps
	cd $| && wget https://ftp.gnu.org/gnu/binutils/$(BINUTILS).tar.xz
	mv $|/$(BINUTILS).tar.xz $@
deps/binutils: deps/binutils.tar.xz | deps
	cd $| && tar -xf binutils.tar.xz
	mv $|/$(BINUTILS) $@
deps/$(TARGET)-binutils: | deps/tools deps/binutils
	mkdir -p $@
deps/.$(TARGET)-binutils-step1: | deps/$(TARGET)-binutils
	cd $| && ../binutils/configure \
                 --prefix="$(abspath deps/tools)" \
                 --target="$(TARGET)" \
                 --with-sysroot="$(abspath deps/tools/$(TARGET))" \
                 --disable-nls --disable-multilib
	touch $@
deps/.$(TARGET)-binutils-step2: deps/.$(TARGET)-binutils-step1 | \
                                deps/$(TARGET)-binutils
	cd $| && make -j "$$(nproc)" configure-host
	touch $@
deps/.$(TARGET)-binutils-step3: deps/.$(TARGET)-binutils-step2 | \
                                deps/$(TARGET)-binutils
	cd $| && make -j "$$(nproc)"
	touch $@
deps/.$(TARGET)-binutils-step4: deps/.$(TARGET)-binutils-step3 | \
                                deps/$(TARGET)-binutils
	cd $| && make -j "$$(nproc)" install
	touch $@

# Gcc compilation
deps/.$(TARGET)-gcc: deps/.$(TARGET)-binutils deps/.$(TARGET)-gcc-step4
	touch $@
deps/gcc.tar.xz: | deps
	cd $| && wget https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC).tar.xz
	mv $|/$(GCC).tar.xz $@
deps/gcc: deps/gcc.tar.xz | deps
	cd $| && tar -xf gcc.tar.xz
	mv $|/$(GCC) $@
deps/$(TARGET)-gcc: | deps/tools deps/gcc
	mkdir -p $@
deps/.$(TARGET)-gcc-step1: | deps/gcc
	cd $| && ./contrib/download_prerequisites
	touch $@
deps/.$(TARGET)-gcc-step2: deps/.$(TARGET)-gcc-step1 | deps/$(TARGET)-gcc
	cd $| && ../gcc/configure --prefix="$(abspath deps/tools)" \
                 --build="$(HOST)" --host="$(HOST)" --target="$(TARGET)" \
                 --with-sysroot="$(abspath deps/tools/$(TARGET))" \
                 --disable-nls --disable-shared \
                 --without-headers --with-newlib --disable-decimal-float \
                 --disable-libgomp --disable-libmudflap \
                 --disable-libssp --disable-libatomic \
                 --disable-libquadmath --disable-threads \
                 --enable-languages=c --disable-multilib
	touch $@
deps/.$(TARGET)-gcc-step3: deps/.$(TARGET)-gcc-step2 | deps/$(TARGET)-gcc
	cd $| && make -j "$$(nproc)" all-gcc all-target-libgcc
	touch $@
deps/.$(TARGET)-gcc-step4: deps/.$(TARGET)-gcc-step3 | deps/$(TARGET)-gcc
	cd $| && make -j "$$(nproc)" install-gcc install-target-libgcc
	touch $@

ifdef CONFIG_ARCH_ARM
include arch/arm/Makefile
endif
ifdef CONFIG_ARCH_I686
include arch/i686/Makefile
endif
