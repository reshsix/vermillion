# Vermillion
An embedded operating system

## Development status
Alpha, some core features are still not implemented.
Will support Orange Pi One (armv7) and x86-32 pcs (i686) at first.

## Dependencies
You may need the following packages, which are necessary to build the
cross-compiler, install the bootloader, go through the compilation
process and debug the resulting image
```sh
gcc make git rsync g++ bison flex texinfo libncurses-dev
kconfig-frontends moreutils swig python3-dev bc
u-boot-tools grub2-common xorriso dialog mtools
qemu-system-arm qemu-system-i386 gdb-multiarch
```

## Compilation
The image will be created in build/
```sh
make -f core.mk i686_defconfig

make -f deps.mk all
make -f core.mk all

make -f core.mk debug
```
