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
.PHONY: all objs image clean debug uart \
        config menuconfig xconfig
all: image
clean:
	@printf '%s\n' "  RM      $(shell basename $(BUILD))"
	@rm -rf $(BUILD)
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
FOLDERS := $(BUILD) $(BUILD)/libc $(BUILD)/arch $(BUILD)/mount
FOLDERS += $(BUILD)/drivers $(BUILD)/drivers/audio
FOLDERS += $(BUILD)/drivers/video $(BUILD)/drivers/fs
FOLDERS += $(BUILD)/drivers/generic $(BUILD)/drivers/protocols
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

# --------------------------------- Objects  --------------------------------- #

OBJS :=

OBJS += devtree.o
OBJS += libk.o

PREFIX = drivers/audio

ifdef CONFIG_AUDIO_BUZZER
OBJS += $(PREFIX)/buzzer.o
endif

PREFIX = drivers/video

ifdef CONFIG_VIDEO_ILI9488
OBJS += $(PREFIX)/ili9488.o
endif

PREFIX = drivers/fs

ifdef CONFIG_FS_FAT32_MBR
OBJS += $(PREFIX)/mbr.o
OBJS += $(PREFIX)/fat32.o
endif

PREFIX = drivers/protocol

ifdef CONFIG_PROTOCOL_SPI_SOFT
OBJS += $(PREFIX)/spi_soft.o
endif

PREFIX = drivers/generic
OBJS += $(PREFIX)/memory.o

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

PREFIX = drivers/i686

ifdef CONFIG_SERIAL_I686
OBJS += $(PREFIX)/serial.o
endif

-include arch/$(ARCH)/core.mk
OBJS := $(addprefix $(BUILD)/, $(OBJS))

# --------------------------------- Recipes  --------------------------------- #

objs: $(OBJS)

# Specific recipes
$(BUILD)/devtree.o: $(shell echo $(CONFIG_DEVICE_TREE))
	@printf '%s\n' "  CC      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
ifneq ($(wildcard arch/$(ARCH)/linker.ld),)
$(BUILD)/kernel.elf: $(BUILD)/arch/linker.ld $(EXTERN) $(OBJS)
	@printf '%s\n' "  LD      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -T $^ -o $@ -lgcc
else
$(BUILD)/kernel.elf: $(EXTERN) $(OBJS)
	@printf '%s\n' "  LD      core/$(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) $^ -o $@ -lgcc
endif
$(BUILD)/kernel.bin: $(BUILD)/kernel.elf | $(BUILD)
	@printf '%s\n' "  OBJCOPY core/$(@:$(BUILD)/%=%)"
	@$(TARGET)-objcopy $< -O binary $@
