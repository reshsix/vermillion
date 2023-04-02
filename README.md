# Vermillion
An embedded operating system for ARM Cortex-A and x86-32 boards

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

## Supported boards
| Name | Configuration |
| ---- | ------------- |
| Orange Pi One | orangepi\_one\_defconfig |
| x86 generic | i686\_defconfig |

## Libc missing features
General
- locale.h
- setjmp.h
- stdio.h
- stdlib.h's atoX strtoX qsort bsearch
- time.h
- fenv.h (C99)
- inttypes.h (C99)

Wide strings
- stdlib.h mbX wcX
- wchar.h (NA1)
- wctype.h (NA1)

Math
- math.h
- complex.h (C99)
- tgmath.h (C99)
