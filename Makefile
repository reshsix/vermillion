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

BUILD  ?= build
CONFIG ?= .config

-include $(CONFIG)

# -------------------------------- Parameters -------------------------------- #

# Compilation parameters
ifeq ($(NDEBUG), 1)
    CFLAGS = -O2 -DNDEBUG=1
else
    CFLAGS = -O0 -ggdb3
endif
CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
CFLAGS += -I. -Iinclude -Isrc -std=c99
CFLAGS += -Wall -Wextra -Wno-attributes
CFLAGS += $(shell echo $(CONFIG_CFLAGS))

# Extra variables
VERSION = 1.2a $$(git rev-parse --short HEAD)
COMPILATION = $$(LC_ALL=C date "+%b %d %Y - %H:%M:%S %z")
KCONFIG_FLAGS = $(shell [ -f $(CONFIG) ] && cat $(CONFIG) \
                  | sed '/^$$/d; /^#/d; s/^/ -D/' | tr '\n' ' ')
CFLAGS += -D__VERMILLION__="\"$(VERSION)\""
CFLAGS += -D__COMPILATION__="\"$(COMPILATION)\""
CFLAGS += $(KCONFIG_FLAGS)

# --------------------------------- Objects  --------------------------------- #

OBJS := boot.o devtree.o

PREFIX = src/arch
ifdef CONFIG_ARM_GIC
OBJS += $(PREFIX)/gic.o
endif

PREFIX = src/util
OBJS += $(PREFIX)/debug.o $(PREFIX)/mem.o $(PREFIX)/str.o

PREFIX = src/hal
OBJS += $(PREFIX)/uart.o  $(PREFIX)/spi.o \
		$(PREFIX)/timer.o $(PREFIX)/gpio.o $(PREFIX)/disk.o

PREFIX = src/sys
OBJS += $(PREFIX)/file.o

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

ifdef CONFIG_SPI_SUNXI_SPI
OBJS += $(PREFIX)/spi.o
endif

ifdef CONFIG_TIMER_SUNXI_TIMER
OBJS += $(PREFIX)/timer.o
endif

ifdef CONFIG_SERIAL_SUNXI_UART
OBJS += $(PREFIX)/uart.o
endif

OBJS := $(addprefix $(BUILD)/, $(OBJS))

# --------------------------------- Recipes  --------------------------------- #

# Helper recipes
.PHONY: all clean install uninstall
all: libvrm_$(CONFIG_NAME).a $(CONFIG_NAME).ld
clean:
	@printf '%s\n' "  RM      $(shell basename $(BUILD))"
	@printf '%s\n' "  RM      libvrm_$(CONFIG_NAME).a"
	@printf '%s\n' "  RM      $(CONFIG_NAME).ld"
	@rm -rf $(BUILD) libvrm_$(CONFIG_NAME).a $(CONFIG_NAME).ld

DESTDIR=/usr/local/
install: include/vermillion
	@cp -r $< "$(DESTDIR)/include/vermillion"
uninstall:
	@rm -rf $< "$(DESTDIR)/include/vermillion"

# Folder creation
FOLDERS += $(BUILD)
FOLDERS += $(BUILD)/src
FOLDERS += $(BUILD)/src/arch
FOLDERS += $(BUILD)/src/hal
FOLDERS += $(BUILD)/src/sys
FOLDERS += $(BUILD)/src/util
FOLDERS += $(BUILD)/drivers
FOLDERS += $(BUILD)/drivers/fs
FOLDERS += $(BUILD)/drivers/arm
FOLDERS += $(BUILD)/drivers/arm/sunxi
$(FOLDERS):
	@mkdir -p $@

# Generic recipes

%_defconfig: config/%_defconfig
	@cp $< $(CONFIG)
$(BUILD)/%.o: %.c | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@

# Specific recipes

$(BUILD)/devtree.o: devtree/$(shell echo $(CONFIG_NAME)).c
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@
$(BUILD)/boot.o: boot/boot.S | $(BUILD)
	@printf '%s\n' "  AS      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -c $< -o $@

$(CONFIG_NAME).ld: boot/linker.ld | $(FOLDERS)
	@printf '%s\n' "  CC      $(@:$(BUILD)/%=%)"
	@$(CC) $(CFLAGS) -xc $< -E -P | grep -v '^#' > $@
libvrm_$(CONFIG_NAME).a: $(OBJS) | $(FOLDERS)
	@printf '%s\n' "  AR      $(@:$(BUILD)/%=%)"
	@ar cr $@ $^
	@printf '%s\n' "  RANLIB  $(@:$(BUILD)/%=%)"
	@ranlib $@
	@printf '  CFLAGS  %s\n' "$(shell echo $(CONFIG_CFLAGS))"
