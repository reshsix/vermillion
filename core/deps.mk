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

CONFIG ?= .config

-include $(CONFIG)
export PATH += :$(shell pwd)/deps/tools/bin

# -------------------------------- Parameters -------------------------------- #

ARCH = $(shell echo $(CONFIG_ARCH))
TARGET = $(shell echo $(CONFIG_TARGET))

# Dependency versions
BINUTILS = binutils-2.41
GCC = gcc-13.2.0

# Dependency parameters
HOST = $(shell printf '%s\n' "$$MACHTYPE" | sed 's/-[^-]*/-cross/')

# --------------------------------- Recipes  --------------------------------- #

.PHONY: all objs clean
all: objs
clean:
	rm -rf deps

# Folder creation
FOLDERS := deps deps/tools
$(FOLDERS):
	@mkdir -p $@

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

# --------------------------------- Objects  --------------------------------- #

OBJS = deps/.$(TARGET)-binutils deps/.$(TARGET)-gcc
-include arch/$(ARCH)/deps.mk
objs: $(OBJS)
