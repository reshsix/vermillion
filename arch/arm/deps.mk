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

UBOOT_IMAGE = $(shell echo $(CONFIG_UBOOT_IMAGE))
UBOOT_CONFIG = $(shell echo $(CONFIG_UBOOT_CONFIG))

# Dependency versions
UBOOT = v2020.04

# --------------------------------- Recipes  --------------------------------- #

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

# --------------------------------- Objects  --------------------------------- #

OBJS += deps/u-boot.bin
