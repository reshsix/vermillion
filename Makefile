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

# -------------------------------- Parameters -------------------------------- #

ARCH = $(shell echo $(CONFIG_ARCH))
TARGET = $(shell echo $(CONFIG_TARGET))

# Compilation parameters
ifeq ($(NDEBUG), 1)
    CFLAGS = -O2 -DNDEBUG=1
else
    CFLAGS = -O1 -ggdb3
endif
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
CFLAGS += -I. -Iinclude -Iarch/$(ARCH)/include
CFLAGS += -std=gnu2x -nostdlib -ffreestanding
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
.PHONY: all image clean debug uart
all: image
clean:
	@printf '%s\n' "  RM      $(shell basename $(BUILD))"
	@rm -rf $(BUILD)
uart: $(UART_DEVICE)
	@printf '%s\n' "  SCREEN  $<"
	@sudo stty -F $< 115200 cs8 -parenb -cstopb -crtscts
	@sudo screen $< 115200

# Folder creation
FOLDERS += deps
FOLDERS += $(BUILD) $(BUILD)/arch $(BUILD)/src
FOLDERS += $(BUILD)/src/general
FOLDERS += $(BUILD)/src/hal/generic
FOLDERS += $(BUILD)/src/hal/classes
FOLDERS += $(BUILD)/src/system
FOLDERS += $(BUILD)/drivers
FOLDERS += $(BUILD)/drivers/fs
FOLDERS += $(BUILD)/drivers/arm
FOLDERS += $(BUILD)/drivers/arm/sunxi
$(FOLDERS):
	@mkdir -p $@

# Image creation
DISK_SIZE=$(shell echo "x=l(($$(du -b root | tail -n1 | cut -f1) \
                            /1000000) + 16)/l(2); \
            scale=0; 2^((x+1)/1)" | bc -l)
$(BUILD)/vermillion.img: build/root root/
	@printf '%s\n' "  BUILD   $(@:$(BUILD)/%=%)"
	@dd if=/dev/zero of=$@ bs=1M count=$(DISK_SIZE) status=none
	@sudo losetup /dev/loop0 $@
	@printf 'start=2048, type=0c, bootable\n' \
     | sudo chronic sfdisk -q /dev/loop0
	@sudo partx -a /dev/loop0
	@sudo chronic mkfs.vfat -F32 /dev/loop0p1
	@mkdir -p $(BUILD)/mount/
	@sudo mount -t vfat /dev/loop0p1 $(BUILD)/mount
	@sudo cp -r root/* $(BUILD)/mount/
	@sudo cp -r build/root/* $(BUILD)/mount/

# Generic recipes
$(BUILD)/%.o: %.c | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/%.a: | $(FOLDERS)
	@printf '%s\n' "  AR      $(@:$(BUILD)/%=%)"
	@chronic $(TARGET)-ar ruv $@ $^
	@printf '%s\n' "  RANLIB  $(@:$(BUILD)/%=%)"
	@$(TARGET)-ranlib $@
$(BUILD)/arch/%: arch/$(ARCH)/% | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -xc $< -E -P | grep -v '^#' > $@
%_defconfig: config/%_defconfig
	@cp $< $(CONFIG)

# --------------------------------- Objects  --------------------------------- #

PREFIX = src/general
OBJS := $(PREFIX)/mem.o $(PREFIX)/str.o $(PREFIX)/dict.o $(PREFIX)/path.o

PREFIX = src/hal
OBJS += $(PREFIX)/block.o $(PREFIX)/stream.o

PREFIX = src/hal/classes
OBJS += $(PREFIX)/fs.o $(PREFIX)/gpio.o $(PREFIX)/pic.o \
        $(PREFIX)/timer.o $(PREFIX)/uart.o

PREFIX = src/system
OBJS += $(PREFIX)/comm.o $(PREFIX)/disk.o $(PREFIX)/time.o $(PREFIX)/vars.o

OBJS += devtree.o src/loader.o src/syslog.o src/main.o

PREFIX = drivers/arm
ifdef CONFIG_ARM_GIC
OBJS += $(PREFIX)/gic.o
endif

PREFIX = drivers/fs

ifdef CONFIG_FS_FAT32
OBJS += $(PREFIX)/fat32.o
endif

ifdef CONFIG_FS_MBR
OBJS += $(PREFIX)/mbr.o
endif

PREFIX = drivers/arm/sunxi

ifdef CONFIG_GPIO_SUNXI_GPIO
OBJS += $(PREFIX)/gpio.o
endif

ifdef CONFIG_STORAGE_SUNXI_MMC
OBJS += $(PREFIX)/mmc.o
endif

ifdef CONFIG_TIMER_SUNXI_TIMER
OBJS += $(PREFIX)/timer.o
endif

ifdef CONFIG_SERIAL_SUNXI_UART
OBJS += $(PREFIX)/uart.o
endif

-include arch/$(ARCH)/core.mk

OBJS := $(addprefix $(BUILD)/, $(OBJS))

# --------------------------------- Recipes  --------------------------------- #

objs: $(OBJS)

# Specific recipes
$(BUILD)/devtree.o: $(shell echo $(CONFIG_DEVICE_TREE))
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
ifneq ($(wildcard arch/$(ARCH)/linker.ld),)
$(BUILD)/kernel.elf: $(BUILD)/arch/linker.ld $(OBJS)
	@printf '%s\n' "  LD      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -T $^ -o $@ -lgcc
else
$(BUILD)/kernel.elf: $(OBJS)
	@printf '%s\n' "  LD      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) $^ -o $@ -lgcc
endif
$(BUILD)/kernel.bin: $(BUILD)/kernel.elf | $(BUILD)
	@printf '%s\n' "  OBJCOPY $(@:$(BUILD)/%=%)"
	@$(TARGET)-objcopy $< -O binary $@
