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
    CFLAGS = -O0 -ggdb3
endif
CC = $(TARGET)-gcc
LD = $(TARGET)-ld
CFLAGS += -I. -Iinclude
CFLAGS += -std=c99 -nostdlib -ffreestanding
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

# Dependency versions
UBOOT = v2020.04

# Dependency parameters
UBOOT_IMAGE  = $(shell echo $(CONFIG_UBOOT_IMAGE))
UBOOT_CONFIG = $(shell echo $(CONFIG_UBOOT_CONFIG))

# Helper parameters
QEMU_MACHINE = $(shell echo $(CONFIG_QEMU_MACHINE))
UART_DEVICE ?= /dev/ttyUSB0

# --------------------------------- Objects  --------------------------------- #

OBJS := boot.o

PREFIX = src/general
OBJS += $(PREFIX)/mem.o $(PREFIX)/str.o $(PREFIX)/path.o

PREFIX = src/hal
OBJS += $(PREFIX)/block.o $(PREFIX)/stream.o

PREFIX = src/hal/classes
OBJS += $(PREFIX)/fs.o $(PREFIX)/gpio.o $(PREFIX)/pic.o \
        $(PREFIX)/timer.o $(PREFIX)/uart.o

PREFIX = src/system
OBJS += $(PREFIX)/comm.o $(PREFIX)/disk.o $(PREFIX)/time.o

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

OBJS := $(addprefix $(BUILD)/, $(OBJS))

# --------------------------------- Programs --------------------------------- #

PREFIX = root/prog

PROGS = $(PREFIX)/shell.elf $(PREFIX)/list.elf
PROGS += $(PREFIX)/create.elf $(PREFIX)/remove.elf
PROGS += $(PREFIX)/stat.elf $(PREFIX)/show.elf
PROGS += $(PREFIX)/copy.elf

PROGS := $(addprefix $(BUILD)/, $(PROGS))

# --------------------------------- Recipes  --------------------------------- #

DEPS = deps
ROOT = $(BUILD)/root
BOOT = $(ROOT)/boot

# Helper recipes
.PHONY: all image clean debug test uart
all: image
image: $(BUILD)/vermillion.img
clean:
	@printf '%s\n' "  RM      $(shell basename $(BUILD))"
	@rm -rf $(BUILD)
debug: $(BUILD)/vermillion.img scripts/debug.gdb
	@printf '%s\n' "  QEMU    $(<:$(BUILD)/%=%)"
	@qemu-system-arm -s -S -M $(QEMU_MACHINE) -drive file=$<,format=raw &
	@gdb-multiarch --command=scripts/debug.gdb
test: $(BUILD)/vermillion.img
	@printf '%s\n' "  TEST    $(<:$(BUILD)/%=%)"
	@qemu-system-arm -M $(QEMU_MACHINE) \
		-nographic -serial mon:stdio -drive file=$<,format=raw
uart: $(UART_DEVICE)
	@printf '%s\n' "  SCREEN  $<"
	@sudo stty -F $< 115200 cs8 -parenb -cstopb -crtscts
	@sudo screen $< 115200

# Folder creation
FOLDERS += $(DEPS)
FOLDERS += $(BUILD)
FOLDERS += $(BUILD)/src
FOLDERS += $(BUILD)/src/general
FOLDERS += $(BUILD)/src/hal/generic
FOLDERS += $(BUILD)/src/hal/classes
FOLDERS += $(BUILD)/src/system
FOLDERS += $(BUILD)/drivers
FOLDERS += $(BUILD)/drivers/fs
FOLDERS += $(BUILD)/drivers/arm
FOLDERS += $(BUILD)/drivers/arm/sunxi
FOLDERS += $(ROOT)
FOLDERS += $(ROOT)/prog
FOLDERS += $(BOOT)
$(FOLDERS):
	@mkdir -p $@

# U-boot compilation
$(DEPS)/$(UBOOT_CONFIG)_u-boot.bin: $(DEPS)/.$(UBOOT_CONFIG)_u-boot-step3
$(DEPS)/u-boot: | deps
	cd $| && git clone https://gitlab.denx.de/u-boot/u-boot.git
$(DEPS)/.u-boot-step1: | $(DEPS)/u-boot
	cd $| && git checkout tags/$(UBOOT)
	touch $@
$(DEPS)/.$(UBOOT_CONFIG)_u-boot-step1: $(DEPS)/.u-boot-step1 | $(DEPS)/u-boot
	cd $| && make ARCH=$(ARCH) CROSS_COMPILE=$(TARGET)- $(UBOOT_CONFIG)
	touch $@
$(DEPS)/.$(UBOOT_CONFIG)_u-boot-step2: $(DEPS)/.$(UBOOT_CONFIG)_u-boot-step1 | \
                                    $(DEPS)/u-boot
	cd $| && make ARCH=$(ARCH) CROSS_COMPILE=$(TARGET)-
	touch $@
$(DEPS)/.$(UBOOT_CONFIG)_u-boot-step3: $(DEPS)/.$(UBOOT_CONFIG)_u-boot-step2 | \
                                    $(DEPS)/u-boot
	cp $|/$(UBOOT_IMAGE) $(DEPS)/$(UBOOT_CONFIG)_u-boot.bin
	touch $@

# Generic recipes

%_defconfig: config/%_defconfig
	@cp $< $(CONFIG)
$(BUILD)/%.o: %.c | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(ROOT)/prog/%.elf: prog/%.c | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(ROOT)/%=%)"
	@$(CC) $(CFLAGS) -shared -fPIE -fPIC -Wl,-evrm_prog -Wl,-z,defs $< -o $@

# Specific recipes

$(BUILD)/devtree.o: $(shell echo $(CONFIG_DEVICE_TREE))
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/boot.o: boot/boot.S | $(BUILD)
	@printf '%s\n' "  AS      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/u-boot.cmd: boot/u-boot.cmd | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -xc $< -E -P | grep -v '^#' > $@
$(BOOT)/boot.scr: $(BUILD)/u-boot.cmd | $(BUILD)
	@printf '%s\n' "  MKIMAGE $(@:$(ROOT)/%=%)"
	@chronic mkimage -C none -A $(ARCH) -T script -d $< $@

$(BUILD)/linker.ld: boot/linker.ld | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -xc $< -E -P | grep -v '^#' > $@
$(BUILD)/kernel.elf: $(BUILD)/linker.ld $(OBJS)
	@printf '%s\n' "  LD      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -T $^ -o $@ -lgcc
$(BOOT)/kernel.bin: $(BUILD)/kernel.elf | $(BUILD)
	@printf '%s\n' "  OBJCOPY $(@:$(ROOT)/%=%)"
	@$(TARGET)-objcopy $< -O binary $@

# Image creation

DISK_SIZE=$(shell echo "x=l(($$(du -b root | tail -n1 | cut -f1) \
                            /1000000) + 16)/l(2); \
            scale=0; 2^((x+1)/1)" | bc -l)
$(BUILD)/vermillion.img: $(BOOT)/kernel.bin $(BOOT)/boot.scr $(PROGS)
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
	@sudo umount $(BUILD)/mount
	@sudo losetup -d /dev/loop0
image: $(BUILD)/vermillion.img $(DEPS)/$(UBOOT_CONFIG)_u-boot.bin
	@sudo losetup /dev/loop0 $<
	@printf '%s\n' "  UBOOT   $(<:$(BUILD)/%=%)"
	@sudo dd if=$(DEPS)/$(UBOOT_CONFIG)_u-boot.bin \
             of=/dev/loop0 bs=1024 seek=8 status=none
	@sleep 1
	@sudo losetup -d /dev/loop0
