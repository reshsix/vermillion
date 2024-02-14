# Vermillion
A 32 bits embedded OS for my personal purposes

Code is licensed as GPLv3, and non-code resources can be used as CC BY-SA 4.0

## Development status
Alpha, some core features are still not implemented

## Support
- NanoPi NEO with ILI9488 screen on SPI0 (nanopi\_neo\_defconfig)
- OrangePi One with ILI9488 screen on SPI0 (orangepi\_one\_defconfig)
- x86-32 computers (i686\_defconfig)

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
