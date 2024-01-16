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
BOARD ?= orangepi_one

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
FOLDERS := $(BUILD) $(BUILD)/arch $(BUILD)/mount $(BUILD)/src
FOLDERS += $(BUILD)/src/general $(BUILD)/src/environ $(BUILD)/src/thread
FOLDERS += $(BUILD)/src/hal/generic $(BUILD)/src/hal/classes
FOLDERS += $(BUILD)/src/system $(BUILD)/src/debug
FOLDERS += $(BUILD)/src/debug/test/general $(BUILD)/src/debug/test/environ
FOLDERS += $(BUILD)/src/debug/test/thread $(BUILD)/src/debug/test/system
FOLDERS += $(BUILD)/drivers $(BUILD)/drivers/audio
FOLDERS += $(BUILD)/drivers/video $(BUILD)/drivers/fs
FOLDERS += $(BUILD)/drivers/generic $(BUILD)/drivers/protocols
FOLDERS += $(BUILD)/drivers/arm $(BUILD)/drivers/i686
FOLDERS += $(BUILD)/drivers/arm/sunxi
$(FOLDERS):
	@mkdir -p $@

# Generic recipes
$(BUILD)/%.o: %.c deps/.$(TARGET)-gcc | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/%.a: deps/.$(TARGET)-binutils
	@printf '%s\n' "  AR      $(@:$(BUILD)/%=%)"
	@chronic $(TARGET)-ar ruv $@ $^
	@printf '%s\n' "  RANLIB  $(@:$(BUILD)/%=%)"
	@$(TARGET)-ranlib $@
$(BUILD)/arch/%: arch/$(ARCH)/% deps/.$(TARGET)-gcc | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -xc $< -E -P | grep -v '^#' > $@
%_defconfig: config/%_defconfig
	@cp $< $(CONFIG)
	@KCONFIG_CONFIG="$(CONFIG)" kconfig-conf --olddefconfig Kconfig
	@rm -rf $(BUILD) .config.old

# --------------------------------- Objects  --------------------------------- #

PREFIX = src/general
OBJS := $(PREFIX)/mem.o $(PREFIX)/str.o

PREFIX = src/environ
OBJS += $(PREFIX)/fork.o $(PREFIX)/generator.o $(PREFIX)/state.o

PREFIX = src/thread
OBJS += $(PREFIX)/channel.o $(PREFIX)/mutex.o \
        $(PREFIX)/semaphore.o $(PREFIX)/thread.o

PREFIX = src/hal/generic
OBJS += $(PREFIX)/block.o $(PREFIX)/stream.o

PREFIX = src/hal/classes
OBJS += $(PREFIX)/gpio.o $(PREFIX)/pic.o $(PREFIX)/spi.o $(PREFIX)/timer.o \
        $(PREFIX)/uart.o $(PREFIX)/video.o

PREFIX = src/system
OBJS += $(PREFIX)/log.o $(PREFIX)/wheel.o

PREFIX = src/debug
OBJS += $(PREFIX)/assert.o $(PREFIX)/exit.o $(PREFIX)/profile.o \
        $(PREFIX)/test/test.o

PREFIX = src/debug/test/general
OBJS += $(PREFIX)/types.o $(PREFIX)/mem.o $(PREFIX)/str.o
PREFIX = src/debug/test/environ
OBJS += $(PREFIX)/fork.o $(PREFIX)/generator.o $(PREFIX)/state.o
PREFIX = src/debug/test/thread
OBJS += $(PREFIX)/channel.o $(PREFIX)/critical.o $(PREFIX)/implicit.o \
        $(PREFIX)/mutex.o $(PREFIX)/semaphore.o $(PREFIX)/thread.o
PREFIX = src/debug/test/system
OBJS += $(PREFIX)/log.o

OBJS += src/main.o

OBJS += devtree.o

PREFIX = drivers/arm
ifdef CONFIG_ARM_GIC
OBJS += $(PREFIX)/gic.o
endif

PREFIX = drivers/audio

ifdef CONFIG_AUDIO_BUZZER
OBJS += $(PREFIX)/buzzer.o
endif

PREFIX = drivers/video

ifdef CONFIG_VIDEO_ILI9488
OBJS += $(PREFIX)/ili9488.o
endif

ifdef CONFIG_VIDEO_VIRTUAL
OBJS += $(PREFIX)/virtual.o
endif

PREFIX = drivers/fs

ifdef CONFIG_FS_FAT32_MBR
OBJS += $(PREFIX)/mbr.o
OBJS += $(PREFIX)/fat32.o
endif

PREFIX = drivers/protocols

ifdef CONFIG_PROTOCOL_SPI
OBJS += $(PREFIX)/spi.o
endif

ifdef CONFIG_PROTOCOL_SIPO
OBJS += $(PREFIX)/sipo.o
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

ifdef CONFIG_I686_PIC
OBJS += $(PREFIX)/pic.o
endif

ifdef CONFIG_TIMER_I686_TIMER
OBJS += $(PREFIX)/timer.o
endif

ifdef CONFIG_VIDEO_I686_FB
OBJS += $(PREFIX)/framebuffer.o
endif

ifdef CONFIG_SERIAL_I686
OBJS += $(PREFIX)/serial.o
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
