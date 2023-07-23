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

BUILD ?= build
CONFIG ?= .config

-include $(CONFIG)
export PATH += :$(shell pwd)/deps/tools/bin

# -------------------------------- Parameters -------------------------------- #

ARCH = $(shell echo $(CONFIG_ARCH))
TARGET = $(shell echo $(CONFIG_TARGET))

# Compilation parameters
ifeq ($(NDEBUG), 1)
    CFLAGS = -O2 -DNDEBUG=1
else
    CFLAGS = -O0 -ggdb3
endif
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
CFLAGS += -I. -Iinclude -Iarch/$(ARCH)/include
CFLAGS += -std=gnu99 -nostdlib -ffreestanding
CFLAGS += -Wall -Wextra -Wno-attributes
CFLAGS += $(shell echo $(CONFIG_CFLAGS_ARCH))
CFLAGS += $(shell echo $(CONFIG_CFLAGS_BOARD))

# Extra variables
VERSION = 0.0a $$(git rev-parse --short HEAD)
COMPILATION = $$(LC_ALL=C date "+%b %d %Y - %H:%M:%S %z")
KCONFIG_FLAGS = $(shell [ -f $(CONFIG) ] && cat $(CONFIG) \
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
.PHONY: all image clean tools depclean debug uart \
        config menuconfig xconfig
all: image
clean:
	@printf '%s\n' "  RM      $(shell basename $(BUILD))"
	@rm -rf $(BUILD)
depclean:
	@printf '%s\n' "  RM      deps"
	@rm -rf deps
uart: $(UART_DEVICE)
	@printf '%s\n' "  SCREEN  $<"
	@sudo stty -F $< 115200 cs8 -parenb -cstopb -crtscts
	@sudo screen $< 115200
config:
	@KCONFIG_CONFIG="$(CONFIG)" kconfig-conf Kconfig
	@rm -rf $(BUILD) .config.old
menuconfig:
	@KCONFIG_CONFIG="$(CONFIG)" kconfig-mconf Kconfig
	@rm -rf $(BUILD) .config.old
xconfig:
	@KCONFIG_CONFIG="$(CONFIG)" kconfig-qconf Kconfig
	@rm -rf $(BUILD) .config.old
defconfig: config/$(BOARD)_defconfig
	@cp $< $(CONFIG)
	@KCONFIG_CONFIG="$(CONFIG)" kconfig-conf --olddefconfig Kconfig
	@rm -rf $(BUILD) .config.old

# Folder creation
FOLDERS = deps deps/tools $(BUILD) $(BUILD)/libc $(BUILD)/arch $(BUILD)/mount
FOLDERS += $(BUILD)/drivers $(BUILD)/drivers/base
FOLDERS += $(BUILD)/drivers/arm $(BUILD)/drivers/i686
FOLDERS += $(BUILD)/drivers/arm/sunxi
$(FOLDERS):
	@mkdir -p $@

# Generic recipes
$(BUILD)/%.o: src/%.c deps/.$(TARGET)-gcc | $(FOLDERS)
	@printf '%s\n' "  CC      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/%.a: deps/.$(TARGET)-binutils
	@printf '%s\n' "  AR      core/$(@:$(BUILD)/%=%)"
	@chronic $(TARGET)-ar ruv $@ $^
	@printf '%s\n' "  RANLIB  core/$(@:$(BUILD)/%=%)"
	@$(TARGET)-ranlib $@
$(BUILD)/arch/%: arch/$(ARCH)/% deps/.$(TARGET)-gcc | $(FOLDERS)
	@printf '%s\n' "  CC      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -xc $< -E -P | grep -v '^#' > $@
%_defconfig: config/%_defconfig
	@cp $< $(CONFIG)
	@KCONFIG_CONFIG="$(CONFIG)" kconfig-conf --olddefconfig Kconfig
	@rm -rf $(BUILD) .config.old

# Objects definitions
CORE = $(BUILD)/interrupts.o $(BUILD)/drivers.o \
       $(BUILD)/devtree.o $(BUILD)/boot.o
include src/libc/make.list
LIBC := $(addprefix $(BUILD)/libc/, $(LIBC))
include src/drivers/make.list
DRIVERS := $(addprefix $(BUILD)/drivers/, $(DRIVERS))

# Specific recipes
$(BUILD)/devtree.o: $(shell echo $(CONFIG_DEVICE_TREE))
	@printf '%s\n' "  CC      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/kernel.elf: $(BUILD)/arch/linker.ld $(EXTERN) \
                     $(LIBC) $(DRIVERS) $(CORE)
	@printf '%s\n' "  LD      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -T $^ -o $@ -lgcc
$(BUILD)/kernel.bin: $(BUILD)/kernel.elf | $(BUILD)
	@printf '%s\n' "  OBJCOPY core/$(@:$(BUILD)/%=%)"
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
