# Vermillion
An embedded operating system for ARM Cortex-A and x86-32 boards

It intends to be used as a loader and HAL for embedded applications

## Development
The project is in alpha stage, lacking some libc functions and drivers

### Libc missing features
- stdlib.h's atoX strtoX qsort bsearch mbX wcX
- locale.h, setjmp.h, stdio.h, math.h, time.h
- wchar.h, wctype.h (NA1)
- fenv.h, inttypes.h, complex.h, tgmath.h (C99)

### Supported boards
| Name | Configuration |
| ---- | ------------- |
| Orange Pi One | orangepi\_one\_defconfig |
| x86 generic | i686\_defconfig |

## Build
You may need the following packages, which are necessary to build the
cross-compiler, install the bootloader, go through the compilation
process and debug the resulting image
```sh
gcc make git rsync g++ bison flex texinfo libncurses-dev
kconfig-frontends moreutils swig python3-dev bc
u-boot-tools grub2-common qemu-system-arm qemu-system-i686
gdb-multiarch
```

The image will be created in build/
```sh
make ${BOARD}_defconfig
make
make debug
```
